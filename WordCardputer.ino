#include "M5Cardputer.h"
#include "M5GFX.h"
#include "esp_system.h"
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>

#define SD_SPI_SCK_PIN  40
#define SD_SPI_MISO_PIN 39
#define SD_SPI_MOSI_PIN 14
#define SD_SPI_CS_PIN   12

// --------- 模式定义 ----------
enum AppMode {
    MODE_LANG_SELECT,
    MODE_FILE_SELECT,
    MODE_STUDY,
    MODE_ESC_MENU,      //  ESC 菜单模式
    MODE_DICTATION,     // 听写模式
    MODE_LISTEN,        // 听读模式
    MODE_STATS,         // 学习统计模式
};

enum StudyLanguage {
    LANG_JP,
    LANG_EN
};

AppMode appMode = MODE_LANG_SELECT;
AppMode previousMode = MODE_FILE_SELECT;  // 记录上一个模式

// --------- 全局变量 ----------
M5Canvas canvas(&M5Cardputer.Display);
const int visibleLines = 4;
int soundVolume = 192;

StudyLanguage currentLanguage = LANG_JP;
String currentWordRoot = "/jp_words_study/word";
String currentAudioRoot = "/jp_words_study/audio";

String currentDir = "/jp_words_study/word";
String selectedFilePath = "";

std::vector<String> langItems = {"日语", "英语"};
int langIndex = 0;

unsigned long volumeMessageDeadline = 0;  // 音量消息显示截止时间

// ---------- 自动亮度管理 ----------
bool userAction = false;                  // 标记是否有用户操作
unsigned long lastActivityTime = 0;       // 上次活动时间
bool isDimmed = false;                    // 是否已进入省电模式
const unsigned long idleTimeout = 60000;  // 超过60秒无操作则降低亮度
const uint8_t normalBrightness = 200;     // 正常亮度
const uint8_t dimBrightness = 40;         // 降低后的亮度
int loopDelay = 30;                       // 动态延迟时间

// -------- 数据结构 ----------
struct Word {
    String jp;
    String zh;
    String kanji;
    String en;
    String pos;
    String phonetic;
    int tone;
    int score;  // 熟练度 1~5
};

std::vector<Word> words;
int wordIndex = 0;

struct DictError
{
    int wordIndex;
    String wrong;
};
std::vector<DictError> dictErrors;
int reviewPos = 0;         // 当前错误回顾的索引

// ---------- 函数声明（在其他 .ino 中实现） ----------
void initFileSelectMode();
void loopFileSelectMode();

void startStudyMode(const String &filePath);
void loopStudyMode();

void initEscMenuMode();
void loopEscMenuMode();

void initDictationMode();
void loopDictationMode();

void initListenMode();    
void loopListenMode();    

void initStatsMode();
void loopStatsMode();

void initLanguageSelectMode();
void loopLanguageSelectMode();
void setLanguage(StudyLanguage lang);

bool loadWordsFromJSON(const String &path);
bool saveListToJSON(const String &filepath, const std::vector<Word> &list);
void saveDictationMistakesAsWordList();

int pickWeightedRandom();

void drawAutoFitString(M5Canvas &cv, const String &text,
                       int x, int y, float baseSize);
void drawTopLeftString(M5Canvas &cv, const String &text);
void drawTopRightString(M5Canvas &cv, const String &text);
                       
void drawTextMenu(
    M5Canvas &cv,
    const String &title,
    const std::vector<String> &items,
    int selectedIndex,
    int scrollIndex,
    int visibleLines,
    const String &emptyText = "无项目",
    bool showBattery = true,
    bool showPager = true
);
void drawSimpleTable(
    M5Canvas &cv,
    int startY,
    int rowHeight,
    const std::vector<String> &headers,
    const std::vector<int> &colXs,
    float headerSize,
    float bodySize,
    const std::vector<std::vector<String>> &rows
);

String matchRomaji(const String &buffer, bool useKatakana);
void removeLastUTF8Char(String &s);

void drawLanguageSelect()
{
    drawTextMenu(
        canvas,
        "选择语言",
        langItems,
        langIndex,
        0,
        visibleLines,
        "无可选项",
        false,
        false
    );
}

void setLanguage(StudyLanguage lang)
{
    currentLanguage = lang;
    if (currentLanguage == LANG_JP)
    {
        currentWordRoot = "/jp_words_study/word";
        currentAudioRoot = "/jp_words_study/audio";
    }
    else
    {
        currentWordRoot = "/en_words_study/word";
        currentAudioRoot = "/en_words_study/audio";
    }
    currentDir = currentWordRoot;
    selectedFilePath = "";
    words.clear();
}

void initLanguageSelectMode()
{
    langIndex = 0;
    drawLanguageSelect();
}

void loopLanguageSelectMode()
{
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed())
    {
        auto st = M5Cardputer.Keyboard.keysState();
        userAction = true;

        for (auto c : st.word)
        {
            if (c == ';')
            {
                langIndex = (langIndex - 1 + langItems.size()) % langItems.size();
                drawLanguageSelect();
            }
            else if (c == '.')
            {
                langIndex = (langIndex + 1) % langItems.size();
                drawLanguageSelect();
            }
        }

        if (st.enter)
        {
            StudyLanguage lang = (langIndex == 0) ? LANG_JP : LANG_EN;
            String root = (lang == LANG_JP) ? "/jp_words_study" : "/en_words_study";
            if (!SD.exists(root))
            {
                canvas.fillSprite(BLACK);
                canvas.setTextDatum(middle_center);
                canvas.setTextColor(RED);
                canvas.drawString("未找到词库文件夹", canvas.width() / 2, canvas.height() / 2);
                canvas.pushSprite(0, 0);
                delay(600);
                drawLanguageSelect();
                return;
            }
            setLanguage(lang);
            appMode = MODE_FILE_SELECT;
            initFileSelectMode();
            return;
        }
    }
}

// =============== 主程序 ===============
void setup() {
    randomSeed(esp_random());
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    Serial.begin(115200);

    // 初始化音频输出
    M5.Speaker.begin();
    M5.Speaker.setVolume(soundVolume);  // 音量范围 0~255,建议 128~192

    // 手动初始化 SPI 与 SD 卡
    SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
    if (!SD.begin(SD_SPI_CS_PIN, SPI, 25000000)) {
        M5Cardputer.Display.println("SD 初始化失败");
        while (1) delay(10);
    }

    bool jpExists = SD.exists("/jp_words_study");
    bool enExists = SD.exists("/en_words_study");
    if (!jpExists && !enExists) {
        M5Cardputer.Display.println("未找到词库文件夹");
        while (1) delay(10);
    }

    // 初始化亮度
    M5Cardputer.Display.setBrightness(normalBrightness);
    lastActivityTime = millis();

    // 初始化显示
    canvas.createSprite(M5Cardputer.Display.width(), M5Cardputer.Display.height());
    canvas.setTextFont(&fonts::efontCN_16);

    // 开始进入语言选择模式
    initLanguageSelectMode();
}

void loop() {
    M5Cardputer.update();
    userAction = false;

    if (appMode == MODE_LANG_SELECT) {
        loopLanguageSelectMode();
    } else if (appMode == MODE_FILE_SELECT) {
        loopFileSelectMode();
    } else if (appMode == MODE_STUDY) {
        loopStudyMode();
    } else if (appMode == MODE_ESC_MENU) {
        loopEscMenuMode();
    } else if (appMode == MODE_DICTATION) {
        loopDictationMode();
    } else if (appMode == MODE_LISTEN) {  
        loopListenMode();
    } else if (appMode == MODE_STATS) {
        loopStatsMode();
    }

    // -------- 自动亮度控制 --------
    unsigned long now = millis();
    if (userAction) {
        lastActivityTime = now;
        if (isDimmed) {
            M5Cardputer.Display.setBrightness(normalBrightness);
            isDimmed = false;
            loopDelay = 30; // 恢复正常
        }
    } else if (!isDimmed && now - lastActivityTime > idleTimeout) {
        // 空闲超过设定时间 → 降低亮度
        M5Cardputer.Display.setBrightness(dimBrightness);
        isDimmed = true;
        loopDelay = 200;  // 节能模式延迟
    }

    delay(loopDelay);
}
