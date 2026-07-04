/**
 * UtilsDb.ino - SQLite 词库访问与导入导出工具
 *
 * 负责和 SD 卡上的 SQLite 词库直接交互，包括：
 * - 数据库路径/表名解析
 * - SQL 语句 prepare 与查询辅助
 * - 词库 source / chapter 浏览
 * - 从数据库加载/保存词条
 * - Web 词库路径解析
 * - JSON <-> SQLite 导入辅助
 */

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

/**
 * 将 score 限制到 1~5
 *
 * 词库导入、数据库读取、Web 导出都会经过这里做统一收口，
 * 避免不同入口对 score 的裁剪规则不一致。
 *
 * @param score 原始分数
 * @return 归一化后的分数（1~5）
 */
int normalizeScoreValue(int score) {
    if (score < 1) return 1;
    if (score > 5) return 5;
    return score;
}

/**
 * 从文件名中提取不带扩展名的 stem
 *
 * 用于 Web 导入时把 JSON 文件名映射成 source / chapter。
 *
 * @param filename 原始文件名或路径
 * @return 去掉目录与扩展名后的名称
 */
String fileStem(const String &filename) {
    int slash = filename.lastIndexOf('/');
    String base = (slash >= 0) ? filename.substring(slash + 1) : filename;
    int dot = base.lastIndexOf('.');
    if (dot > 0) {
        base = base.substring(0, dot);
    }
    return base;
}

/**
 * 校验词库虚拟路径是否合法
 *
 * 运行时词库浏览已经不再直接暴露真实文件系统，只允许访问当前语言
 * 对应的 source / chapter 逻辑路径。
 *
 * @param path 待校验路径
 * @return true 路径合法；false 非法
 */
bool isValidVocabPath(const String &path) {
    if (path.indexOf("..") != -1) {
        return false;
    }
    return path == currentWordRoot || path.startsWith(currentWordRoot + "/");
}

/**
 * 解析词库虚拟路径，提取 source / chapter
 *
 * 支持三种层级：
 * - `currentWordRoot`：source 根层
 * - `currentWordRoot/<source>`：某个来源
 * - `currentWordRoot/<source>/<chapter>`：某个章节；`全部` 会被映射为空 chapter
 *
 * @param path     待解析路径；空字符串会退回 currentWordRoot
 * @param isRoot   输出是否处于 source 根层
 * @param source   输出 source
 * @param chapter  输出 chapter
 * @return true 解析成功；false 路径非法
 */
bool parseVocabPath(const String &path, bool &isRoot, String &source, String &chapter) {
    String target = path;
    if (target.isEmpty()) {
        target = currentWordRoot;
    }
    if (!isValidVocabPath(target)) {
        return false;
    }

    isRoot = false;
    source = "";
    chapter = "";

    if (target == currentWordRoot) {
        isRoot = true;
        return true;
    }

    String relative = target.substring(currentWordRoot.length());
    if (relative.startsWith("/")) {
        relative.remove(0, 1);
    }
    if (relative.isEmpty()) {
        isRoot = true;
        return true;
    }

    int slash = relative.indexOf('/');
    if (slash < 0) {
        source = relative;
        return true;
    }

    source = relative.substring(0, slash);
    chapter = relative.substring(slash + 1);
    if (chapter.indexOf('/') >= 0) {
        return false;
    }
    if (chapter == "全部") {
        chapter = "";
    }
    return !source.isEmpty();
}

/**
 * 根据当前路径推导上传导入目标
 *
 * - 在根层上传：文件名 stem 作为 source，chapter 为空
 * - 在 source 层上传：当前 source 固定，文件名 stem 作为 chapter
 *
 * @param path     当前 Web 浏览路径
 * @param filename 上传文件名
 * @param source   输出目标 source
 * @param chapter  输出目标 chapter
 * @return true 推导成功；false 当前层级不允许导入
 */
bool deriveUploadTarget(const String &path, const String &filename, String &source, String &chapter) {
    bool isRoot = false;
    if (!parseVocabPath(path, isRoot, source, chapter)) {
        return false;
    }

    String stem = fileStem(filename);
    if (stem.isEmpty()) {
        return false;
    }

    if (isRoot) {
        source = stem;
        chapter = "";
        return true;
    }

    if (chapter.isEmpty()) {
        chapter = stem;
        return true;
    }

    return false;
}

/**
 * 从 JSON 文件导入词条并写入数据库
 *
 * JSON 只作为导入导出交换格式；运行时词库管理全部基于 SQLite。
 *
 * @param jsonPath       临时 JSON 文件路径
 * @param source         导入目标 source
 * @param chapter        导入目标 chapter
 * @param importedCount  输出成功解析的词条数量
 * @param error          输出错误信息
 * @return true 导入成功；false 解析或写入失败
 */
bool importJsonFileToDb(const String &jsonPath, const String &source, const String &chapter, int &importedCount, String &error) {
    importedCount = 0;
    error = "";

    File f = SD.open(jsonPath);
    if (!f) {
        error = "无法读取上传文件";
        return false;
    }

    size_t capacity = (size_t)f.size() * 2 + 4096;
    if (capacity < 16384) {
        capacity = 16384;
    }

    DynamicJsonDocument doc(capacity);
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) {
        error = "JSON 解析失败";
        return false;
    }
    if (!doc.is<JsonArray>()) {
        error = "JSON 顶层必须是数组";
        return false;
    }

    JsonArray arr = doc.as<JsonArray>();
    std::vector<Word> list;
    list.reserve(arr.size());

    for (JsonVariant v : arr) {
        if (!v.is<JsonObject>()) {
            continue;
        }

        JsonObject obj = v.as<JsonObject>();
        Word w;
        w.dbId = 0;
        w.jp = "";
        w.zh = "";
        w.kanji = "";
        w.romaji = "";
        w.en = "";
        w.pos = "";
        w.phonetic = "";
        w.tone = -1;
        w.score = normalizeScoreValue(obj["score"] | 3);

        if (currentLanguage == LANG_JP) {
            w.jp = String(obj["jp"] | "");
            if (w.jp.isEmpty()) {
                continue;
            }
            w.zh = String(obj["zh"] | "");
            w.kanji = String(obj["kanji"] | "");
            w.romaji = String(obj["romaji"] | "");
            w.tone = obj["tone"] | -1;
        } else {
            w.en = String(obj["en"] | "");
            if (w.en.isEmpty()) {
                continue;
            }
            w.zh = String(obj["zh"] | "");
            w.pos = String(obj["pos"] | "");
            w.phonetic = String(obj["phonetic"] | "");
        }
        list.push_back(w);
    }

    importedCount = list.size();
    if (list.empty()) {
        error = "JSON 中没有可导入词条";
        return false;
    }
    if (!saveWordListToDB(source, chapter, list)) {
        error = "写入数据库失败";
        return false;
    }
    return true;
}

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
            w.score = normalizeScoreValue(sqlite3_column_int(stmt, currentLanguage == LANG_JP ? 6 : 5));

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
