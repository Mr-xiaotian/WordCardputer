/**
 * DataUtils.ino - 数据持久化与词库管理工具
 *
 * 负责从 SD 卡加载/保存 JSON 格式的词库文件，包括：
 * - 词库的读取与解析（支持日语/英语两种格式）
 * - 词库的序列化与写入
 * - 基于熟练度的加权随机抽词算法
 * - 学习进度自动保存机制
 * - 听写错词的导出功能
 */

/**
 * 从 SD 卡加载 JSON 格式的词库文件
 *
 * 读取指定路径的 JSON 文件，解析为 Word 结构体数组并存入全局变量 words。
 * 根据 currentLanguage 决定解析日语字段还是英语字段。
 * 每个单词的 score（熟练度）会被限制在 1~5 范围内，默认值为 3。
 *
 * @param filepath SD 卡上 JSON 文件的完整路径
 * @return true 加载成功且词库不为空；false 文件不存在、为空、解析失败或无有效单词
 */
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

    if (fileSize >= 2)
    {
        Serial.printf("文件大小: %d 字节\n", fileSize);
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
            w.en = obj["en"] | "";
            w.zh = obj["zh"] | "";
            w.pos = obj["pos"] | "";
            w.phonetic = obj["phonetic"] | "";
            if (w.en.length() > 0)
                words.push_back(w);
        }
    }

    return !words.empty();
}

/**
 * 将单词列表保存为 JSON 文件到 SD 卡
 *
 * 将指定的 Word 列表序列化为 JSON 数组并写入 SD 卡。若目标文件已存在则先删除再写入。
 * 根据 currentLanguage 决定写入日语字段还是英语字段，同时保存 score 熟练度。
 * 输出格式为带缩进的 Pretty JSON，便于人工查看。
 *
 * @param filepath SD 卡上的目标文件路径
 * @param list 要保存的单词列表
 * @return true 写入成功；false 文件无法打开或序列化失败
 */
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
            obj["en"] = w.en;
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

/**
 * 基于熟练度的加权随机抽词
 *
 * 使用反向权重策略：熟练度越低的单词被抽中的概率越高。
 * 权重计算公式为 (6 - score)，即 score=1 的单词权重为 5，
 * score=5 的单词权重为 1，从而让不熟悉的单词更频繁出现。
 *
 * @return 被选中的单词在 words 数组中的索引；若词库为空返回 0
 */
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

/**
 * 标记学习进度已变更，达到阈值时触发自动保存
 *
 * 每次调用将 dirtyCount 加 1，当累计变更次数达到 autoSaveThreshold 时，
 * 自动调用 autoSaveIfNeeded() 将进度写入 SD 卡，避免频繁写盘。
 */
// 标记 score 已变更，累计达到阈值时自动保存
void markScoreDirty() {
    scoresDirty = true;
    dirtyCount++;
    if (dirtyCount >= autoSaveThreshold) {
        autoSaveIfNeeded();
    }
}

/**
 * 自动保存学习进度（如果有未保存的变更）
 *
 * 检查 scoresDirty 标志，若有未保存的变更且当前词库路径和单词列表有效，
 * 则将整个词库（含最新 score）写回原文件。保存完成后重置脏标记和计数器。
 */
// 如果有未保存的变更，则写入 SD 卡
void autoSaveIfNeeded() {
    if (!scoresDirty || selectedFilePath.isEmpty() || words.empty()) return;

    if (saveListToJSON(selectedFilePath, words)) {
        Serial.println("[AutoSave] 进度已自动保存");
    } else {
        Serial.println("[AutoSave] 自动保存失败");
    }
    scoresDirty = false;
    dirtyCount = 0;
}

/**
 * 将听写错误单词导出为新的词库文件
 *
 * 从 dictErrors 中提取所有答错的单词，保存到 SD 卡的 Mistake 子文件夹中。
 * 文件名格式为 "原词库名(时间戳).json"，方便用户后续针对错词进行重点复习。
 * 若 Mistake 文件夹不存在则自动创建。
 */
void saveDictationMistakesAsWordList() {
    if (dictErrors.empty()) return;

    String folder = currentWordRoot + "/Mistake";
    if (!SD.exists(folder)) {
        SD.mkdir(folder);
    }

    // 从当前词库文件名提取名称（去掉 .json 后缀）
    String baseName = selectedFilePath;
    int slashPos = baseName.lastIndexOf('/');
    if (slashPos >= 0) baseName = baseName.substring(slashPos + 1);
    if (baseName.endsWith(".json")) baseName = baseName.substring(0, baseName.length() - 5);

    // 用词库名 + 时间戳命名，如: word_01(26-03-30_16-28).json
    String timeStr = getNtpTimeString();
    String filename = folder + "/" + baseName + "(" + timeStr + ").json";

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
