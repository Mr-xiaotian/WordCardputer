/**
 * main.cpp - 主程序入口
 *
 * 基于 M5Cardputer 的便携单词学习机。
 */

#include "globals.h"

// =============== 主程序 ===============

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
    canvas.setFont(&fonts::efontCN_16);

    // 开始进入启动画面
    initSplashMode();
}

void loop() {
    M5Cardputer.update();
    userAction = false;

    if (appMode == MODE_SPLASH) {
        loopSplashMode();
    } else if (appMode == MODE_LANG_SELECT) {
        loopLanguageSelectMode();
    } else if (appMode == MODE_CLASSIFY_SELECT) {
        loopClassifySelectMode();
    } else if (appMode == MODE_FILE_SELECT) {
        loopFileSelectMode();
    } else if (appMode == MODE_SCORE_SELECT) {
        loopScoreSelectMode();
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
    } else if (appMode == MODE_CLOCK) {
        loopClockMode();
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
            loopDelay = 30;
        }
    } else if (!isDimmed && now - lastActivityTime > idleTimeout) {
        M5Cardputer.Display.setBrightness(dimBrightness);
        isDimmed = true;
        loopDelay = 200;
    }

    delay(loopDelay);
}
