# ModeLangSelect.ino

## 概述
`ModeLangSelect.ino` 实现了**语言选择模式**，通常作为设备启动后的首个交互界面。它允许用户在多语言（当前支持日语和英语）学习库之间进行切换。

## 核心内容
- **界面渲染 (`drawLanguageSelect`)**：
  - 定义了可选语言的静态数组 `langItems`（包含 "日语 (Japanese)" 和 "英语 (English)"）。
  - 调用 `UtilsMenu.ino` 的 `drawTextMenu` 函数，在屏幕上绘制一个简洁的上下选择菜单。
- **用户交互 (`loopLangSelectMode`)**：
  - 监听 M5Cardputer 的键盘输入：
    - `;/` 键：上下移动高亮光标。
    - `Enter` 键：确认当前选择的语言。
- **环境切换逻辑**：
  - 用户确认选择后，代码会更新全局的 `currentLanguage` 枚举变量。
  - 最关键的是：它会根据选择重新绑定底层的文件系统路径。
    - 日语：设置 `currentWordRoot` 为 `/words_study/jp/word/`，音频根目录为 `/words_study/jp/audio/`。
    - 英语：设置 `currentWordRoot` 为 `/words_study/en/word/`，音频根目录为 `/words_study/en/audio/`。
  - 路径切换完成后，自动将状态机流转到 `MODE_FILE_SELECT`，引导用户进入对应语言的词库目录。

## 关联模块
- **依赖**：使用 `UtilsMenu.ino` 进行菜单渲染。
- **后继流程**：设置全局变量后，跳转到 `ModeFileSelect.ino` 继续执行。