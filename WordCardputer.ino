#include "M5Cardputer.h"
#include "M5GFX.h"
#include <SD.h>
#include <ArduinoJson.h>

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
void loadWordsFromJSON() {
    if (!SD.begin()) {
        Serial.println("❌ SD 卡初始化失败");
        return;
    }

    File file = SD.open("/jp_words_study/words.json");
    if (!file) {
        Serial.println("❌ 未找到文件 /jp_words_study/words.json");
        return;
    }

    String jsonString;
    while (file.available()) jsonString += char(file.read());
    file.close();

    StaticJsonDocument<16384> doc;
    if (deserializeJson(doc, jsonString)) {
        Serial.println("❌ JSON 解析失败");
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

    canvas.createSprite(M5Cardputer.Display.width(), M5Cardputer.Display.height());
    canvas.setTextFont(&fonts::efontCN_16);

    loadWordsFromJSON();
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
