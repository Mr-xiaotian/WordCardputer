# Subagent Prompt: 核心与跨领域文档

## 区域说明

负责核心文件及跨领域主题文档的同步：

| 文件 | 对应源码 | 类型 |
|------|---------|------|
| `docs/zh-CN/WordCardputer.md` | `src/main.cpp` + `src/globals.cpp/.h` | 主程序文档 |
| `docs/zh-CN/DataFormat.md` | 全项目数据库 schema | 主题文档 |
| `docs/zh-CN/WebAPI.md` | `src/UtilsWebServer.cpp/.h` | 主题文档 |
| `docs/zh-CN/PythonTools.md` | `utils/*.py` | 工具文档 |
| `docs/zh-CN/README.md` | — | 索引文档 |

## 审计重点

### WordCardputer.md — 全局快照

这篇文档是所有模式的入口汇总，每次审计必须更新以下内容：

1. **`AppMode` 枚举完整列表**：从 `src/globals.h` 枚举中逐项提取，保持声明顺序。
2. **`Word` 结构体全部字段**：包括 `dbId`, `jp`, `zh`, `kanji`, `romaji`, `en`, `pos`, `phonetic`, `sentence`, `sentenceZh`, `root`, `affix`, `tone`, `score`。
3. **`DictationReviewEntry`**：含 `errorId` 字段（用于数据库删除操作）。
4. **启动流程**：`setup()` 的流程——`SD卡 → SQLite → 检查数据库 → loadAppConfig → 创建画布 → MODE_SPLASH`。
5. **主循环分发**：`loop()` 中 `if/else if` 链覆盖所有 `AppMode` 值。
6. **全局变量**：根据 `src/globals.cpp` 中的初始值同步（如 `autoSaveThreshold = 5`）。
7. **SD 卡引脚**：SCK=40, MISO=39, MOSI=14, CS=12, SPI 频率 25MHz。

### DataFormat.md — 数据库 schema

数据格式文档涵盖 SQLite 数据库结构、JSON 兼容格式和配置文件。审计要点：

1. **数据库 journal 模式**：必须写 **DELETE**（不是 WAL！）。
2. **表结构**：C++ 源码中没有 `CREATE TABLE` 语句（数据库外部预创建），需要从 SQL 查询中推断表结构。无法确认的约束（如 `NOT NULL`、`DEFAULT`）标 `⚠️ 待确认`。
3. **关联表**：`en_word_roots`、`en_word_affixes` 是新增的关联表。
4. **听写错误表字段**：注意列名是 `wrong_text`（源码中同时出现 `wrong_text` 和 `wrong`，以 CREATE 语句中的 `wrong_text` 为准）。

推断表结构时应检查的函数：
- `loadWordsBySource()` — en_words 查询
- `loadWordsByScore()` — en_words 查询
- `populateRootAffixFromJunction()` — 关联表查询
- `saveDictationErrorsToDB()` — 错题表 INSERT
- `loadDictationReviewEntriesFromDB()` — 错题表 SELECT

### WebAPI.md — HTTP API

审计 `src/UtilsWebServer.cpp` 中所有路由：

1. 路由注册（`server.on(...)` 调用）
2. 请求/响应参数和字段
3. 特别注意响应 JSON 中的字段名是否与代码一致

### PythonTools.md — Python 工具

审计 `utils/` 下所有 `.py` 文件：

1. `utils/tts.py` — TTS 生成函数
2. `utils/audio.py` — 音频处理函数
3. `utils/json_utils.py` — JSON 词库操作函数（函数较多，容易遗漏新增的英语专用函数如 `collect_merged_entries_en`）
4. `utils/stats.py` — 统计分析函数

### README.md — 文档索引

最后更新，汇总所有文档变更后刷新目录：
- 新增文档条目
- 修改的源码引用
- 最近更新重点（本次同步的日期和摘要）