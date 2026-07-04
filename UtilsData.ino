/**
 * DataUtils.ino - 数据持久化与词库管理工具
 *
 * 负责从 SD 卡上的 SQLite 词库加载/保存学习数据，包括：
 * - 词库浏览（source / chapter）
 * - 按当前语言读取词条
 * - 保存 score 进度
 * - 基于熟练度的加权随机抽词
 * - 听写错词导出到数据库
 */

namespace {

/**
 * 获取当前语言对应的 SQLite 数据库路径
 *
 * 由于 SQLite 库通过 VFS 访问 SD 卡文件，这里返回带挂载前缀的路径：
 * - 日语：`/sd/words_study/jp/jp_words.db`
 * - 英语：`/sd/words_study/en/en_words.db`
 *
 * @return 当前语言对应的数据库完整路径
 */
String currentDbFilePath() {
    if (currentLanguage == LANG_JP) {
        return "/sd/words_study/jp/jp_words.db";
    }
    return "/sd/words_study/en/en_words.db";
}

/**
 * 获取当前语言对应的词表名称
 *
 * 日语与英语分表存储，避免字段互相干扰：
 * - 日语：`jp_words`
 * - 英语：`en_words`
 *
 * @return 当前语言对应的 SQLite 表名
 */
const char *currentWordTable() {
    return (currentLanguage == LANG_JP) ? "jp_words" : "en_words";
}

/**
 * 从 SQLite 查询结果中读取文本列
 *
 * SQLite 的 `sqlite3_column_text()` 返回 `unsigned char*`，
 * 且当字段为 NULL 时返回空指针。该函数统一完成空值保护和
 * `String` 转换，避免各处重复判断。
 *
 * @param stmt 已执行到当前行的查询语句
 * @param col  目标列索引
 * @return 转换后的文本；若为 NULL 则返回空字符串
 */
String sqliteColumnText(sqlite3_stmt *stmt, int col) {
    const unsigned char *txt = sqlite3_column_text(stmt, col);
    if (!txt) {
        return "";
    }
    return String(reinterpret_cast<const char *>(txt));
}

/**
 * 打开当前语言的词库数据库
 *
 * 根据当前语言解析数据库路径，并调用 `sqlite3_open()` 打开 SD 卡上的
 * SQLite 文件。打开失败时输出错误信息并确保返回空指针。
 *
 * @param db 输出参数，成功时写入数据库句柄
 * @return true 打开成功；false 打开失败
 */
bool openVocabularyDb(sqlite3 **db) {
    *db = nullptr;
    String dbPath = currentDbFilePath();
    int rc = sqlite3_open(dbPath.c_str(), db);
    if (rc != SQLITE_OK) {
        Serial.printf("无法打开数据库: %s (%s)\n", dbPath.c_str(), *db ? sqlite3_errmsg(*db) : "unknown");
        if (*db) {
            sqlite3_close(*db);
            *db = nullptr;
        }
        return false;
    }
    return true;
}

/**
 * 预编译 SQL 语句
 *
 * 对传入的 SQL 字符串执行 `sqlite3_prepare_v2()`，并在失败时打印
 * 数据库错误信息。该函数只负责 prepare，不负责 bind 与 step。
 *
 * @param db   已打开的数据库句柄
 * @param sql  要预编译的 SQL 语句
 * @param stmt 输出参数，成功时写入语句句柄
 * @return true 预编译成功；false 失败
 */
bool prepareStatement(sqlite3 *db, const String &sql, sqlite3_stmt **stmt) {
    *stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, stmt, nullptr);
    if (rc != SQLITE_OK) {
        Serial.printf("SQL prepare 失败: %s\n", sqlite3_errmsg(db));
        return false;
    }
    return true;
}

/**
 * 执行单列字符串查询并收集结果
 *
 * 用于读取 source 列表、chapter 列表等“单列多行”的简单查询。
 * 若传入 bindValue，则绑定到参数 1。
 *
 * @param sql       查询语句，结果集第一列必须是文本
 * @param bindValue 可选绑定值；为 nullptr 时不绑定
 * @param items     输出结果数组
 * @return true 查询成功；false 查询失败
 */
bool loadSingleColumnList(
    const String &sql,
    const String *bindValue,
    std::vector<String> &items
) {
    sqlite3 *db = nullptr;
    sqlite3_stmt *stmt = nullptr;
    items.clear();

    if (!openVocabularyDb(&db)) {
        return false;
    }
    if (!prepareStatement(db, sql, &stmt)) {
        sqlite3_close(db);
        return false;
    }

    if (bindValue) {
        sqlite3_bind_text(stmt, 1, bindValue->c_str(), -1, SQLITE_TRANSIENT);
    }

    while (true) {
        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            items.push_back(sqliteColumnText(stmt, 0));
            continue;
        }
        if (rc != SQLITE_DONE) {
            Serial.printf("SQL 查询失败: %s\n", sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return false;
        }
        break;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return true;
}

}  // namespace

/**
 * 从 SQLite 数据库加载指定 source / chapter 的词库
 *
 * 根据当前语言选择 `jp_words` 或 `en_words`，并按以下规则取数：
 * - `source` 必须匹配
 * - `chapter` 为空时，表示加载整个 source
 * - `chapter` 非空时，仅加载对应章节
 *
 * 加载结果会写入全局 `words`，并把数据库主键保存到 `Word::dbId`，
 * 后续保存 score 时直接回写该记录。
 *
 * @param source  词库来源
 * @param chapter 章节名；空字符串表示整个来源
 * @return true 加载成功且非空；false 打开失败、SQL 失败或无有效词条
 */
bool loadWordsFromDB(const String &source, const String &chapter)
{
    sqlite3 *db = nullptr;
    sqlite3_stmt *stmt = nullptr;
    words.clear();

    if (!openVocabularyDb(&db)) {
        return false;
    }

    String sql;
    if (currentLanguage == LANG_JP) {
        sql = "SELECT id, jp, zh, kanji, romaji, tone, score "
              "FROM jp_words "
              "WHERE source = ?1 AND (?2 = '' OR chapter = ?2) "
              "ORDER BY id";
    } else {
        sql = "SELECT id, en, zh, pos, phonetic, score "
              "FROM en_words "
              "WHERE source = ?1 AND (?2 = '' OR chapter = ?2) "
              "ORDER BY id";
    }

    if (!prepareStatement(db, sql, &stmt)) {
        sqlite3_close(db);
        return false;
    }

    sqlite3_bind_text(stmt, 1, source.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, chapter.c_str(), -1, SQLITE_TRANSIENT);

    while (true) {
        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            Word w;
            w.dbId = sqlite3_column_int(stmt, 0);
            w.jp = "";
            w.zh = "";
            w.kanji = "";
            w.romaji = "";
            w.en = "";
            w.pos = "";
            w.phonetic = "";
            w.tone = -1;
            int score = sqlite3_column_int(stmt, currentLanguage == LANG_JP ? 6 : 5);
            if (score < 1) score = 1;
            if (score > 5) score = 5;
            w.score = score;

            if (currentLanguage == LANG_JP) {
                w.jp = sqliteColumnText(stmt, 1);
                w.zh = sqliteColumnText(stmt, 2);
                w.kanji = sqliteColumnText(stmt, 3);
                w.romaji = sqliteColumnText(stmt, 4);
                w.tone = sqlite3_column_int(stmt, 5);
                if (!w.jp.isEmpty()) {
                    words.push_back(w);
                }
            } else {
                w.en = sqliteColumnText(stmt, 1);
                w.zh = sqliteColumnText(stmt, 2);
                w.pos = sqliteColumnText(stmt, 3);
                w.phonetic = sqliteColumnText(stmt, 4);
                if (!w.en.isEmpty()) {
                    words.push_back(w);
                }
            }
            continue;
        }

        if (rc != SQLITE_DONE) {
            Serial.printf("SQL 读取词库失败: %s\n", sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            words.clear();
            return false;
        }
        break;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return !words.empty();
}

/**
 * 将当前内存中的学习进度回写到数据库
 *
 * 这里只更新 `score`，不改动词条正文。更新依据是 `Word::dbId`，
 * 因此要求当前 `words` 必须来自数据库加载结果。
 * 为减少写盘次数，整个更新过程放在一个事务中执行。
 *
 * @return true 保存成功；false 任一记录更新失败
 */
bool saveCurrentWordsToDB()
{
    if (words.empty()) {
        return true;
    }

    sqlite3 *db = nullptr;
    sqlite3_stmt *stmt = nullptr;
    if (!openVocabularyDb(&db)) {
        return false;
    }

    String sql = String("UPDATE ") + currentWordTable() + " SET score = ?1 WHERE id = ?2";
    if (!prepareStatement(db, sql, &stmt)) {
        sqlite3_close(db);
        return false;
    }

    if (sqlite3_exec(db, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr) != SQLITE_OK) {
        Serial.printf("开启事务失败: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return false;
    }

    bool ok = true;
    for (auto &w : words) {
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        sqlite3_bind_int(stmt, 1, w.score);
        sqlite3_bind_int(stmt, 2, w.dbId);
        int rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            Serial.printf("写入 score 失败: %s\n", sqlite3_errmsg(db));
            ok = false;
            break;
        }
    }

    if (ok) {
        sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
    } else {
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return ok;
}

/**
 * 将一个词条列表写入数据库指定 source / chapter
 *
 * 主要用于错题导出等“生成新词库”的场景：
 * - 日语写入 `jp_words`
 * - 英语写入 `en_words`
 *
 * 若同一 `(source, chapter, jp/en)` 已存在，则执行 upsert，
 * 用新值覆盖原有正文和 score。
 *
 * @param source  目标来源名
 * @param chapter 目标章节名
 * @param list    要写入的词条列表
 * @return true 写入成功；false 事务或 SQL 执行失败
 */
bool saveWordListToDB(const String &source, const String &chapter, const std::vector<Word> &list)
{
    sqlite3 *db = nullptr;
    sqlite3_stmt *stmt = nullptr;
    if (!openVocabularyDb(&db)) {
        return false;
    }

    String sql;
    if (currentLanguage == LANG_JP) {
        sql = "INSERT INTO jp_words (jp, zh, kanji, romaji, tone, score, source, chapter) "
              "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8) "
              "ON CONFLICT(source, chapter, jp) DO UPDATE SET "
              "zh = excluded.zh, "
              "kanji = excluded.kanji, "
              "romaji = excluded.romaji, "
              "tone = excluded.tone, "
              "score = excluded.score";
    } else {
        sql = "INSERT INTO en_words (en, zh, pos, phonetic, score, source, chapter) "
              "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7) "
              "ON CONFLICT(source, chapter, en) DO UPDATE SET "
              "zh = excluded.zh, "
              "pos = excluded.pos, "
              "phonetic = excluded.phonetic, "
              "score = excluded.score";
    }

    if (!prepareStatement(db, sql, &stmt)) {
        sqlite3_close(db);
        return false;
    }

    if (sqlite3_exec(db, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr) != SQLITE_OK) {
        Serial.printf("开启事务失败: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return false;
    }

    bool ok = true;
    for (auto &w : list) {
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        if (currentLanguage == LANG_JP) {
            sqlite3_bind_text(stmt, 1, w.jp.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, w.zh.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, w.kanji.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 4, w.romaji.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 5, w.tone);
            sqlite3_bind_int(stmt, 6, w.score);
            sqlite3_bind_text(stmt, 7, source.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 8, chapter.c_str(), -1, SQLITE_TRANSIENT);
        } else {
            sqlite3_bind_text(stmt, 1, w.en.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, w.zh.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, w.pos.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 4, w.phonetic.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 5, w.score);
            sqlite3_bind_text(stmt, 6, source.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 7, chapter.c_str(), -1, SQLITE_TRANSIENT);
        }

        int rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            Serial.printf("插入词条失败: %s\n", sqlite3_errmsg(db));
            ok = false;
            break;
        }
    }

    if (ok) {
        sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
    } else {
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return ok;
}

/**
 * 加载当前语言下的所有 source 列表
 *
 * 结果按不区分大小写的字母序返回，用于文件选择模式的根层菜单。
 *
 * @param items 输出的 source 列表
 * @return true 查询成功；false 查询失败
 */
bool loadSourceList(std::vector<String> &items)
{
    String sql = String("SELECT DISTINCT source FROM ") + currentWordTable() +
                 " ORDER BY source COLLATE NOCASE";
    return loadSingleColumnList(sql, nullptr, items);
}

/**
 * 加载指定 source 下的 chapter 列表
 *
 * 仅返回非空 chapter，结果按不区分大小写的字母序排序。
 * 对于没有章节拆分的 source，该列表会为空。
 *
 * @param source 目标来源名
 * @param items  输出的 chapter 列表
 * @return true 查询成功；false 查询失败
 */
bool loadChapterList(const String &source, std::vector<String> &items)
{
    String sql = String("SELECT DISTINCT chapter FROM ") + currentWordTable() +
                 " WHERE source = ?1 AND chapter <> '' ORDER BY chapter COLLATE NOCASE";
    return loadSingleColumnList(sql, &source, items);
}

/**
 * 判断某个 source 是否存在 chapter 结构
 *
 * 用于文件选择模式中决定：
 * - 直接加载整个 source
 * - 还是先进入 chapter 子层
 *
 * @param source 目标来源名
 * @return true 至少存在一个非空 chapter；false 不存在或查询失败
 */
bool sourceHasChapters(const String &source)
{
    sqlite3 *db = nullptr;
    sqlite3_stmt *stmt = nullptr;
    if (!openVocabularyDb(&db)) {
        return false;
    }

    String sql = String("SELECT 1 FROM ") + currentWordTable() +
                 " WHERE source = ?1 AND chapter <> '' LIMIT 1";
    if (!prepareStatement(db, sql, &stmt)) {
        sqlite3_close(db);
        return false;
    }

    sqlite3_bind_text(stmt, 1, source.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    bool hasChapter = (rc == SQLITE_ROW);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return hasChapter;
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
 * 获取当前单词用于语音播放的文本
 *
 * 根据当前语言设置返回对应的音频文本。
 * 英语模式返回英文单词，日语模式返回日语假名。
 *
 * @param w 单词对象
 * @return 用于语音播放的文本字符串
 */
String listenAudioText(const Word &w)
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
    if (!path.startsWith("/")) {
        return path;
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

/**
 * 将听写错误单词导出为新的数据库词库
 *
 * 从 dictErrors 中提取所有答错的单词，并写入当前语言数据库中的：
 * - source = "Mistake"
 * - chapter = "<当前词库标签>(时间戳)"
 *
 * 这样可以继续沿用现有“错题本”概念，同时不再依赖 JSON 文件导出。
 */
void saveDictationMistakesAsWordList() {
    if (dictErrors.empty()) return;

    String baseName = selectedFilePath;
    if (baseName.isEmpty()) {
        baseName = "Mistake";
    }
    baseName.replace("/", "_");

    String timeStr = getNtpTimeString();
    String chapter = baseName + "(" + timeStr + ")";

    std::vector<Word> mistakeList;
    mistakeList.reserve(dictErrors.size());
    for (auto &e : dictErrors) {
        mistakeList.push_back(words[e.wordIndex]);
    }

    if (saveWordListToDB("Mistake", chapter, mistakeList)) {
        Serial.printf("错词已经另存到数据库: Mistake/%s\n", chapter.c_str());
    } else {
        Serial.println("错词保存到数据库失败");
    }
}
