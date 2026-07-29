# UtilsDb.ino

> 最后更新日期: 2026/07/29

## 作用

`UtilsDb.ino` 是项目的 **SQLite 数据库访问层**。负责词库数据的 CRUD 操作、数据库初始化、词库浏览和导入导出。它将原先分散在 `UtilsData.ino` 中的 JSON 文件操作统一替换为 SQLite 查询，解决了大词库加载慢、内存占用高的问题。

## 数据库结构

### 日语（`jp_words.db`）

```sql
CREATE TABLE IF NOT EXISTS jp_words (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    jp TEXT NOT NULL,
    zh TEXT NOT NULL,
    kanji TEXT DEFAULT '',
    romaji TEXT DEFAULT '',
    tone INTEGER DEFAULT 0,
    score INTEGER DEFAULT 3,
    sentence TEXT DEFAULT '',
    sentence_zh TEXT DEFAULT ''
);

CREATE TABLE IF NOT EXISTS jp_source (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    word_id INTEGER NOT NULL,
    source TEXT NOT NULL,
    chapter TEXT DEFAULT '',
    FOREIGN KEY (word_id) REFERENCES jp_words(id)
);

CREATE TABLE IF NOT EXISTS jp_errors (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    word_id INTEGER NOT NULL,
    wrong_text TEXT NOT NULL,
    created_at TEXT NOT NULL,
    FOREIGN KEY (word_id) REFERENCES jp_words(id) ON DELETE CASCADE
);
```

### 英语（`en_words.db`）

```sql
CREATE TABLE IF NOT EXISTS en_words (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    en TEXT NOT NULL,
    zh TEXT NOT NULL,
    pos TEXT DEFAULT '',
    phonetic TEXT DEFAULT '',
    score INTEGER DEFAULT 3,
    sentence TEXT DEFAULT '',
    sentence_zh TEXT DEFAULT ''
);

CREATE TABLE IF NOT EXISTS en_source (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    word_id INTEGER NOT NULL,
    source TEXT NOT NULL,
    chapter TEXT DEFAULT '',
    FOREIGN KEY (word_id) REFERENCES en_words(id)
);

CREATE TABLE IF NOT EXISTS en_errors (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    word_id INTEGER NOT NULL,
    wrong_text TEXT NOT NULL,
    created_at TEXT NOT NULL,
    FOREIGN KEY (word_id) REFERENCES en_words(id) ON DELETE CASCADE
);

-- 英语词根/词缀关联表
CREATE TABLE IF NOT EXISTS en_word_roots (
    word_id INTEGER NOT NULL,
    root_id INTEGER NOT NULL,
    PRIMARY KEY (word_id, root_id),
    FOREIGN KEY (word_id) REFERENCES en_words(id) ON DELETE CASCADE,
    FOREIGN KEY (root_id) REFERENCES en_roots(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS en_word_affixes (
    word_id INTEGER NOT NULL,
    affix_id INTEGER NOT NULL,
    PRIMARY KEY (word_id, affix_id),
    FOREIGN KEY (word_id) REFERENCES en_words(id) ON DELETE CASCADE,
    FOREIGN KEY (affix_id) REFERENCES en_affixes(id) ON DELETE CASCADE
);
```

**数据库文件位置：**

| 语言 | 数据库文件 |
|------|-----------|
| 日语 | `/words_study/jp/jp_words.db` |
| 英语 | `/words_study/en/en_words.db` |

> ⚠️ **注意**：数据库文件使用 `DELETE` journal 模式（非 WAL），这是由单片机的 SQLite 库兼容性决定的。

## 核心函数

### 表名映射

| 函数 | 返回值 |
|------|--------|
| `currentWordTable()` | `"jp_words"` 或 `"en_words"` |
| `currentSourceTable()` | `"jp_source"` 或 `"en_source"` |
| `currentDictationErrorTable()` | `"jp_errors"` 或 `"en_errors"` |

### 数据库操作

| 函数 | 作用 |
|------|------|
| `openVocabularyDb(&db)` | 打开当前语言的 SQLite 数据库 |
| `prepareStatement(db, sql, &stmt)` | 准备 SQL 语句，失败时打印错误日志 |
| `sqliteColumnText(stmt, col)` | 安全获取列的文本值 |
| `normalizeScoreValue(score)` | 将 score 钳位到 1~5 范围 |

### 词库读写

| 函数 | 作用 |
|------|------|
| `loadWordsBySource(source, chapter)` | 按 source/chapter 筛选加载词库到 `words` |
| `loadWordsByScore(score, groupIndex)` | 按熟练度分组加载词库（每批最多 50 条） |
| `loadScoreCounts(counts)` | 一次性查询 Score 1~5 各级单词数 |
| `loadWordsByIds(ids)` | 按 ID 列表加载词条到 `words` |
| `saveCurrentWordsToDB()` | 将当前 `words` 的 score 批量回写到数据库 |
| `saveWordListToDB(source, chapter, list)` | 将词库列表导入数据库（upsert） |
| `importJsonFileToDb(jsonPath, source, chapter, &count, &error)` | 从 JSON 文件导入到数据库 |

### 词库浏览

| 函数 | 作用 |
|------|------|
| `loadSourceList(items)` | 列出当前语言下的所有 source |
| `loadChapterList(source, items)` | 列出指定 source 下的所有 chapter |
| `sourceHasChapters(source)` | 判断 source 是否包含 chapter 子划分 |

### 路径解析

| 函数 | 作用 |
|------|------|
| `parseVocabPath(path, &isRoot, &source, &chapter)` | 将虚拟路径解析为 source/chapter |
| `isValidVocabPath(path)` | 校验虚拟路径是否合法 |
| `deriveUploadTarget(path, filename, &source, &chapter)` | 从上传请求推导 source 和 chapter |

### 错题管理

| 函数 | 作用 |
|------|------|
| `saveDictationErrorsToDB(errors)` | 将听写错误批量写入数据库 |
| `loadDictationReviewEntriesFromDB(items)` | 从数据库加载历史错题回顾列表 |
| `deleteDictationError(errorId)` | 删除指定 ID 的错题记录 |

### 词根/词缀查询

| 函数 | 作用 |
|------|------|
| `loadRootAffixNames(idList, table, nameCol, &out)` | 根据逗号分隔的 ID 列表查询词根/词缀的名称与释义 |

## 关键流程

### 词库加载流程

```mermaid
flowchart TD
    A[loadWordsBySource] --> B[openVocabularyDb]
    B --> C{source 有 chapter?}
    C -->|是| D[SELECT 带 source + chapter 过滤]
    C -->|否| E[SELECT 带 source 过滤]
    D & E --> F[遍历结果集]
    F --> G[构造 Word 对象<br/>（不含 root/affix）]
    G --> H[populateRootAffixFromJunction<br/>批量填充 root/affix ID]
    H --> I[关闭数据库]
    I --> J[返回 words]
```

> `root` 和 `affix` 不再直接存储在 `en_words` 表中，而是通过关联表 `en_word_roots` 和 `en_word_affixes` 存储。加载时通过 `populateRootAffixFromJunction()` 一次批量查询填充到 `Word` 结构体中。

### JSON 导入流程

```mermaid
flowchart TD
    A[importJsonFileToDb] --> B[SD.open JSON 文件]
    B --> C[deserializeJson]
    C --> D[遍历 JSON 数组]
    D --> E[构建 Word 列表]
    E --> F[saveWordListToDB]
    F --> G[UPSERT 写入每个单词]
    G --> H[返回导入计数]
```

### 自动保存流程

```mermaid
flowchart LR
    A[saveCurrentWordsToDB] --> B[遍历 words]
    B --> C[UPDATE score WHERE id = ?]
    C --> D[标记完成]
```

### 错题回顾加载流程

```mermaid
flowchart TD
    A[loadDictationReviewEntriesFromDB] --> B[SELECT e.id, e.word_id,<br/>e.wrong_text, e.created_at,<br/>w.jp/en AS correct]
    B --> C[构造 DictationReviewEntry]
    C --> D[填写 errorId / wordDbId / wrong / createdAt / correct]
    D --> E[items 列表]
```

> 查询结果按 `rowid DESC` 倒序，最新记录在前。`errorId` 对应错题表的 `id` 列，用于后续的 `deleteDictationError()` 调用。

## 重要细节

### Word ID 与 dbId

每个 `Word` 结构体包含 `dbId` 字段，记录该单词在数据库中的主键。`loadWordsBySource()` 等加载函数会填充此字段，`saveCurrentWordsToDB()` 通过 `dbId` 进行 UPDATE 操作。

### 词根/词缀关联表设计

英语词根和词缀不再直接存储在 `en_words` 表的 `root`/`affix` 列中，改为通过 `en_word_roots` 和 `en_word_affixes` 两张关联表以多对多关系存储。加载流程分为两步：

1. 主查询 SELECT 除 `root`/`affix` 之外的所有字段到 `Word` 结构体
2. `populateRootAffixFromJunction()` 通过 `word_id IN (...)` 批量查询关联表，将词根/词缀 ID 重新拼接为逗号字符串填入 `Word.root` / `Word.affix`

上层展示代码（如学习模式中的词根词缀显示）通过 `loadRootAffixNames()` 将逗号分隔的 ID 列表解析为具体的名称和释义文本。

### UPSERT 策略

`saveWordListToDB()` 使用两步操作实现导入：
1. 先查询是否已存在相同 `(jp/en, source, chapter)` 的单词
2. 若存在则更新，不存在则插入
3. 同时维护 `*_source` 关联表

### score 规范化

- `normalizeScoreValue()`：score < 1 → 1，score > 5 → 5。
- 加载时自动应用，写回时也会校验。

### 数据库 journal 模式

- 数据库文件使用 `DELETE` journal 模式（非 WAL），单片机的 SQLite 库无法读取 WAL 模式的文件。
- `saveWordListToDB()` 在导入时使用事务，确保数据一致性。

## 使用示例

### 加载词库

```cpp
std::vector<Word> words;
loadWordsBySource("Demo_Basics", "");        // 加载整个 source
loadWordsBySource("Lesson", "Unit_1");       // 加载 source 的特定 chapter
```

### 按 ID 列表加载词条

```cpp
std::vector<int> ids = {101, 205, 307};
loadWordsByIds(ids);  // 按 ID 批量加载到 words
```

### 导入 JSON

```cpp
int count = 0;
String error;
if (importJsonFileToDb("/words_study/jp/word/N5/vocab.json", "N5", "", count, error)) {
    Serial.printf("导入了 %d 个单词\n", count);
} else {
    Serial.printf("导入失败: %s\n", error.c_str());
}
```

### 浏览词库

```cpp
std::vector<String> sources;
loadSourceList(sources);  // 获取所有 source 名称
for (auto &s : sources) {
    if (sourceHasChapters(s)) {
        std::vector<String> chapters;
        loadChapterList(s, chapters);
    }
}
```

### 删除错题

```cpp
deleteDictationError(errorId);  // 从数据库中删除指定错题记录
```

## 注意事项

- 数据库文件使用 `DELETE` journal 模式，不可切换为 WAL（单片机 SQLite 库限制）。
- `saveWordListToDB()` 在导入时使用事务，确保数据一致性。
- `openVocabularyDb()` 失败时会打印 Serial 错误日志并返回 `false`。
- 虚拟路径格式为 `/words_study/<lang>/word`，内部通过 `parseVocabPath()` 解析为 source/chapter。
- `deleteDictationError()` 只删除错题记录，不影响原词条。
