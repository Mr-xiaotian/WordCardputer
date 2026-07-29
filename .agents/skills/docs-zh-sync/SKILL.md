---
name: "docs-zh-sync"
description: "WordCardputer 代码→中文文档同步。Audits src/*.cpp/.h and updates matching docs/zh-CN markdown files. Invoke when code changes require Chinese docs sync."
---

# Docs Zh Sync — WordCardputer 项目专属配置

## 项目概况

- 语言: C++ (Arduino/PlatformIO), Python
- 源码扩展名: `.cpp`, `.h`, `.py`
- 目标文档路径: `docs/zh-CN/*.md`（扁平结构，不使用 `src/` 子目录镜像）

## 子任务划分

每次同步时，主 Agent 按以下 3 个区域并行委派子代理：

| 子任务 | 名称 | Subagent Prompt | 扫描目录 | 说明 |
|:------:|------|----------------|---------|------|
| 1 | Mode 模式 | `subagent-modes.md` | `src/Mode*.cpp`, `src/Mode*.h` | 所有 Mode 类功能文档 |
| 2 | Utils 工具 | `subagent-utils.md` | `src/Utils*.cpp`, `src/Utils*.h` | 工具与数据层文档 |
| 3 | 核心与跨领域 | `subagent-core.md` | `src/main.cpp`, `src/globals.cpp`, `src/globals.h`, `utils/*.py` | 主程序入口、全局状态、Python 工具、主题文档 |

### 子任务 1: Mode 模式

负责所有以 `Mode` 开头的 C++ 源文件对应的文档。

| 源码 | 目标文档 |
|------|---------|
| `src/ModeClassifySelect.cpp/.h` | `docs/zh-CN/ModeClassifySelect.md` |
| `src/ModeClock.cpp/.h` | `docs/zh-CN/ModeClock.md` |
| `src/ModeDictation.cpp/.h` | `docs/zh-CN/ModeDictation.md` |
| `src/ModeDictationReview.cpp/.h` | `docs/zh-CN/ModeDictationReview.md` |
| `src/ModeEscMenu.cpp/.h` | `docs/zh-CN/ModeEscMenu.md` |
| `src/ModeKeyHelp.cpp/.h` | `docs/zh-CN/ModeKeyHelp.md` |
| `src/ModeLangSelect.cpp/.h` | `docs/zh-CN/ModeLangSelect.md` |
| `src/ModeListen.cpp/.h` | `docs/zh-CN/ModeListen.md` |
| `src/ModeScoreSelect.cpp/.h` | `docs/zh-CN/ModeScoreSelect.md` |
| `src/ModeSourceSelect.cpp/.h` | `docs/zh-CN/ModeSourceSelect.md` |
| `src/ModeSplash.cpp/.h` | `docs/zh-CN/ModeSplash.md` |
| `src/ModeStats.cpp/.h` | `docs/zh-CN/ModeStats.md` |
| `src/ModeStudy.cpp/.h` | `docs/zh-CN/ModeStudy.md` |
| `src/ModeWiFiScan.cpp/.h` | `docs/zh-CN/ModeWiFiScan.md` |
| `src/ModeWordTable.cpp/.h` | `docs/zh-CN/ModeWordTable.md` |

### 子任务 2: Utils 工具

负责所有以 `Utils` 开头的 C++ 源文件对应的文档。

| 源码 | 目标文档 |
|------|---------|
| `src/UtilsAudio.cpp/.h` | `docs/zh-CN/UtilsAudio.md` |
| `src/UtilsConfig.cpp/.h` | `docs/zh-CN/UtilsConfig.md` |
| `src/UtilsData.cpp/.h` | `docs/zh-CN/UtilsData.md` |
| `src/UtilsDb.cpp/.h` | `docs/zh-CN/UtilsDb.md` |
| `src/UtilsIme.cpp/.h` | `docs/zh-CN/UtilsIme.md` |
| `src/UtilsMenu.cpp/.h` | `docs/zh-CN/UtilsMenu.md` |
| `src/UtilsString.cpp/.h` | `docs/zh-CN/UtilsString.md` |
| `src/UtilsTable.cpp/.h` | `docs/zh-CN/UtilsTable.md` |
| `src/UtilsWebServer.cpp/.h` | `docs/zh-CN/UtilsWebServer.md` |
| `src/UtilsWiFi.cpp/.h` | `docs/zh-CN/UtilsWiFi.md` |

### 子任务 3: 核心与跨领域

负责主程序入口、全局状态定义、Python 工具及主题文档。

| 源码 | 目标文档 |
|------|---------|
| `src/main.cpp` + `src/globals.cpp/.h` | `docs/zh-CN/WordCardputer.md` |
| — (主题文档) | `docs/zh-CN/DataFormat.md` |
| `src/UtilsWebServer.cpp/.h` (API 部分) | `docs/zh-CN/WebAPI.md` |
| `utils/*.py` | `docs/zh-CN/PythonTools.md` |
| — (索引) | `docs/zh-CN/README.md` |

## 并行度

项目规则限制最多 5 个子代理并行。当前 3 个子任务可一次性全部并行委派。
