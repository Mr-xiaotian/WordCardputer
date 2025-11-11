#include "M5Cardputer.h"
#include "M5GFX.h"
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>

#define SD_SPI_SCK_PIN  40
#define SD_SPI_MISO_PIN 39
#define SD_SPI_MOSI_PIN 14
#define SD_SPI_CS_PIN   12

M5Canvas canvas(&M5Cardputer.Display);

struct Word {
    String jp;
    String zh;
    String kanji;
    int tone;
    int score;  // 熟练度 0~5
};

std::vector<Word> words;
int wordIndex = 0;
bool showMeaning = false;
bool showJPFirst = true;  // true=先显示日语, false=先显示中文

// ------------------- 工具函数 -------------------
String selectJsonFile() {
    M5Cardputer.Display.fillScreen(BLACK);
    M5Canvas menuCanvas(&M5Cardputer.Display);
    menuCanvas.createSprite(M5Cardputer.Display.width(), M5Cardputer.Display.height());
    menuCanvas.setTextFont(&fonts::efontCN_16);
    menuCanvas.setTextSize(1.2);

    std::vector<String> files;

    File root = SD.open("/jp_words_study/word");
    if (!root || !root.isDirectory()) {
        menuCanvas.println("无法打开 /jp_words_study/word/");
        menuCanvas.pushSprite(0, 0);
        delay(3000);
        return "";
    }

    while (true) {
        File entry = root.openNextFile();
        if (!entry) break;
        String name = entry.name();
        if (name.endsWith(".json")) {
            files.push_back(name);
        }
        entry.close();
    }
    root.close();

    if (files.empty()) {
        menuCanvas.println("未找到任何 JSON 文件");
        menuCanvas.pushSprite(0, 0);
        delay(3000);
        return "";
    }

    int index = 0;
    bool selected = false;

    while (!selected) {
        menuCanvas.fillSprite(BLACK);
        menuCanvas.setTextColor(GREEN);
        menuCanvas.setTextDatum(top_left);
        menuCanvas.drawString("选择词库文件", 8, 8); // 左上角标题
        menuCanvas.setTextColor(WHITE);

        for (int i = 0; i < files.size(); i++) {
            int y = 40 + i * 24;
            if (i == index) {
                menuCanvas.setTextColor(YELLOW);
                menuCanvas.drawString("> " + files[i], 8, y);
                menuCanvas.setTextColor(WHITE);
            } else {
                menuCanvas.drawString("  " + files[i], 8, y);
            }
        }

        menuCanvas.pushSprite(0, 0);

        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto status = M5Cardputer.Keyboard.keysState();

            for (auto c : status.word) {
                if (c == ';') index = (index - 1 + files.size()) % files.size();  // 上
                if (c == '.') index = (index + 1) % files.size();                 // 下
                if (c == ',') index = (index - 1 + files.size()) % files.size();  // 左(备用)
                if (c == '/') index = (index + 1) % files.size();                 // 右(备用)
            }

            if (status.enter) {
                selected = true;
                break;
            }
        }

        delay(60);
    }

    menuCanvas.fillSprite(BLACK);
    menuCanvas.setTextColor(CYAN);
    menuCanvas.drawString("加载中...", menuCanvas.width() / 2, menuCanvas.height() / 2);
    menuCanvas.pushSprite(0, 0);

    String chosen = "/jp_words_study/word/" + files[index];
    Serial.printf("✅ 已选择: %s\n", chosen.c_str());
    return chosen;
}

void loadWordsFromJSON(String filepath) {
    File file = SD.open(filepath);
    if (!file) {
        Serial.println("未找到文件: " + filepath);
        return;
    }

    String jsonString;
    while (file.available()) jsonString += char(file.read());
    file.close();

    StaticJsonDocument<16384> doc;
    DeserializationError err = deserializeJson(doc, jsonString);
    if (err) {
        Serial.printf("JSON 解析失败: %s\n", err.c_str());
        return;
    }

    words.clear();
    for (JsonObject obj : doc.as<JsonArray>()) {
        Word w;
        w.jp = obj["jp"] | "";
        w.zh = obj["zh"] | "";
        w.kanji = obj["kanji"].isNull() ? "" : (const char*)obj["kanji"];
        w.tone = obj["tone"] | 0;
        w.score = 2;  // 默认中等熟悉度
        if (w.jp.length() > 0) words.push_back(w);
    }

    Serial.printf("✅ 成功加载 %d 个单词\n", words.size());
}

void playAudioForWord(const String& jpWord) {
    String path = "/jp_words_study/audio/" + jpWord + ".wav";

    // 检查文件是否存在
    if (!SD.exists(path)) {
        Serial.printf("⚠️ 无音频文件: %s\n", path.c_str());
        M5.Speaker.tone(880, 80);  // 提示音
        return;
    }

    // 如果正在播放旧音频则停止
    if (M5.Speaker.isPlaying()) {
        M5.Speaker.stop();
    }

    Serial.printf("🎵 播放音频: %s\n", path.c_str());

    // 播放 SD 卡上的 WAV 文件
    bool ok = M5.Speaker.playWav(path.c_str(), true); // true = 阻塞直到播放完毕
    if (!ok) {
        Serial.println("❌ 播放失败");
        M5.Speaker.tone(440, 100);
    }
}

// ------------------- 抽词算法 -------------------
int pickWeightedRandom() {
    int totalWeight = 0;
    for (auto &w : words) totalWeight += (6 - w.score);
    int r = random(totalWeight);
    int sum = 0;
    for (int i = 0; i < words.size(); i++) {
        sum += (6 - words[i].score);
        if (r < sum) return i;
    }
    return random(words.size());
}

// ------------------- 显示逻辑 -------------------
void drawWord() {
    M5Cardputer.Display.fillScreen(BLACK);
    canvas.fillSprite(BLACK);
    canvas.setTextDatum(middle_center);

    if (words.empty()) {
        canvas.setTextColor(RED);
        canvas.drawString("未找到单词数据", canvas.width()/2, canvas.height()/2);
        canvas.pushSprite(0, 0);
        return;
    }

    Word &w = words[wordIndex];

    if (showJPFirst) {
        // === 模式1：显示日语，隐藏中文 ===
        canvas.setTextSize(2.2);
        canvas.setTextColor(CYAN);
        canvas.drawString(w.jp, canvas.width()/2, canvas.height()/2 - 25);

        canvas.setTextSize(1.3);
        canvas.setTextColor(GREEN);
        canvas.drawString("Tone: " + String(w.tone), canvas.width()/2, canvas.height()/2 + 5);

        if (showMeaning) {
            canvas.setTextColor(YELLOW);
            canvas.setTextSize(1.5);
            canvas.drawString(w.zh, canvas.width()/2, canvas.height()/2 + 40);
        }
    } else {
        // === 模式2：显示中文，隐藏日语 ===
        canvas.setTextSize(2.0);
        canvas.setTextColor(YELLOW);
        canvas.drawString(w.zh, canvas.width()/2, canvas.height()/2 - 25);
        
        if (w.kanji.length() > 0) {
            canvas.setTextColor(ORANGE);
            canvas.setTextSize(1.4);
            canvas.drawString(w.kanji, canvas.width()/2, canvas.height()/2 + 5);
        }

        if (showMeaning) {
            canvas.setTextColor(CYAN);
            canvas.setTextSize(1.8);
            canvas.drawString(w.jp, canvas.width()/2, canvas.height()/2 + 40);
        }
    }

    // 熟练度提示
    canvas.setTextColor(TFT_DARKGREY);
    canvas.setTextSize(1.0);
    canvas.drawString("Score: " + String(words[wordIndex].score), 50, 15);

    // // 底部提示栏
    // canvas.setTextDatum(bottom_center);
    // canvas.setTextColor(TFT_LIGHTGREY);
    // canvas.setTextSize(0.8);
    // canvas.drawString("Go:释义  Enter:记住  Del:不熟", canvas.width()/2, canvas.height() - 5);

    canvas.pushSprite(0, 0);
}

// ------------------- 主程序 -------------------
void setup() {
    randomSeed(millis());
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    Serial.begin(115200);

    // ✅ 初始化音频输出
    M5.Speaker.begin();
    M5.Speaker.setVolume(192);  // 音量范围 0~255，建议 128~192

    // ✅ 手动初始化 SPI 与 SD 卡
    SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
    if (!SD.begin(SD_SPI_CS_PIN, SPI, 25000000)) {
        M5Cardputer.Display.println("SD 初始化失败");
        while (1) delay(10);
    }

    if (!SD.exists("/jp_words_study")) {
        M5Cardputer.Display.println("未找到 /jp_words_study 文件夹");
        while (1) delay(10);
    }

    // ✅ 初始化显示
    canvas.createSprite(M5Cardputer.Display.width(), M5Cardputer.Display.height());
    canvas.setTextFont(&fonts::efontCN_16);

    // ✅ 选择词库
    String filePath = selectJsonFile();
    if (filePath.length() == 0) return;

    loadWordsFromJSON(filePath);
    wordIndex = pickWeightedRandom();
    drawWord();
}

void loop() {
    M5Cardputer.update();

    // A键 → 切换释义
    if (M5Cardputer.BtnA.wasPressed()) {
        showMeaning = !showMeaning;
        drawWord();
    }

    // 键盘操作
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

        // 检测字母 a
        for (auto c : status.word) {
            if (c == 'a' || c == 'A') {
                playAudioForWord(words[wordIndex].jp);
            }
        }

        // Enter = 记住，提升熟练度
        if (status.enter) {
            words[wordIndex].score = min(5, words[wordIndex].score + 1);
        }
        // Del = 不熟，降低熟练度
        else if (status.del) {
            words[wordIndex].score = max(0, words[wordIndex].score - 1);
        }
        if (status.enter || status.del) {
            wordIndex = pickWeightedRandom();
            showMeaning = false;
            showJPFirst = random(2);  // 👈 0 或 1 随机决定显示方向
            drawWord();
        }
    }

    delay(30);
}
