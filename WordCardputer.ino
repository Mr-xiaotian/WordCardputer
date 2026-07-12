/**
 * WordCardputer.ino - 主程序入口文件
 *
 * 基于 M5Cardputer 的便携单词学习机。支持日语和英语词库的闪卡学习、
 * 听写测试、听读练习及学习统计等多种模式。本文件定义了全局变量、
 * 数据结构、引脚配置，并实现 setup() 和 loop() 两个 Arduino 入口函数。
 */

#include "M5Cardputer.h"
#include "M5GFX.h"
#include "esp_system.h"
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <sqlite3.h>

#define SD_SPI_SCK_PIN  40
#define SD_SPI_MISO_PIN 39
#define SD_SPI_MOSI_PIN 14
#define SD_SPI_CS_PIN   12

// --------- 模式定义 ----------
/** 应用运行模式枚举，控制主循环中的分发逻辑 */
enum AppMode {
    MODE_LANG_SELECT,  // 语言选择模式
    MODE_FILE_SELECT,  // 文件选择模式
    MODE_STUDY,        // 学习模式
    MODE_ESC_MENU,     // ESC 菜单模式
    MODE_DICTATION,    // 听写模式
    MODE_DICTATION_REVIEW, // 听写错误回顾模式
    MODE_LISTEN,       // 听读模式
    MODE_STATS,        // 学习统计模式
    MODE_WORD_TABLE,   // 当前范围按分数词表模式
    MODE_WIFI_SCAN,    // WiFi 扫描连接模式
    MODE_KEY_HELP,     // 按键帮助模式
};

/** 学习语言枚举：日语或英语 */
enum StudyLanguage {
    LANG_JP,
    LANG_EN
};

AppMode appMode = MODE_LANG_SELECT;          // 当前应用模式
AppMode previousMode = MODE_FILE_SELECT;     // 记录上一个模式（用于 ESC 菜单返回）

// --------- 全局变量 ----------
M5Canvas canvas(&M5Cardputer.Display);       // 离屏画布，用于双缓冲绘制
const int visibleLines = 4;                  // 菜单一屏可见行数
int soundVolume = 192;                       // 扬声器音量（0~255）
bool wifiConnected = false;                  // WiFi 连接状态

StudyLanguage currentLanguage = LANG_JP;     // 当前学习语言
String currentWordRoot = "/words_study/jp/word";    // 当前词库浏览根（虚拟目录）
String currentAudioRoot = "/words_study/jp/audio";  // 当前音频根目录

String currentDir = "/words_study/jp/word";  // 词库浏览当前目录（虚拟目录）
String selectedFilePath = "";                // 当前词库标签（source 或 source/chapter）
String selectedSource = "";                  // 当前词库来源
String selectedChapter = "";                 // 当前词库章节；空表示整个 source

std::vector<String> langItems = {"日语", "英语"};  // 语言选择菜单项
int langIndex = 0;                           // 语言选择菜单索引

unsigned long volumeMessageDeadline = 0;  // 音量消息显示截止时间

// ---------- 自动保存 ----------
bool scoresDirty = false;                 // 是否有未保存的 score 变更
int dirtyCount = 0;                       // 累计变更次数
int autoSaveThreshold = 5;                // 每 N 次变更自动保存

// ---------- 自动亮度管理 ----------
bool userAction = false;                  // 标记是否有用户操作
unsigned long lastActivityTime = 0;       // 上次活动时间
bool isDimmed = false;                    // 是否已进入省电模式
unsigned long idleTimeout = 60000;        // 超过60秒无操作则降低亮度
uint8_t normalBrightness = 200;           // 正常亮度
uint8_t dimBrightness = 40;               // 降低后的亮度
int loopDelay = 30;                       // 动态延迟时间

// -------- 数据结构 ----------
/**
 * 单词数据结构，同时兼容日语和英语词库字段
 *
 * 运行时统一使用一个内存结构承载两种语言的词条：
 * - 日语使用 `jp / zh / kanji / romaji / tone / score`
 * - 英语使用 `en / zh / pos / phonetic / score`
 *
 * `dbId` 用于把当前内存中的 score 直接回写到 SQLite 对应记录。
 */
struct Word {
    int dbId;        // 数据库主键
    String jp;       // 日语假名
    String zh;       // 中文释义
    String kanji;    // 日语汉字写法
    String romaji;   // 日语罗马音
    String en;       // 英语单词
    String pos;      // 词性（英语）
    String phonetic; // 音标（英语）
    int tone;        // 声调（日语）
    int score;       // 熟练度评分 1~5（数值越高越熟练）
};

std::vector<Word> words;  // 当前加载的词库单词列表
int wordIndex = 0;        // 当前显示/学习的单词索引

/** 听写错误记录，保存回顾所需索引、数据库主键、错误输入和出错时间 */
struct DictError
{
    int wordIndex;    // 对应 words 列表中的索引，用于本轮错误回顾
    int wordDbId;     // 对应原词表的数据库主键，用于落库
    String wrong;     // 用户输入的错误答案
    String createdAt; // 记录错误发生时间
};
std::vector<DictError> dictErrors;  // 听写错误列表
int reviewPos = 0;                  // 当前错误回顾的索引

/** 错误回顾页面条目，既可来自本轮听写，也可来自数据库历史记录 */
struct DictationReviewEntry
{
    int wordDbId;      // 对应原词表主键，供后续扩展或定位
    String correct;    // 正确答案
    String wrong;      // 错误输入
    String createdAt;  // 错误时间
};
std::vector<DictationReviewEntry> dictationReviewEntries;
int dictationReviewIndex = 0;
String dictationReviewTitle = "错误回顾";

// ---------- 配置 ----------
struct WiFiCredential {
    String ssid;
    String pass;
};
std::vector<WiFiCredential> savedWiFiList;

// ---------- 函数声明（在其他 .ino 中实现） ----------

// --- ModeFileSelect.ino ---
void initFileSelectMode();
void loopFileSelectMode();

// --- ModeStudy.ino ---
void loopStudyMode();

// --- ModeEscMenu.ino ---
void initEscMenuMode();
void loopEscMenuMode();

// --- ModeDictation.ino ---
void initDictationMode();
void loopDictationMode();

// --- ModeDictationReview.ino ---
void drawDictationReviewPage();
void initDictationReviewFromSession();
void initDictationReviewHistoryMode();
void loopDictationReviewMode();

// --- ModeListen.ino ---
void initListenMode();
void loopListenMode();

// --- ModeStats.ino ---
void initStatsMode();
void loopStatsMode();

// --- ModeWordTable.ino ---
void initWordTableMode();
void loopWordTableMode();

// --- ModeLangSelect.ino ---
void initLanguageSelectMode();
void loopLanguageSelectMode();

// --- ModeWiFiScan.ino ---
void initWiFiScanMode();
void loopWiFiScanMode();

// --- ModeKeyHelp.ino ---
void initKeyHelpMode();
void loopKeyHelpMode();

// --- UtilsDb.ino ---
// SQLite 访问、词库浏览、导入导出辅助。
const char *currentWordTable();
const char *currentSourceTable();
const char *currentDictationErrorTable();
String sqliteColumnText(sqlite3_stmt *stmt, int col);
bool openVocabularyDb(sqlite3 **db);
bool prepareStatement(sqlite3 *db, const String &sql, sqlite3_stmt **stmt);
bool loadWordsFromDB(const String &source, const String &chapter);
bool saveCurrentWordsToDB();
bool saveWordListToDB(const String &source, const String &chapter, const std::vector<Word> &list);
bool saveDictationErrorsToDB(const std::vector<DictError> &errors);
bool loadDictationReviewEntriesFromDB(std::vector<DictationReviewEntry> &items);
bool loadSourceList(std::vector<String> &items);
bool loadChapterList(const String &source, std::vector<String> &items);
bool sourceHasChapters(const String &source);
int normalizeScoreValue(int score);
bool isValidVocabPath(const String &path);
bool parseVocabPath(const String &path, bool &isRoot, String &source, String &chapter);
bool deriveUploadTarget(const String &path, const String &filename, String &source, String &chapter);
bool importJsonFileToDb(const String &jsonPath, const String &source, const String &chapter, int &importedCount, String &error);

// --- UtilsData.ino ---
// 运行时词库状态、统计、自动保存与学习流程。
void autoSaveIfNeeded();
void markScoreDirty();
int pickWeightedRandom();
String dictationPromptText(const Word &w);
String statsFileName(const String &path);
void computeStatsFromWords();
void setLanguage(StudyLanguage lang);
void startStudyMode();

// --- UtilsString.ino ---
void drawAutoFitString(M5Canvas &cv, const String &text,
                       int x, int y, float baseSize);
void drawTopLeftString(M5Canvas &cv, const String &text, uint16_t color, float size);
void drawTopRightString(M5Canvas &cv, const String &text, uint16_t color, float size);
void drawCenterString(M5Canvas &cv, const String &message, uint16_t color, float size);
bool isEnglishInputChar(char c);
String normalizeEnglishAnswer(String s);

// --- UtilsIme.ino ---
void commitCandidate();
bool isSokuonConsonant(char c);
String matchRomaji(const String &buffer, bool useKatakana);
void removeLastUTF8Char(String &s);

// --- UtilsMenu.ino ---
void navigateMenu(int &index, int &scroll, int itemCount, int visible, bool moveUp);
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

// --- UtilsAudio.ino ---
bool adjustVolume(char c);

// --- UtilsConfig.ino ---
bool saveAppConfig();
void loadAppConfig();

// --- UtilsWiFi.ino ---
String getNtpTimeString();
String rssiIndicator(int rssi);
void processWiFiScanResults(int count);
void attemptWiFiConnect();
void saveWiFiCredential(const String &ssid, const String &pass);
String findSavedPassword(const String &ssid);

// --- UtilsWebServer.ino ---
void initWebServer();
void handleWebServer();
extern bool webServerRunning;

// =============== 主程序 ===============

/**
 * 设备初始化
 *
 * 依次完成以下初始化工作：
 * 1. 初始化随机数种子、M5Cardputer 硬件、串口
 * 2. 初始化音频输出（扬声器）
 * 3. 手动初始化 SPI 总线并挂载 SD 卡
 * 4. 初始化 SQLite 并检查词库数据库是否存在
 * 5. 设置屏幕亮度、创建离屏画布
 * 6. 进入语言选择模式
 */
void setup() {
    randomSeed(esp_random());
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    Serial.begin(115200);

    // 初始化音频输出
    M5.Speaker.begin();

    // 手动初始化 SPI 与 SD 卡
    SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
    if (!SD.begin(SD_SPI_CS_PIN, SPI, 25000000)) {
        M5Cardputer.Display.println("SD 初始化失败");
        while (1) delay(10);
    }

    // 初始化 SQLite 运行时
    sqlite3_initialize();

    // 至少保证一种语言的词库数据库存在
    bool jpExists = SD.exists("/words_study/jp/jp_words.db");
    bool enExists = SD.exists("/words_study/en/en_words.db");
    if (!jpExists && !enExists) {
        M5Cardputer.Display.println("未找到词库数据库");
        while (1) delay(10);
    }

    // 加载配置并同步语言/音量/亮度
    loadAppConfig();
    setLanguage(currentLanguage);
    M5.Speaker.setVolume(soundVolume);

    // 初始化亮度
    M5Cardputer.Display.setBrightness(normalBrightness);
    lastActivityTime = millis();

    // 初始化显示
    canvas.createSprite(M5Cardputer.Display.width(), M5Cardputer.Display.height());
    canvas.setTextFont(&fonts::efontCN_16);

    // 开始进入语言选择模式
    initLanguageSelectMode();
}

/**
 * 主循环
 *
 * 每次循环执行以下操作：
 * 1. 刷新 M5Cardputer 外设状态
 * 2. 根据当前 appMode 分发到对应模式的 loop 函数
 * 3. 自动亮度控制：若超过 idleTimeout 无用户操作则降低屏幕亮度以省电，
 *    用户有操作时立即恢复正常亮度
 * 4. 动态延迟：省电模式下使用较长延迟以降低 CPU 功耗
 */
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
    } else if (appMode == MODE_DICTATION_REVIEW) {
        loopDictationReviewMode();
    } else if (appMode == MODE_LISTEN) {  
        loopListenMode();
    } else if (appMode == MODE_STATS) {
        loopStatsMode();
    } else if (appMode == MODE_WORD_TABLE) {
        loopWordTableMode();
    } else if (appMode == MODE_WIFI_SCAN) {
        loopWiFiScanMode();
    } else if (appMode == MODE_KEY_HELP) {
        loopKeyHelpMode();
    }

    // -------- Web 服务器处理 --------
    handleWebServer();

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
