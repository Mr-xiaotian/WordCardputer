# Python 工具链

> 最后更新日期: 2026/06/22

## 作用

`util/` 目录下的 Python 脚本用于在 PC 端 **生成词库音频、处理 WAV 文件、合并/去重/拆分 JSON 词库、分析掌握程度**。这些工具不运行在设备上，而是辅助用户准备 SD 卡数据。

## 环境准备

在项目根目录创建并激活虚拟环境：

```bash
python -m venv .venv
.venv\Scripts\activate        # Windows
# source .venv/bin/activate   # Linux/macOS
pip install requests python-dotenv
```

确保 `ffmpeg` 已安装并加入 PATH（`generate_tts_youdao` 需要）。

## 模块说明

### `util/tts.py` — TTS 语音生成

| 函数 | 作用 |
|------|------|
| `generate_tts_minimax(text, output_path, ...)` | 调用 MiniMax T2A API 生成 WAV |
| `generate_tts_youdao(text, output_path)` | 抓取有道词典发音并转 WAV |

#### MiniMax 示例

```python
from util.tts import generate_tts_minimax
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
from util.tts import generate_tts_youdao
from pathlib import Path

ok, result = generate_tts_youdao(
    text="apple",
    output_path=Path("words_study/en/audio/apple.wav"),
)
print(result)
```

> 需要在 `.env` 中配置 `API_KEY` 才能使用 MiniMax。

---

### `util/audio.py` — 音频处理

| 函数 | 作用 |
|------|------|
| `trim_leading_silence_wav(...)` | 裁剪单个 WAV 文件的前导静音 |
| `trim_leading_silence_in_folder(...)` | 批量裁剪文件夹内 WAV |

#### 批量裁剪示例

```python
from util.audio import trim_leading_silence_in_folder
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

### `util/json_utils.py` — 词库 JSON 操作

| 函数 | 作用 |
|------|------|
| `load_json_list(path)` | 读取 JSON 并校验顶层为列表 |
| `extract_field_values(path, field)` | 提取指定字段 |
| `extract_jp_fields(path)` / `extract_en_fields(path)` | 提取 `jp` / `en` 字段 |
| `extract_all_*_from_folder(folder)` | 遍历文件夹提取并去重 |
| `list_wav_filenames(folder)` | 列出 WAV 文件名（不含扩展名） |
| `collect_merged_entries(folder)` | 合并多个 JSON 为一个大词典 |
| `apply_merge_and_rewrite(folder)` | 合并后写回每个 JSON |
| `filter_json_by_jp_difference(a, b)` | 保留 a 中相对 b 的差集 |
| `dedupe_json_by_jp(folder)` | 按 `jp` 去重 |
| `split_json_file(path, max_per_file)` | 按数量拆分大词库 |
| `process_folder(folder, max_per_file)` | 批量拆分文件夹内 JSON |

#### 合并多个词库

```python
from util.json_utils import collect_merged_entries
from pathlib import Path

entries = collect_merged_entries(Path("words_study/jp/word"))
print(f"合并后共 {len(entries)} 个唯一词条")
```

#### 拆分大词库

```python
from util.json_utils import process_folder
from pathlib import Path

process_folder(Path("words_study/jp/word/N5"), max_per_file=60)
```

执行后会生成 `xxx_part1.json`、`xxx_part2.json` 等文件。

#### 检查缺失音频

```python
from util.json_utils import extract_all_jp_from_folder, list_wav_filenames
from pathlib import Path

words = set(extract_all_jp_from_folder(Path("words_study/jp/word")))
audios = set(list_wav_filenames(Path("words_study/jp/audio")))
missing = words - audios
print(f"缺失音频: {missing}")
```

---

### `util/stats.py` — 词库统计分析

| 函数 | 作用 |
|------|------|
| `analyze_vocab_mastery(json_path)` | 分析词库掌握程度 |

#### 示例

```python
from util.stats import analyze_vocab_mastery
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
from util.json_utils import extract_all_jp_from_folder
from util.tts import generate_tts_minimax

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

- `.env` 文件包含 API Key，不应提交到版本控制。
- `generate_tts_youdao` 依赖 ffmpeg 将 MP3 转为 WAV，请确保 ffmpeg 可用。
- `collect_merged_entries` 合并规则：
  - `score` 字段取最大值。
  - `tone` 字段冲突时置为 `-1`。
  - 其他字段用 `; ` 拼接不同值。
- 批量生成音频前建议先用小批量测试，确认音色和语速符合预期。
