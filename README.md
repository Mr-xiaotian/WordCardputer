# M5Cardputer 日语单词学习工具

一个运行在 **M5Cardputer** 上的离线日语单词记忆器。
项目支持从 SD 卡读取 JSON 词库文件、随机抽取单词进行记忆测试，并可播放对应发音音频。
适合日语学习者随时巩固词汇。

## ✨ 功能特色

* 📁 **词库选择**：自动扫描 `/jp_words_study/word/` 文件夹中的 `.json` 词库文件，并支持菜单选择。
* 🔊 **语音播放**：从 `/jp_words_study/audio/` 目录加载 `.wav` 音频并播放发音。
* 🎯 **智能抽词算法**：根据熟练度动态加权随机抽取，未掌握的单词更容易出现。
* 🔄 **中日双向模式**：支持“日语→中文”与“中文→日语”两种随机切换模式。
* 🌙 **自动节能**：长时间无操作会自动降低屏幕亮度以节省电量。
* 💾 **离线运行**：全部数据与音频均存储在 SD 卡中，完全离线可用。

## 快速开始

1. 准备 SD 卡内容
将项目中的 jp_words_study 文件夹完整复制到 SD 卡根目录。

2. 编译与烧录程序
- 打开 WordCardputer.ino（使用 Arduino IDE 或 PlatformIO）
- 选择开发板类型：M5Stack Cardputer (ESP32S3)
- 编译并上传代码到设备

## 📂 文件结构示例

```
📁 WordCardputer	(11MB 107KB 979B)
    📁 jp_words_study	(6MB 370KB 839B)
        📁 audio	(6MB 362KB 136B)
            📄 [112项排除的文件]	(6MB 362KB 136B)
        📁 word 	(8KB 703B)
            📄 [3项排除的文件]	(8KB 703B)
    📁 temp          	(507KB 724B)
        📄 [1项排除的文件]	(507KB 724B)
    📁 [1项排除的目录]	(4MB 212KB 157B)
    ❓ .gitignore       	(23B)
    📝 README.md        	(2KB 604B)
    🐍 tools.py         	(3KB 334B)
    ❓ WordCardputer.ino	(12KB 760B)
    📄 [1项排除的文件]	(22KB 610B)
```

(该视图由我的另一个项目[CelestialVault](https://github.com/Mr-xiaotian/CelestialVault)中inst_file生成。)

词库 JSON 格式：

```json
[
  { "jp": "わたし", "zh": "我", "kanji": "私", "tone": 0 },
  { "jp": "ほん", "zh": "书", "kanji": "本", "tone": 0 }
]
```

## ⚙️ 使用方法

1. 将词库 JSON 和音频文件放入 SD 卡指定目录。在word和audio中可以按需求放入文件, word格式参照上述, wav要求如下:
   - PCM 格式 (audiofmt == 1);
   - 8-bit 或 16-bit；
   - 1 或 2 通道；
   - 采样率 ≤ 48kHz。
2. 启动设备后选择词库文件。
3. * 按 **Go 键** 切换显示释义
   * 键盘 **Enter** 表示记住（熟练度 +1）
   * 键盘 **Del** 表示不熟（熟练度 -1）
   * 键盘 **A** 键播放日语发音
4. 无操作 60 秒后自动进入节能模式。

## 🚧 TODO / 未来计划

* ✅ 支持发音播放
* ✅ 支持双向模式
* 🔲 保存熟练度进度到 SD
* 🔲 增加复习统计与学习历史
* 🔲 可视化学习报告（例如熟悉度分布图）

## 🪶 作者

Author: Mr-xiaotian 
Email: mingxiaomingtian@gmail.com  
Project Link: [https://github.com/Mr-xiaotian/WordCardputer](https://github.com/Mr-xiaotian/WordCardputer)

> *“让硬件成为记忆的一部分。”*

