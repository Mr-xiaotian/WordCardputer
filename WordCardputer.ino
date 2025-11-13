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

// ---------- 自动亮度管理 ----------
unsigned long lastActivityTime = 0;       // 上次活动时间
bool isDimmed = false;                    // 是否已进入省电模式
const unsigned long idleTimeout = 60000;  // 超过60秒无操作则降低亮度
const uint8_t normalBrightness = 200;     // 正常亮度
const uint8_t dimBrightness = 40;         // 降低后的亮度
int loopDelay = 30;                       // 动态延迟时间

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
    int scrollOffset = 0;               // 👈 新增滚动偏移
    const int visibleLines = 4;         // 每屏最多显示几行
    bool selected = false;

    while (!selected) {
        menuCanvas.fillSprite(BLACK);

        // 标题（左上角）
        menuCanvas.setTextDatum(top_left);
        menuCanvas.setTextColor(GREEN);
        menuCanvas.drawString("选择词库文件", 8, 8);

        // 电量（右上角）
        int batteryLevel = M5Cardputer.Power.getBatteryLevel();
        menuCanvas.setTextDatum(top_right);
        menuCanvas.setTextColor(TFT_DARKGREY);
        menuCanvas.setTextSize(1.0);
        menuCanvas.drawString(String(batteryLevel) + " %", menuCanvas.width() - 8, 8);

        // 绘制完右上角后恢复对齐方式
        menuCanvas.setTextDatum(top_left);
        menuCanvas.setTextColor(WHITE);

        // ✅ 只绘制当前窗口范围的项目
        int end = min(scrollOffset + visibleLines, (int)files.size());
        for (int i = scrollOffset; i < end; i++) {
            int y = 40 + (i - scrollOffset) * 24;
            if (i == index) {
                menuCanvas.setTextColor(YELLOW);
                menuCanvas.drawString("> " + files[i], 8, y);
                menuCanvas.setTextColor(WHITE);
            } else {
                menuCanvas.drawString("  " + files[i], 8, y);
            }
        }

        // ✅ 显示滚动条提示（选配）
        if (files.size() > visibleLines) {
            menuCanvas.setTextColor(TFT_DARKGREY);
            menuCanvas.drawRightString(
                String(index + 1) + "/" + String(files.size()),
                menuCanvas.width() - 8,
                menuCanvas.height() - 24);
        }

        menuCanvas.pushSprite(0, 0);

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

    menuCanvas.fillSprite(BLACK);
    menuCanvas.setTextColor(CYAN);
    menuCanvas.drawString("加载中...", menuCanvas.width() / 2, menuCanvas.height() / 2);
    menuCanvas.pushSprite(0, 0);

    String chosen = "/jp_words_study/word/" + files[index];
    Serial.printf("已选择: %s\n", chosen.c_str());
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
        w.score = 3;  // 默认中等熟悉度
        if (w.jp.length() > 0) words.push_back(w);
    }

    Serial.printf("成功加载 %d 个单词\n", words.size());
}

// ------------------- 音频播放 -------------------
bool playWavFile(const String& path) {
    if (!SD.exists(path)) {
        Serial.printf("文件不存在: %s\n", path.c_str());
        M5.Speaker.tone(880, 80);
        return false;
    }

    File f = SD.open(path, FILE_READ);
    if (!f) {
        Serial.printf("无法打开: %s\n", path.c_str());
        M5.Speaker.tone(440, 100);
        return false;
    }

    size_t len = f.size();
    if (len == 0) {
        Serial.printf("空文件: %s\n", path.c_str());
        f.close();
        return false;
    }

    // 为短音频一次性分配缓冲；若文件可能偏大，建议改成分块 playRaw 流式
    uint8_t* buf = (uint8_t*)malloc(len);
    if (!buf) {
        Serial.println("内存分配失败");
        f.close();
        return false;
    }

    size_t readn = f.read(buf, len);
    f.close();
    if (readn == 0) {
        Serial.println("读取失败");
        free(buf);
        return false;
    }

    // 停掉正在播放的声音，避免叠加
    if (M5.Speaker.isPlaying()) M5.Speaker.stop();

    bool ok = M5.Speaker.playWav(buf, readn, 1, -1, true); // 传入数据指针
    if (!ok) {
        Serial.println("playWav 调用失败");
        free(buf);
        return false;
    }

    // 阻塞等待播放结束，再释放内存（关键！不要提前 free）
    while (M5.Speaker.isPlaying()) { delay(10); }
    free(buf);
    return true;
}

void playAudioForWord(const String& jpWord) {
    String path = "/jp_words_study/audio/" + jpWord + ".wav";

    // 检查文件是否存在
    if (!SD.exists(path)) {
        Serial.printf("无音频文件: %s\n", path.c_str());
        M5.Speaker.tone(880, 80);  // 提示音
        return;
    }

    // 如果正在播放旧音频则停止
    if (M5.Speaker.isPlaying()) {
        M5.Speaker.stop();
    }

    // 播放 SD 卡上的 WAV 文件
    bool ok = playWavFile(path);
    if (!ok) {
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
void drawAutoFitString(M5Canvas &cv, const String &text, int x, int y,
                       int maxWidth, float baseSize = 2.0, float minSize = 0.8) {
    // 自动缩放绘制文字（防止超出屏幕宽度）
    if (text.length() == 0) return;

    float size = baseSize;
    cv.setTextSize(size);
    int width = cv.textWidth(text);

    // 如果太宽则逐步缩小字号直到适配
    while (width > maxWidth && size > minSize) {
        size -= 0.1;
        cv.setTextSize(size);
        width = cv.textWidth(text);
    }

    // 水平居中
    cv.setTextDatum(middle_center);
    cv.drawString(text, x, y);
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

    if (showJPFirst) {
        // === 模式1：显示日语，隐藏中文 ===
        canvas.setTextColor(CYAN);
        drawAutoFitString(canvas, w.jp, canvas.width()/2, canvas.height()/2 - 25,
                        canvas.width() - 20, 2.2);  // 自动适配

        canvas.setTextColor(GREEN);
        canvas.setTextSize(1.3);
        canvas.drawString("Tone: " + String(w.tone), canvas.width()/2, canvas.height()/2 + 5);

        if (showMeaning) {
            canvas.setTextColor(YELLOW);
            drawAutoFitString(canvas, w.zh, canvas.width()/2, canvas.height()/2 + 40,
                            canvas.width() - 20, 1.5);  // 显示中文释义
        }

    } else {
        // === 模式2：显示中文，隐藏日语 ===
        canvas.setTextColor(YELLOW);
        drawAutoFitString(canvas, w.zh, canvas.width()/2, canvas.height()/2 - 25,
                        canvas.width() - 20, 2.0);  // 显示中文释义主行

        if (w.kanji.length() > 0) {
            canvas.setTextColor(ORANGE);
            canvas.setTextSize(1.4);
            canvas.drawString(w.kanji, canvas.width()/2, canvas.height()/2 + 5);
        }

        if (showMeaning) {
            canvas.setTextColor(CYAN);
            drawAutoFitString(canvas, w.jp, canvas.width()/2, canvas.height()/2 + 40,
                            canvas.width() - 20, 1.8);  // 显示日语原文
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

    // 选择词库
    String filePath = selectJsonFile();
    if (filePath.length() == 0) return;

    loadWordsFromJSON(filePath);
    wordIndex = pickWeightedRandom();
    drawWord();
}

void loop() {
    M5Cardputer.update();
    bool userAction = false;  // 标记是否有用户操作

    // A键 → 切换释义
    if (M5Cardputer.BtnA.wasPressed()) {
        showMeaning = !showMeaning;
        drawWord();
        userAction = true;
    }

    // 键盘操作
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

        // 检测字母 a
        for (auto c : status.word) {
            if (c == 'a' || c == 'A') {
                playAudioForWord(words[wordIndex].jp);
                userAction = true;
            }
        }

        // Enter = 记住，提升熟练度
        if (status.enter) {
            words[wordIndex].score = min(5, words[wordIndex].score + 1);
        }
        // Del = 不熟，降低熟练度
        else if (status.del) {
            words[wordIndex].score = max(1, words[wordIndex].score - 1);
        }
        if (status.enter || status.del) {
            wordIndex = pickWeightedRandom();
            showMeaning = false;
            showJPFirst = random(2);  // 👈 0 或 1 随机决定显示方向
            drawWord();
            userAction = true;
        }
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
