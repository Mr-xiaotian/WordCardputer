# UtilsWiFi.ino

## 概述
`UtilsWiFi.ino` 负责管理设备的**网络连接与系统时钟同步**。这是支持 Web 控制台和正确生成时间戳文件（如错题本）的基础组件。

## 核心内容
- **读取凭据与连接 (`connectWiFi`)**：
  - **凭据读取**：启动时尝试打开 SD 卡上的 `/words_study/.env` 配置文件，逐行解析以提取 `WIFI_SSID` 和 `WIFI_PASS`。
  - **网络连接**：调用 ESP32 的 `WiFi.begin` 接口进行连接。
  - **超时控制**：在 `while` 循环中加入计时器（最大 8 秒），防止因网络问题导致设备在启动阶段永久卡死。
  - **NTP 同步**：如果成功连接到 WiFi，则立即调用 `configTime` 连接到 `pool.ntp.org` 进行时间同步，并将系统时区硬编码设置为东八区（UTC+8）。
- **时间获取 (`getFormattedTime`)**：
  - 提供一个快速获取当前本地时间的函数。
  - 通过 `getLocalTime` 获取结构化的时间信息，并将其格式化为紧凑的字符串（例如 `20240321_143000`），主要用于生成唯一的文件名。

## 关联模块
- **前置依赖**：在 `WordCardputer.ino` 的 `setup` 阶段最先被调用，为后续的 Web 服务启动扫清障碍。
- **服务提供**：为 `UtilsWebServer.ino` 提供底层网络层支持，为 `UtilsData.ino`（保存错题时）提供时间戳。