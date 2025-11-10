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
};

std::vector<Word> words;
int wordIndex = 0;
bool showMeaning = false;

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
    while (file.available()) {
        jsonString += char(file.read());
    }
    file.close();

    StaticJsonDocument<8192> doc;
    DeserializationError err = deserializeJson(doc, jsonString);

    if (err) {
        Serial.print("❌ JSON 解析失败: ");
        Serial.println(err.c_str());
        return;
    }

    words.clear();
    for (JsonObject obj : doc.as<JsonArray>()) {
        Word w;
        w.jp = obj["jp"] | "";
        w.zh = obj["zh"] | "";
        w.kanji = obj["kanji"].isNull() ? "" : (const char*)obj["kanji"];
        w.tone = obj["tone"] | 0;
        if (w.jp.length() > 0 && w.zh.length() > 0) {
            words.push_back(w);
        }
    }

    Serial.printf("✅ 成功加载 %d 个单词\n", words.size());
}

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

    // 显示日语 + 汉字
    canvas.setTextSize(2.0);
    canvas.setTextColor(CYAN);
    String jpLine = w.jp;
    if (w.kanji.length() > 0) {
        jpLine += " (" + w.kanji + ")";
    }
    canvas.drawString(jpLine, canvas.width()/2, canvas.height()/2 - 25);

    // 显示释义
    if (showMeaning) {
        canvas.setTextSize(1.5);
        canvas.setTextColor(YELLOW);
        canvas.drawString(w.zh, canvas.width()/2, canvas.height()/2 + 10);

        // 显示 tone
        canvas.setTextSize(1.2);
        canvas.setTextColor(GREEN);
        String toneStr = "Tone: " + String(w.tone);
        canvas.drawString(toneStr, canvas.width()/2, canvas.height()/2 + 40);
    }

    canvas.pushSprite(0, 0);
}

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    Serial.begin(115200);

    canvas.createSprite(M5Cardputer.Display.width(), M5Cardputer.Display.height());
    canvas.setTextFont(&fonts::efontCN_16);

    loadWordsFromJSON();
    if (words.empty()) {
        canvas.setTextColor(RED);
        canvas.drawString("请检查 SD 卡 JSON 文件", canvas.width()/2, canvas.height()/2);
        canvas.pushSprite(0,0);
        return;
    }

    randomSeed(millis());
    wordIndex = random(words.size());
    drawWord();
}

void loop() {
    M5Cardputer.update();

    // A 键切换显示释义
    if (M5Cardputer.BtnA.wasPressed()) {
        showMeaning = !showMeaning;
        drawWord();
    }

    // 空格/回车切换单词
    if (M5Cardputer.Keyboard.isChange()) {
        if (M5Cardputer.Keyboard.isPressed()) {
            Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
            if (status.enter || (status.word.size() == 1 && status.word[0] == ' ')) {
                wordIndex = random(words.size());
                showMeaning = false;
                drawWord();
            }
        }
    }

    delay(30);
}
