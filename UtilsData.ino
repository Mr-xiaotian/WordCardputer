/**
 * UtilsData.ino - 运行时词库数据与学习状态管理
 *
 * 负责管理设备内存中的词库状态与学习流程，包括：
 * - 基于熟练度的加权随机抽词
 * - score 脏标记与自动保存触发
 * - 听写/听读提示文本
 * - 当前已加载词库的统计计算
 * - 语言切换后的运行时状态重置
 * - 开始学习与错题本保存流程
 *
 * 与 SQLite 的直接交互已拆分到 UtilsDb.ino。
 */

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
        return 0;

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
 * 自动调用 autoSaveIfNeeded() 将 score 回写到数据库，避免每次评分都立即写盘。
 */
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
 * 检查 scoresDirty 标志，若当前已加载词库且存在未保存 score，
 * 则调用 `saveCurrentWordsToDB()` 批量写回数据库。
 * 保存完成后重置脏标记和计数器。
 */
void autoSaveIfNeeded() {
    if (!scoresDirty || selectedSource.isEmpty() || words.empty()) return;

    if (saveCurrentWordsToDB()) {
        Serial.println("[AutoSave] 进度已自动保存");
    } else {
        Serial.println("[AutoSave] 自动保存失败");
    }
    scoresDirty = false;
    dirtyCount = 0;
}

/**
 * 获取当前单词的听写提示文本
 *
 * 根据当前语言设置，返回用于语音播放和答案比对的文本。
 * 英语模式返回英文单词，日语模式返回日语假名。
 *
 * @param w 单词对象
 * @return 用于听写的目标文本（英文单词或日语假名）
 */
String dictationPromptText(const Word &w)
{
    if (currentLanguage == LANG_EN)
        return w.en;
    return w.jp;
}

/**
 * 从当前词库标签中提取用于显示的名称
 *
 * 迁移到数据库后，`selectedFilePath` 不再是真实文件路径，而是 UI 标签：
 * - `source`
 * - `source/chapter`
 *
 * 这里仍保留原有“取最后一段”的行为，使统计页、Web 面板展示简洁名称。
 *
 * @param path 当前词库标签
 * @return 显示用名称
 */
String statsFileName(const String &path)
{
    if (path.isEmpty()) {
        return "";
    }
    int pos = path.lastIndexOf('/');
    if (pos >= 0 && pos + 1 < (int)path.length())
    {
        return path.substring(pos + 1);
    }
    return path;
}

/**
 * 从当前词库计算学习统计数据
 *
 * 遍历所有单词，统计各等级（1-5）的数量、计算平均分和中位数，
 * 并根据平均分生成掌握程度评价文字（非常熟练/较为熟练/掌握中/不牢固/需要重点复习）。
 * 分数不在 1-5 范围内的默认按 3 处理。词库为空时设置提示文字并直接返回。
 */
void computeStatsFromWords()
{
    for (int i = 0; i < 6; i++)
    {
        statsCounts[i] = 0;
    }
    statsTotal = words.size();
    statsAvg = 0;
    statsMedian = 0;
    statsLevel = "";

    if (statsTotal == 0)
    {
        statsLevel = "词库为空";
        return;
    }

    std::vector<int> scores;
    scores.reserve(statsTotal);
    long sum = 0;

    for (auto &w : words)
    {
        int score = w.score;
        if (score < 1 || score > 5)
            score = 3;
        scores.push_back(score);
        statsCounts[score] += 1;
        sum += score;
    }

    statsAvg = (float)sum / (float)statsTotal;
    std::sort(scores.begin(), scores.end());

    if (statsTotal % 2 == 1)
    {
        statsMedian = scores[statsTotal / 2];
    }
    else
    {
        int mid = statsTotal / 2;
        statsMedian = (scores[mid - 1] + scores[mid]) / 2.0f;
    }

    if (statsAvg >= 4.5)
    {
        statsLevel = "非常熟练";
    }
    else if (statsAvg >= 3.8)
    {
        statsLevel = "较为熟练";
    }
    else if (statsAvg >= 3.0)
    {
        statsLevel = "掌握中";
    }
    else if (statsAvg >= 2.0)
    {
        statsLevel = "不牢固";
    }
    else
    {
        statsLevel = "需要重点复习";
    }
}

/**
 * 设置当前学习语言并更新相关状态
 *
 * 根据所选语言设置词库浏览根目录和音频根目录，
 * 同时重置当前虚拟目录、当前已选 source/chapter，以及已加载的单词列表。
 *
 * @param lang 要设置的语言枚举值（LANG_JP 或 LANG_EN）
 */
void setLanguage(StudyLanguage lang)
{
    currentLanguage = lang;
    if (currentLanguage == LANG_JP)
    {
        currentWordRoot = "/words_study/jp/word";
        currentAudioRoot = "/words_study/jp/audio";
    }
    else
    {
        currentWordRoot = "/words_study/en/word";
        currentAudioRoot = "/words_study/en/audio";
    }
    currentDir = currentWordRoot;
    selectedFilePath = "";
    selectedSource = "";
    selectedChapter = "";
    words.clear();
}

/**
 * 初始化学习模式并从数据库加载词库
 *
 * 加载新词库前先自动保存旧词库进度，然后根据当前选中的
 * `selectedSource` / `selectedChapter` 从数据库读取词条。
 * 加载成功后通过加权随机算法选取第一个单词并绘制闪卡。
 */
void startStudyMode()
{
    autoSaveIfNeeded();

    bool ok = loadWordsFromDB(selectedSource, selectedChapter);
    if (!ok)
    {
        drawCenterString(canvas, "词库加载失败", RED, 1.2);
        return;
    }
    else if (words.empty())
    {
        drawCenterString(canvas, "词库为空", RED, 1.2);
        return;
    }

    wordIndex = pickWeightedRandom();
    showMeaning = false;
    if (currentLanguage == LANG_EN)
        showAnkiSideA = true;
    drawStudyWord();
}
