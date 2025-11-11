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

// ------------------- 工具函数 -------------------
String selectJsonFile() {
    M5Cardputer.Display.fillScreen(BLACK);
    M5Canvas canvas(&M5Cardputer.Display);
    canvas.createSprite(M5Cardputer.Display.width(), M5Cardputer.Display.height());
    canvas.setTextFont(&fonts::efontCN_16);
    canvas.setTextSize(1.2);

    std::vector<String> files;

    File root = SD.open("/jp_words_study");
    if (!root || !root.isDirectory()) {
        canvas.println("无法打开 /jp_words_study/");
        canvas.pushSprite(0, 0);
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
        canvas.println("未找到任何 JSON 文件");
        canvas.pushSprite(0, 0);
        delay(3000);
        return "";
    }

    int index = 0;
    int scrollOffset = 0;               // 👈 新增滚动偏移
    const int visibleLines = 4;         // 每屏最多显示几行
    bool selected = false;

    while (!selected) {
        canvas.fillSprite(BLACK);
        canvas.setTextColor(GREEN);
        canvas.setTextDatum(top_left);
        canvas.drawString("选择词库文件", 8, 8);
        canvas.setTextColor(WHITE);

        // ✅ 只绘制当前窗口范围的项目
        int end = min(scrollOffset + visibleLines, (int)files.size());
        for (int i = scrollOffset; i < end; i++) {
            int y = 40 + (i - scrollOffset) * 24;
            if (i == index) {
                canvas.setTextColor(YELLOW);
                canvas.drawString("> " + files[i], 8, y);
                canvas.setTextColor(WHITE);
            } else {
                canvas.drawString("  " + files[i], 8, y);
            }
        }

        // ✅ 显示滚动条提示（选配）
        if (files.size() > visibleLines) {
            canvas.setTextColor(TFT_DARKGREY);
            canvas.drawRightString(
                String(index + 1) + "/" + String(files.size()),
                canvas.width() - 8,
                canvas.height() - 24);
        }

        canvas.pushSprite(0, 0);

        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto status = M5Cardputer.Keyboard.keysState();

            for (auto c : status.word) {
                if (c == ';') {
                    index = (index - 1 + files.size()) % files.size();
                    if (index == files.size() - 1) {
                        // ✅ 从第一行上翻到最后一行
                        scrollOffset = max(0, (int)files.size() - visibleLines);
                    } else if (index < scrollOffset) {
                        scrollOffset = index;
                    }
                }

                if (c == '.') {
                    index = (index + 1) % files.size();
                    if (index == 0) {
                        // ✅ 从最后一行下翻回到第一行
                        scrollOffset = 0;
                    } else if (index >= scrollOffset + visibleLines) {
                        scrollOffset = index - visibleLines + 1;
                    }
                }
            }

            if (status.enter) {
                selected = true;
                break;
            }
        }

        delay(60);
    }

    canvas.fillSprite(BLACK);
    canvas.setTextColor(CYAN);
    canvas.drawString("加载中...", canvas.width() / 2, canvas.height() / 2);
    canvas.pushSprite(0, 0);

    String chosen = "/jp_words_study/" + files[index];
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
    if (deserializeJson(doc, jsonString)) {
        Serial.println("JSON 解析失败");
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

    // 假名
    canvas.setTextSize(2.2);
    canvas.setTextColor(CYAN);
    canvas.drawString(w.jp, canvas.width()/2, canvas.height()/2 - 25);

    // Tone
    canvas.setTextSize(1.3);
    canvas.setTextColor(GREEN);
    canvas.drawString("Tone: " + String(w.tone), canvas.width()/2, canvas.height()/2 + 5);

    // 显示释义
    if (showMeaning) {
        if (w.kanji.length() > 0) {
            // canvas.setTextColor(ORANGE);
            // canvas.setTextSize(1.6);
            // canvas.drawString(w.kanji, canvas.width()/2, canvas.height()/2 + 40);
        }
        canvas.setTextColor(YELLOW);
        canvas.setTextSize(1.5);
        canvas.drawString(w.zh, canvas.width()/2, canvas.height()/2 + 40);
    }

    // 熟练度提示
    canvas.setTextColor(TFT_DARKGREY);
    canvas.setTextSize(1.0);
    canvas.drawString("Score: " + String(words[wordIndex].score), 50, 15);

    canvas.pushSprite(0, 0);
}

// ------------------- 主程序 -------------------
void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    Serial.begin(115200);

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
    randomSeed(millis());
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

        // 回车 = 记住，提升熟练度
        if (status.enter) {
            words[wordIndex].score = min(5, words[wordIndex].score + 1);
            wordIndex = pickWeightedRandom();
            showMeaning = false;
            drawWord();
        }
        // <- = 不熟，降低熟练度
        else if (status.del) {
            words[wordIndex].score = max(0, words[wordIndex].score - 1);
            wordIndex = pickWeightedRandom();
            showMeaning = false;
            drawWord();
        }
    }

    delay(30);
}
