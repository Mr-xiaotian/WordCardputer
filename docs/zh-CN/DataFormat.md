# 数据格式规范

> 最后更新日期: 2026/07/11

## 作用

本文档定义 WordCardputer 项目使用的 **SD 卡数据存储结构与数据格式规范**。项目已从 JSON 文件存储迁移至 SQLite 数据库，词库数据不再以独立 JSON 文件形式存在于 SD 卡上。

## 目录结构

SD 卡根目录必须包含 `words_study` 文件夹：

```
SD 卡根目录/
└── words_study/
    ├── config.json             # 设备配置文件（替代旧版 wifi.json）
    ├── .env                    # API Key 等敏感配置（PC 端工具使用）
    ├── jp/
    │   ├── jp_words.db         # 日语词库 SQLite 数据库
    │   └── audio/              # 日语发音 WAV
    │       ├── あめ.wav
    │       └── いぬ.wav
    ├── en/
    │   ├── en_words.db         # 英语词库 SQLite 数据库
    │   └── audio/              # 英语发音 WAV
    │       ├── apple.wav
    │       └── run.wav
    └── www/
        └── index.html          # Web 控制面板前端
```

> ⚠️ **已变更**：词库数据不再使用 `words_study/<lang>/word/` 目录下的独立 JSON 文件。旧版 JSON 文件可通过 Web 面板的导入功能迁移到数据库。

## 数据库结构

### 日语（`jp_words.db`）

#### jp_words 表

```sql
CREATE TABLE IF NOT EXISTS jp_words (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    jp TEXT NOT NULL,          -- 日语假名
    zh TEXT NOT NULL,          -- 中文释义
    kanji TEXT DEFAULT '',     -- 日语汉字写法
    romaji TEXT DEFAULT '',    -- 罗马音标注
    tone INTEGER DEFAULT 0,   -- 声调编号，-1 表示无/未知
    score INTEGER DEFAULT 3   -- 熟练度 1~5
);
```

#### jp_source 表

```sql
CREATE TABLE IF NOT EXISTS jp_source (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    word_id INTEGER NOT NULL,
    source TEXT NOT NULL,      -- 词库来源（如 "Demo_Basics"、"N5"）
    chapter TEXT DEFAULT '',   -- 章节（如 "Unit_1"），空表示无章节
    FOREIGN KEY (word_id) REFERENCES jp_words(id)
);
```

#### jp_dictation_errors 表

```sql
CREATE TABLE IF NOT EXISTS jp_dictation_errors (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    word_id INTEGER NOT NULL,
    wrong TEXT NOT NULL,       -- 用户错误输入
    created_at TEXT NOT NULL,  -- 错误发生时间
    FOREIGN KEY (word_id) REFERENCES jp_words(id)
);
```

### 英语（`en_words.db`）

#### en_words 表

```sql
CREATE TABLE IF NOT EXISTS en_words (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    en TEXT NOT NULL,          -- 英语单词或短语
    zh TEXT NOT NULL,          -- 中文释义
    pos TEXT DEFAULT '',       -- 词性（noun/verb/adj 等）
    phonetic TEXT DEFAULT '',  -- IPA 音标
    score INTEGER DEFAULT 3   -- 熟练度 1~5
);
```

#### en_source 表

```sql
CREATE TABLE IF NOT EXISTS en_source (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    word_id INTEGER NOT NULL,
    source TEXT NOT NULL,
    chapter TEXT DEFAULT '',
    FOREIGN KEY (word_id) REFERENCES en_words(id)
);
```

#### en_dictation_errors 表

与 `jp_dictation_errors` 结构相同。

### 字段汇总

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `jp` / `en` | TEXT | 是 | 日语假名或英语单词 |
| `zh` | TEXT | 是 | 中文释义 |
| `kanji` | TEXT | 否 | 日语汉字写法 |
| `romaji` | TEXT | 否 | 罗马音标注 |
| `tone` | INTEGER | 否 | 声调编号，默认 0 |
| `pos` | TEXT | 否 | 词性 |
| `phonetic` | TEXT | 否 | IPA 音标 |
| `score` | INTEGER | 是 | 熟练度 1~5，默认 3 |
| `source` | TEXT | 是 | 词库来源标识 |
| `chapter` | TEXT | 否 | 章节标识 |

## 配置文件（config.json）

统一配置文件替代了旧版 `wifi.json`，结构如下：

```json
{
  "version": 1,
  "settings": {
    "volume": 192,
    "language": "jp",
    "brightness": 200,
    "dim_brightness": 40,
    "idle_timeout_ms": 60000,
    "auto_save_threshold": 5
  },
  "wifi": [
    { "ssid": "MyHome", "pass": "password123" }
  ]
}
```

详见 [UtilsConfig.md](UtilsConfig.md)。

## 其他文件

### `.env`

PC 端 Python 工具使用，包含 API Key 等敏感信息。示例：

```bash
API_KEY=your_minimax_api_key_here
```

该文件不应提交到版本控制。

## 数据流

```mermaid
flowchart LR
    A[SQLite 数据库] -->|loadWordsFromDB| B[内存 words 数组]
    B --> C[学习/听写/听读]
    C --> D[score 变更]
    D -->|markScoreDirty| E[累计 autoSaveThreshold 次]
    E -->|saveCurrentWordsToDB| F[UPDATE 写回数据库]
    G[Web 面板上传 JSON] -->|importJsonFileToDb| A
```

## JSON 兼容性

> ⚠️ **兼容说明**：旧版 JSON 词库格式仍受支持，可通过 Web 控制面板的导入功能转换为数据库格式。JSON 格式与旧版相同——顶层必须是数组，包含对象元素，字段与上述数据库字段一一对应。

### 日语 JSON 示例（导入用）

```json
[
  {
    "jp": "わたし",
    "zh": "我",
    "kanji": "私",
    "tone": 0,
    "score": 3,
    "romaji": "watashi"
  }
]
```

### 英语 JSON 示例（导入用）

```json
[
  {
    "en": "run",
    "zh": "跑；运行",
    "pos": "verb",
    "phonetic": "/rʌn/",
    "score": 3
  }
]
```

## 音频文件要求

| 参数 | 要求 |
|------|------|
| 格式 | WAV |
| 编码 | PCM（线性脉冲编码调制） |
| 位深 | 8-bit 或 16-bit |
| 声道 | 单声道或立体声 |
| 采样率 | ≤ 48 kHz，推荐 16 kHz |

### 文件命名

音频文件名必须与词库中的主键字段完全一致：

- 日语：`jp` 字段内容 + `.wav`
- 英语：`en` 字段内容 + `.wav`

示例：

| 词库字段 | 音频文件名 |
|----------|-----------|
| `"jp": "あめ"` | `あめ.wav` |
| `"en": "apple"` | `apple.wav` |

> 注意：文件名区分大小写，英语短语中的空格需与词库字段保持一致。

## 注意事项

- 数据库文件通过 SQLite 的 WAL 模式访问，支持并发读写。
- `score` 字段在加载和保存时自动钳位到 1~5 范围。
- 音频文件缺失时，设备会播放 880Hz 提示音代替，不影响程序运行。
- 听写错题保存在数据库的 `*_dictation_errors` 表中，不再导出为单独 JSON 文件。
- 通过 Web 面板可随时将数据库中任意 source/chapter 导出为 JSON 文件备份。
