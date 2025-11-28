bool loadWordsFromJSON(const String &filepath)
{
    File file = SD.open(filepath);
    if (!file)
    {
        Serial.printf("无法打开文件: %s\n", filepath.c_str());
        return false;
    }

    String jsonString;
    while (file.available())
        jsonString += char(file.read());
    file.close();

    StaticJsonDocument<16384> doc;
    deserializeJson(doc, jsonString);

    words.clear();

    for (JsonObject obj : doc.as<JsonArray>())
    {
        Word w;
        w.jp = obj["jp"] | "";
        w.zh = obj["zh"] | "";
        w.kanji = obj["kanji"] | "";
        w.tone = obj["tone"] | -1;
        w.score = obj["score"] | 3;

        if (w.jp.length() > 0)
            words.push_back(w);
    }

    return !words.empty();
}

bool saveListToJSON(const String &filepath, const std::vector<Word> &list)
{
    File file = SD.open(filepath, FILE_WRITE);
    if (!file)
    {
        Serial.printf("无法写入文件: %s\n", filepath.c_str());
        return false;
    }

    StaticJsonDocument<16384> doc;
    JsonArray arr = doc.to<JsonArray>();

    for (auto &w : list)
    {
        JsonObject obj = arr.createNestedObject();
        obj["jp"] = w.jp;
        obj["zh"] = w.zh;
        obj["kanji"] = w.kanji;
        obj["tone"] = w.tone;
        obj["score"] = w.score;
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
        return 0; // 兜底，调用方需要自己判断

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

    String folder = "/jp_words_study/word/Mistake";
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
