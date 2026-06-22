# WordCardputer 文档索引

> 最后更新日期: 2026/06/22

本目录包含 WordCardputer 项目的详细技术文档。文档按源码模块组织，每篇文档对应一个 `.ino` 文件或一个主题。

## 快速导航

### 主程序与架构

| 文档 | 说明 |
|------|------|
| [WordCardputer.md](WordCardputer.md) | 主程序入口、全局状态、`setup()` / `loop()` |
| [DataFormat.md](DataFormat.md) | SD 卡目录结构、JSON 字段规范、音频文件要求 |
| [WebAPI.md](WebAPI.md) | Web 控制面板 HTTP API 完整规范 |
| [PythonTools.md](PythonTools.md) | PC 端 Python 工具链使用说明 |

### 功能模式

| 文档 | 对应源码 | 说明 |
|------|---------|------|
| [ModeLangSelect.md](ModeLangSelect.md) | `ModeLangSelect.ino` | 启动语言选择 |
| [ModeFileSelect.md](ModeFileSelect.md) | `ModeFileSelect.ino` | SD 卡词库浏览器 |
| [ModeStudy.md](ModeStudy.md) | `ModeStudy.ino` | 双面闪卡学习 |
| [ModeDictation.md](ModeDictation.md) | `ModeDictation.ino` | 听写测试（日/英） |
| [ModeListen.md](ModeListen.md) | `ModeListen.ino` | 自动循环听读 |
| [ModeStats.md](ModeStats.md) | `ModeStats.ino` | 学习统计报表 |
| [ModeEscMenu.md](ModeEscMenu.md) | `ModeEscMenu.ino` | 全局 ESC 菜单 |
| [ModeKeyHelp.md](ModeKeyHelp.md) | `ModeKeyHelp.ino` | 按键帮助页面 |
| [ModeWiFiScan.md](ModeWiFiScan.md) | `ModeWiFiScan.ino` | WiFi 扫描与连接 |

### 工具模块

| 文档 | 对应源码 | 说明 |
|------|---------|------|
| [UtilsData.md](UtilsData.md) | `UtilsData.ino` | JSON 读写、加权抽词、自动保存 |
| [UtilsAudio.md](UtilsAudio.md) | `UtilsAudio.ino` | WAV 流式播放、音量调节 |
| [UtilsMenu.md](UtilsMenu.md) | `UtilsMenu.ino` | 菜单与表格绘制 |
| [UtilsString.md](UtilsString.md) | `UtilsString.ino` | 自适应文本、IPA 转 ASCII |
| [UtilsIme.md](UtilsIme.md) | `UtilsIme.ino` | 日语罗马音→假名 IME |
| [UtilsWiFi.md](UtilsWiFi.md) | `UtilsWiFi.ino` | WiFi 凭据、NTP 时间 |
| [UtilsWebServer.md](UtilsWebServer.md) | `UtilsWebServer.ino` | HTTP 服务器 |

## 文档规范

所有文档遵循统一的骨架结构：

1. **作用**：一句话说明模块职责。
2. **核心对象 / 核心函数**：关键变量、结构体、函数列表。
3. **关键流程**：Mermaid 流程图展示主要逻辑。
4. **重要细节**：算法、边界情况、配置参数。
5. **使用示例**：代码片段或用户操作示例。
6. **注意事项**：常见误区、依赖关系、已修正的历史错误。

## 最近更新重点

- 重写 `ModeStudy.md`、`ModeDictation.md`、`ModeListen.md`，修正与代码不一致的按键说明和语言支持描述。
- 新增 `ModeKeyHelp.md`、`ModeWiFiScan.md`、`UtilsIme.md`。
- 新增 `DataFormat.md`、`WebAPI.md`、`PythonTools.md` 三份主题文档。
- 新增本文档索引。
