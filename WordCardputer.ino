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
    MODE_FILE_SELECT,
    MODE_STUDY
};

AppMode appMode = MODE_FILE_SELECT;

// --------- 全局变量 ----------
M5Canvas canvas(&M5Cardputer.Display);

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
    int tone;
    int score;  // 熟练度 1~5
};

std::vector<Word> words;
int wordIndex = 0;

// ---------- 函数声明（在其他 .ino 中实现） ----------
void initFileSelectMode();
void loopFileSelectMode();

void startStudyMode(const String &filePath);
void loopStudyMode();

void loadWordsFromJSON(const String &path);
int pickWeightedRandom();

void setup() {
    randomSeed(esp_random());
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    Serial.begin(115200);

    // 初始化音频输出
    M5.Speaker.begin();
    M5.Speaker.setVolume(192);  // 音量范围 0~255，建议 128~192

    // 手动初始化 SPI 与 SD 卡
    SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
    if (!SD.begin(SD_SPI_CS_PIN, SPI, 25000000)) {
        M5Cardputer.Display.println("SD 初始化失败");
        while (1) delay(10);
    }

    if (!SD.exists("/jp_words_study")) {
        M5Cardputer.Display.println("未找到 /jp_words_study 文件夹");
        while (1) delay(10);
    }

    // 初始化亮度
    M5Cardputer.Display.setBrightness(normalBrightness);
    lastActivityTime = millis();

    // 初始化显示
    canvas.createSprite(M5Cardputer.Display.width(), M5Cardputer.Display.height());
    canvas.setTextFont(&fonts::efontCN_16);

    // 开始进入文件选择模式
    initFileSelectMode();
}

void loop() {
    userAction = false;

    if (appMode == MODE_FILE_SELECT) {
        loopFileSelectMode();
    } else if (appMode == MODE_STUDY) {
        loopStudyMode();
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
}
