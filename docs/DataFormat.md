# 数据格式规范

> 最后更新日期: 2026/06/22

## 作用

本文档定义 WordCardputer 项目使用的 **SD 卡数据目录结构、词库 JSON 字段规范、音频文件命名规则以及环境变量说明**。所有数据文件均存储在 SD 卡上，设备通过 SPI 挂载并读取。

## 目录结构

SD 卡根目录必须包含 `words_study` 文件夹：

```
SD 卡根目录/
└── words_study/
    ├── wifi.json              # WiFi 凭据（连接成功后自动生成）
    ├── .env                   # API Key 等敏感配置（PC 端工具使用）
    ├── jp/
    │   ├── word/              # 日语词库 JSON
    │   │   ├── Demo_Basics.json
    │   │   ├── N5/
    │   │   ├── Lesson/
    │   │   └── Mistake/       # 听写错题导出目录
    │   └── audio/             # 日语发音 WAV
    │       ├── あめ.wav
    │       └── いぬ.wav
    ├── en/
    │   ├── word/              # 英语词库 JSON
    │   │   ├── Demo_Basics.json
    │   │   ├── Demo_Travel.json
    │   │   └── Mistake/
    │   └── audio/             # 英语发音 WAV
    │       ├── apple.wav
    │       └── run.wav
    └── www/
        └── index.html         # Web 控制面板前端
```

## 词库 JSON 字段

### 日语词库

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

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `jp` | string | 是 | 日语假名（ hiragana/katakana 混合均可） |
| `zh` | string | 是 | 中文释义 |
| `kanji` | string | 否 | 日语汉字写法 |
| `tone` | int | 否 | 声调编号，`-1` 表示无/未知 |
| `score` | int | 是 | 熟练度 1~5，默认 3 |
| `romaji` | string | 否 | 罗马音标注，PC 端工具使用 |

### 英语词库

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

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `en` | string | 是 | 英语单词或短语 |
| `zh` | string | 是 | 中文释义 |
| `pos` | string | 否 | 词性，如 `noun`、`verb`、`adj` |
| `phonetic` | string | 否 | IPA 音标，设备端会转换为 ASCII 近似显示 |
| `score` | int | 是 | 熟练度 1~5，默认 3 |

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

## 其他文件

### `wifi.json`

由设备自动生成，PC 端无需手动创建。格式为对象数组：

```json
[
  { "ssid": "MyHome", "pass": "password123" }
]
```

### `.env`

PC 端 Python 工具使用，包含 API Key 等敏感信息。示例：

```bash
API_KEY=your_minimax_api_key_here
```

该文件不应提交到版本控制。

## 数据流

```mermaid
flowchart LR
    A[SD 卡 JSON] -->|loadWordsFromJSON| B[内存 words 数组]
    B --> C[学习/听写/听读]
    C --> D[score 变更]
    D -->|markScoreDirty| E[累计 5 次]
    E -->|autoSaveIfNeeded| F[写回 SD 卡]
```

## 注意事项

- 词库 JSON 顶层必须是数组，单个对象会被 `ArduinoJson` 解析失败。
- `score` 字段超出 1~5 范围时，设备端会自动钳位到该范围。
- 音频文件缺失时，设备会播放 880Hz 提示音代替，不影响程序运行。
- 建议定期将 `Mistake` 目录中的错题本合并回原词库，进行针对性复习。
