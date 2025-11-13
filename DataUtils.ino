void loadWordsFromJSON(const String &filepath) {
    File file = SD.open(filepath);
    if (!file) return;

    String jsonString;
    while (file.available()) jsonString += char(file.read());
    file.close();

    StaticJsonDocument<16384> doc;
    deserializeJson(doc, jsonString);

    words.clear();

    for (JsonObject obj : doc.as<JsonArray>()) {
        Word w;
        w.jp = obj["jp"] | "";
        w.zh = obj["zh"] | "";
        w.kanji = obj["kanji"] | "";
        w.tone = obj["tone"] | 0;
        w.score = 3;

        if (w.jp.length() > 0) words.push_back(w);
    }
}

int pickWeightedRandom() {
    int total = 0;
    for (auto &w : words) total += (6 - w.score);

    int r = random(total);
    int sum = 0;

    for (int i = 0; i < words.size(); i++) {
        sum += (6 - words[i].score);
        if (r < sum) return i;
    }
    return random(words.size());
}
