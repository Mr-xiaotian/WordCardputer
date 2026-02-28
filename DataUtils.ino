bool loadWordsFromJSON(const String &filepath)
{
    File file = SD.open(filepath);
    if (!file)
    {
        Serial.printf("无法打开文件: %s\n", filepath.c_str());
        return false;
    }

    size_t fileSize = file.size();
    if (fileSize == 0)
    {
        Serial.printf("空文件: %s\n", filepath.c_str());
        file.close();
        return false;
    }

    size_t capacity = static_cast<size_t>(fileSize * 1.2) + 1024;
    DynamicJsonDocument doc(capacity);
    DeserializationError err = deserializeJson(doc, file);
    file.close();
    if (err)
    {
        Serial.printf("JSON 解析失败: %s\n", err.c_str());
        return false;
    }
    if (!doc.is<JsonArray>())
    {
        Serial.printf("JSON 结构不为数组: %s\n", filepath.c_str());
        return false;
    }

    words.clear();

    for (JsonObject obj : doc.as<JsonArray>())
    {
        Word w;
        w.jp = "";
        w.zh = "";
        w.kanji = "";
        w.en = "";
        w.pos = "";
        w.phonetic = "";
        w.tone = -1;
        int score = obj["score"] | 3;
        if (score < 1)
            score = 1;
        else if (score > 5)
            score = 5;
        w.score = score;

        if (currentLanguage == LANG_JP)
        {
            w.jp = obj["jp"] | "";
            w.zh = obj["zh"] | "";
            w.kanji = obj["kanji"] | "";
            w.tone = obj["tone"] | -1;
            if (w.jp.length() > 0)
                words.push_back(w);
        }
        else if (currentLanguage == LANG_EN)
        {
            w.en = obj["word"] | "";
            w.zh = obj["zh"] | "";
            w.pos = obj["pos"] | "";
            w.phonetic = obj["phonetic"] | "";
            if (w.en.length() > 0)
                words.push_back(w);
        }
    }

    return !words.empty();
}

bool saveListToJSON(const String &filepath, const std::vector<Word> &list)
{
    if (SD.exists(filepath))
    {
        SD.remove(filepath);
    }
    File file = SD.open(filepath, FILE_WRITE);
    if (!file)
    {
        Serial.printf("无法写入文件: %s\n", filepath.c_str());
        return false;
    }

    size_t totalStringLen = 0;
    for (auto &w : list)
    {
        totalStringLen += w.jp.length();
        totalStringLen += w.zh.length();
        totalStringLen += w.kanji.length();
        totalStringLen += w.en.length();
        totalStringLen += w.pos.length();
        totalStringLen += w.phonetic.length();
    }
    size_t objectSize = (currentLanguage == LANG_JP) ? JSON_OBJECT_SIZE(5) : JSON_OBJECT_SIZE(5);
    size_t capacity = JSON_ARRAY_SIZE(list.size()) +
                      list.size() * objectSize +
                      totalStringLen +
                      list.size() * 16 +
                      256;
    DynamicJsonDocument doc(capacity);
    JsonArray arr = doc.to<JsonArray>();

    for (auto &w : list)
    {
        JsonObject obj = arr.createNestedObject();
        if (currentLanguage == LANG_JP)
        {
            obj["jp"] = w.jp;
            obj["zh"] = w.zh;
            obj["kanji"] = w.kanji;
            obj["tone"] = w.tone;
            obj["score"] = w.score;
        }
        else
        {
            obj["word"] = w.en;
            obj["zh"] = w.zh;
            obj["pos"] = w.pos;
            obj["phonetic"] = w.phonetic;
            obj["score"] = w.score;
        }
    }

    if (serializeJsonPretty(doc, file) == 0)
    {
        Serial.println("写入 JSON 失败！");
        file.close();
        return false;
    }

    file.close();
    return true;
}

int pickWeightedRandom()
{
    if (words.empty())
        return 0; // 兜底,调用方需要自己判断

    int total = 0;
    for (auto &w : words)
        total += (6 - w.score);

    int r = random(total);
    int sum = 0;

    for (int i = 0; i < words.size(); i++)
    {
        sum += (6 - words[i].score);
        if (r < sum)
            return i;
    }
    return random(words.size());
}

void saveDictationMistakesAsWordList() {
    if (dictErrors.empty()) return;

    String folder = currentWordRoot + "/Mistake";
    if (!SD.exists(folder)) {
        SD.mkdir(folder);
    }

    // 自动生成文件名
    String filename = folder + "/mistake_" + String(millis()) + ".json";

    // 构造纯 Word 列表
    std::vector<Word> mistakeList;
    mistakeList.reserve(dictErrors.size());
    for (auto &e : dictErrors) {
        mistakeList.push_back(words[e.wordIndex]);
    }

    // 调用已有的通用方法
    saveListToJSON(filename, mistakeList);

    Serial.printf("错词已经另存为词库: %s\n", filename.c_str());
}
