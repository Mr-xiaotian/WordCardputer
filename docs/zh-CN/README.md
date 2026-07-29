# WordCardputer 文档索引

> 最后更新日期: 2026/07/29

本目录包含 WordCardputer 项目的详细技术文档。文档按源码模块组织，每篇文档对应一个 `.cpp` / `.h` 文件或一个主题。

## 快速导航

### 主程序与架构

| 文档 | 说明 |
|------|------|
| [WordCardputer.md](WordCardputer.md) | 主程序入口、全局状态、`setup()` / `loop()` |
| [DataFormat.md](DataFormat.md) | SD 卡目录结构、SQLite 数据库 schema、JSON 兼容格式、音频文件要求 |
| [WebAPI.md](WebAPI.md) | Web 控制面板 HTTP API 完整规范 |
| [PythonTools.md](PythonTools.md) | PC 端 Python 工具链使用说明 |

### 功能模式

| 文档 | 对应源码 | 说明 |
|------|---------|------|
| [ModeLangSelect.md](ModeLangSelect.md) | `ModeLangSelect.cpp` | 启动语言选择 |
| [ModeClassifySelect.md](ModeClassifySelect.md) | `ModeClassifySelect.cpp` | 分类方式选择（按词源/按 Score） |
| [ModeSourceSelect.md](ModeSourceSelect.md) | `ModeSourceSelect.cpp` | 词库浏览器（数据库驱动） |
| [ModeScoreSelect.md](ModeScoreSelect.md) | `ModeScoreSelect.cpp` | 按 Score 分类的词库选择 |
| [ModeStudy.md](ModeStudy.md) | `ModeStudy.cpp` | 双面闪卡学习 |
| [ModeDictation.md](ModeDictation.md) | `ModeDictation.cpp` | 听写测试（日/英） |
| [ModeDictationReview.md](ModeDictationReview.md) | `ModeDictationReview.cpp` | 听写错题回顾页 |
| [ModeListen.md](ModeListen.md) | `ModeListen.cpp` | 自动循环听读 |
| [ModeStats.md](ModeStats.md) | `ModeStats.cpp` | 学习统计报表 |
| [ModeWordTable.md](ModeWordTable.md) | `ModeWordTable.cpp` | 当前词表（按分数分组） |
| [ModeEscMenu.md](ModeEscMenu.md) | `ModeEscMenu.cpp` | 全局 ESC 菜单 |
| [ModeKeyHelp.md](ModeKeyHelp.md) | `ModeKeyHelp.cpp` | 按键帮助页面 |
| [ModeWiFiScan.md](ModeWiFiScan.md) | `ModeWiFiScan.cpp` | WiFi 扫描与连接 |
| [ModeClock.md](ModeClock.md) | `ModeClock.cpp` | 时间显示页面 |
| [ModeSplash.md](ModeSplash.md) | `ModeSplash.cpp` | 启动画面（ASCII Logo） |

### 工具模块

| 文档 | 对应源码 | 说明 |
|------|---------|------|
| [UtilsData.md](UtilsData.md) | `UtilsData.cpp` | 运行时词库状态、加权抽词、自动保存、统计计算 |
| [UtilsDb.md](UtilsDb.md) | `UtilsDb.cpp` | SQLite 数据库访问层、词库 CRUD、导入导出 |
| [UtilsConfig.md](UtilsConfig.md) | `UtilsConfig.cpp` | 设备配置持久化（config.json） |
| [UtilsAudio.md](UtilsAudio.md) | `UtilsAudio.cpp` | WAV 流式播放、音量调节 |
| [UtilsMenu.md](UtilsMenu.md) | `UtilsMenu.cpp` | 菜单与表格绘制 |
| [UtilsTable.md](UtilsTable.md) | `UtilsTable.cpp` | 通用表格绘制工具 |
| [UtilsString.md](UtilsString.md) | `UtilsString.cpp` | 自适应文本、IPA 转 ASCII |
| [UtilsIme.md](UtilsIme.md) | `UtilsIme.cpp` | 日语罗马音→假名 IME |
| [UtilsWiFi.md](UtilsWiFi.md) | `UtilsWiFi.cpp` | WiFi 凭据、NTP 时间 |
| [UtilsWebServer.md](UtilsWebServer.md) | `UtilsWebServer.cpp` | HTTP 服务器 |

## 文档规范

所有文档遵循统一的骨架结构：

1. **作用**：一句话说明模块职责。
2. **核心对象 / 核心函数**：关键变量、结构体、函数列表。
3. **关键流程**：Mermaid 流程图展示主要逻辑。
4. **重要细节**：算法、边界情况、配置参数。
5. **使用示例**：代码片段或用户操作示例。
6. **注意事项**：常见误区、依赖关系、已修正的历史错误。

## 最近更新重点

- **2026/07/29**：同步新增的 5 个模式（ModeSplash、ModeClassifySelect、ModeScoreSelect、ModeWordTable、ModeClock），更新全局变量表、AppMode 列表与结构体定义。修正 DataFormat.md 中数据库表结构（新增 `sentence`/`sentence_zh` 列、词根词缀关联表），修正听写错误表列名 `wrong` → `wrong_text`，修正 journal 模式 WAL → DELETE。修正 WebAPI.md 中 `/api/stats` 和 `/api/settings` 响应字段名为 `vocabLabel`。更新 PythonTools.md 新增 `fill_missing_audio` 及英语 JSON 工具函数。更新索引文档。
- **2026/07/11**：全面同步代码变更，新增 `ModeDictationReview.md`、`UtilsConfig.md`、`UtilsDb.md` 三份文档。更新主程序、ESC 菜单、数据管理、数据格式、WiFi、Web 服务器、Web API 和 Python 工具文档，反映 SQLite 数据库迁移、配置系统统一、错题回顾独立模式等重大重构。
- 重写 `ModeStudy.md`、`ModeDictation.md`、`ModeListen.md`，修正与代码不一致的按键说明和语言支持描述。
- 新增 `ModeKeyHelp.md`、`ModeWiFiScan.md`、`UtilsIme.md`。
- 新增 `DataFormat.md`、`WebAPI.md`、`PythonTools.md` 三份主题文档。
- 新增本文档索引。
