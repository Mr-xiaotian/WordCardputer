# Python 工具链

> 最后更新日期: 2026/09/01

## 作用

`utils/` 目录下的 Python 脚本用于在 PC 端 **生成词库音频、处理 WAV 文件、合并/去重/拆分 JSON 词库、分析掌握程度**。这些工具不运行在设备上，而是辅助用户准备 SD 卡数据。

包入口 `utils/__init__.py` 统一 re-export 了所有公共函数，因此推荐按 `from utils.xxx import ...` 方式引用，而不是直接 import 子模块。

## 环境准备

项目使用 `uv` 管理 Python 依赖：

```bash
uv sync
```

确保 `ffmpeg` 已安装并加入 PATH（`generate_tts_youdao` 需要）。

## 模块说明

### `utils/tts.py` — TTS 语音生成

| 函数 | 作用 |
|------|------|
| `generate_tts_minimax(text, output_path, ...)` | 调用 MiniMax T2A v2 接口（`https://api.minimax.io/v1/t2a_v2`）生成 WAV。默认模型 `speech-2.6-turbo`、默认声音 `Japanese_DecisivePrincess`、默认采样率 32000、比特率 128000、声道 1、情感 `calm` |
| `generate_tts_youdao(text, output_path)` | 抓取有道词典发音（`https://dict.youdao.com/dictvoice?audio=...&type=2`），通过 ffmpeg 转 16 kHz / 单声道 / `pcm_s16le` WAV |
| `fill_missing_audio(db_path, audio_dir, delay)` | 读取英语 `en_words` 表，为缺少音频的单词批量生成有道 TTS 音频，串行带 `delay` 秒间隔避免被限流，返回 `(成功数, 失败数)` |

#### MiniMax 示例

```python
from utils.tts import generate_tts_minimax
from pathlib import Path

ok, result = generate_tts_minimax(
    text="こんにちは",
    output_path=Path("words_study/jp/audio/こんにちは.wav"),
    voice_id="Japanese_DecisivePrincess",
    language="Japanese",
)
print(result)
```

#### 有道示例

```python
from utils.tts import generate_tts_youdao
from pathlib import Path

ok, result = generate_tts_youdao(
    text="apple",
    output_path=Path("words_study/en/audio/apple.wav"),
)
print(result)
```

#### 批量补全缺失音频

```python
from utils.tts import fill_missing_audio

success, fail = fill_missing_audio(
    db_path="words_study/en/en_words.db",
    audio_dir="words_study/en/audio",
    delay=0.5,
)
print(f"成功: {success}, 失败: {fail}")
```

> 需要在 `.env` 中配置 `API_KEY` 才能使用 MiniMax。

---

### `utils/audio.py` — 音频处理

| 函数 | 作用 |
|------|------|
| `trim_leading_silence_wav(...)` | 裁剪单个 WAV 文件的前导静音 |
| `trim_leading_silence_in_folder(...)` | 批量裁剪文件夹内 WAV |

#### 批量裁剪示例

```python
from utils.audio import trim_leading_silence_in_folder
from pathlib import Path

summary = trim_leading_silence_in_folder(
    folder_path=Path("words_study/jp/audio"),
    recursive=True,
    threshold_ratio=0.015,
    keep_ms=40,
    max_trim_ms=800,
)
print(summary)
```

输出示例：

```python
{
    "folder": "words_study/jp/audio",
    "total": 120,
    "success": 118,
    "failed": 2,
    "errors": [
        {"file": "...", "error": "未检测到有效发音段"}
    ]
}
```

---

### `utils/json_utils.py` — 词库 JSON 操作

> 所有函数均以 JSON **文件**（不是 SQLite）为操作目标，列表形式顶层。

| 函数 | 作用 |
|------|------|
| `load_json_list(path)` | 读取 JSON 并校验顶层为列表 |
| `extract_field_values(path, field)` | 提取指定字段 |
| `extract_jp_fields(path)` / `extract_en_fields(path)` | 提取 `jp` / `en` 字段 |
| `extract_all_values_from_folder(folder, field)` | 遍历文件夹（`rglob`）提取指定字段，去重并排序返回 |
| `extract_all_jp_from_folder(folder)` / `extract_all_en_from_folder(folder)` | 遍历文件夹提取并去重 |
| `list_wav_filenames(folder)` | 列出 `*.wav` 文件名（不含扩展名） |
| `collect_merged_entries_by_key(folder, key_field, score_field, tone_field)` | 通用合并：按指定 key 字段聚合 |
| `collect_merged_entries(folder)` | 合并多个日本语 JSON（键为 `jp`，启用 `tone` 冲突处理） |
| `collect_merged_entries_en(folder)` | 合并多个英语 JSON（键为 `en`，不处理 `tone`） |
| `apply_merge_and_rewrite_by_key(folder, key_field, score_field, tone_field)` | 通用合并后写回每个 JSON |
| `apply_merge_and_rewrite(folder)` | 日语合并后写回（与上面同义） |
| `apply_merge_and_rewrite_en(folder)` | 英语合并后写回 |
| `filter_json_by_key_difference(a, b, key_field)` | 保留 a 中相对 b 的差集，并写回 a |
| `filter_json_by_jp_difference(a, b)` / `filter_json_by_en_difference(a, b)` | 按 `jp` / `en` 差集过滤 |
| `dedupe_json_by_key(folder, key_field)` | 按 key 字段去重（`glob` 不递归），写回 |
| `dedupe_json_by_jp(folder)` / `dedupe_json_by_en(folder)` | 按 `jp` / `en` 去重 |
| `split_json_file(path, max_per_file)` | 按数量拆分大词库（生成 `<stem>_part<N>.json`） |
| `process_folder(folder, max_per_file)` | 批量拆分文件夹内 JSON（仅顶层 `*.json`，不递归） |

#### 常用工作流示例

```python
from utils.json_utils import extract_all_jp_from_folder, list_wav_filenames
from pathlib import Path

# 检查缺失音频
words = set(extract_all_jp_from_folder(Path("words_study/jp/word")))
audios = set(list_wav_filenames(Path("words_study/jp/audio")))
missing = words - audios
print(f"缺失音频: {missing}")
```

#### 合并日语词库

```python
from utils.json_utils import collect_merged_entries
from pathlib import Path

entries = collect_merged_entries(Path("words_study/jp/word"))
print(f"合并后共 {len(entries)} 个唯一词条")
```

#### 合并英语词库

```python
from utils.json_utils import collect_merged_entries_en
from pathlib import Path

entries = collect_merged_entries_en(Path("words_study/en/word"))
print(f"合并后共 {len(entries)} 个唯一词条")
```

#### 拆分大词库

```python
from utils.json_utils import process_folder
from pathlib import Path

process_folder(Path("words_study/jp/word/N5"), max_per_file=60)
```

执行后会生成 `xxx_part1.json`、`xxx_part2.json` 等文件。

---

### `utils/stats.py` — 词库统计分析

| 函数 | 作用 |
|------|------|
| `analyze_vocab_mastery(json_path)` | 分析词库掌握程度 |

#### 示例

```python
from utils.stats import analyze_vocab_mastery
from pathlib import Path

result = analyze_vocab_mastery(Path("words_study/en/word/Demo_Basics.json"))
print(result)
```

输出示例：

```python
{
    "file": "words_study/en/word/Demo_Basics.json",
    "total_words": 4,
    "average_score": 3.25,
    "median_score": 3,
    "distribution": {
        1: {"count": 0, "ratio": 0.0},
        2: {"count": 1, "ratio": 0.25},
        3: {"count": 2, "ratio": 0.5},
        4: {"count": 1, "ratio": 0.25},
        5: {"count": 0, "ratio": 0.0},
    },
    "mastery_level": "掌握中",
}
```

## 批量生成音频工作流

```python
from pathlib import Path
from utils.json_utils import extract_all_jp_from_folder
from utils.tts import generate_tts_minimax

word_folder = Path("words_study/jp/word/N5")
audio_folder = Path("words_study/jp/audio")
audio_folder.mkdir(parents=True, exist_ok=True)

words = extract_all_jp_from_folder(word_folder)
for w in words:
    out = audio_folder / f"{w}.wav"
    if out.exists():
        continue
    ok, msg = generate_tts_minimax(w, out, voice_id="Japanese_DecisivePrincess")
    print(w, "OK" if ok else msg)
```

## 注意事项

- `.env` 文件包含 API Key，不应提交到版本控制。`generate_tts_minimax` 依赖 `API_KEY` 环境变量（由 `python-dotenv` 的 `load_dotenv()` 加载）。
- `generate_tts_youdao` 依赖 ffmpeg 将有道返回的 MP3 转为 16 kHz / 单声道 / `pcm_s16le` WAV（`ffmpeg -y -i in.mp3 -ar 16000 -ac 1 -c:a pcm_s16le out.wav`），请确保 ffmpeg 可用。
- `collect_merged_entries` 合并规则（`collect_merged_entries_by_key` 实现）：
  - `score` 字段取最大值（按 `max(old, val)` 合并）。
  - `tone` 字段冲突时按以下规则：仅当 old/val 都不为 -1 且不同才置为 `-1`；否则保留非 -1 的值。
  - 英语合并 (`collect_merged_entries_en`) 默认 `tone_field=None`，不参与合并规则。
  - 其他字段用 `; ` 拼接不同值（且不会重复）。
- `process_folder` 与 `dedupe_json_by_key` 只匹配顶层 `*.json`（`folder.glob`），而 `extract_all_values_from_folder` / `collect_merged_entries_by_key` 递归子目录（`folder.rglob`）。
- `fill_missing_audio` 通过 `SELECT en FROM en_words` 加载词表并按词去重比较 audio 目录中现有 `.wav` 的 stem（大小写不敏感）。它仅适用于英语词库（默认路径 `words_study/en/en_words.db`）。
- 批量生成音频前建议先用小批量测试，确认音色和语速符合预期。
- 词库数据在设备端已迁移至 SQLite 数据库，PC 端 JSON 工具主要用于生成音频和准备导入数据。导入操作可通过 Web 控制面板完成。
- `stats.py` 的 `analyze_vocab_mastery` 当前对每个 JSON 文件独立分析；score 字段缺失或非数值类型时按 3 处理。
