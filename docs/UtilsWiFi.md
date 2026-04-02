# UtilsWiFi.ino

## 概述
`UtilsWiFi.ino` 负责管理设备的**网络连接、凭据持久化与系统时钟同步**。这是支持 Web 控制台和正确生成时间戳文件（如错题本）的基础组件。

## 核心内容
- **WiFi 凭据持久化**：
  - **数据结构**：定义 `WiFiCredential` 结构体（`ssid` + `pass`），维护全局列表 `savedWiFiList`。
  - **存储格式**：以 JSON 数组形式保存在 `/words_study/wifi.json`，支持多组 WiFi 凭据。
  - **加载 (`loadSavedWiFiCredentials`)**：从 SD 卡读取 wifi.json，解析填充到 `savedWiFiList`。
  - **保存 (`saveWiFiCredential`)**：连接成功后自动调用。同名 SSID 更新密码，新 SSID 追加条目，写回 wifi.json。
  - **查找 (`findSavedPassword`)**：根据 SSID 查找已保存密码，供扫描列表自动连接使用。
- **扫描连接 (`attemptWiFiConnect`)**：
  - 使用用户选中的 SSID 和密码尝试连接（超时 10 秒）。
  - 连接成功后：同步 NTP、启用 Modem Sleep、保存凭据到 SD 卡、启动 Web 控制台。
- **扫描结果处理 (`processWiFiScanResults`)**：
  - 去重并按信号强度排序。
  - 已保存凭据的 SSID 在列表中显示 `★` 前缀标记。
- **省电管理**：
  - 连接成功后启用 `WIFI_PS_MIN_MODEM`（最小 Modem Sleep），在无数据传输时自动关闭射频以降低功耗。
- **时间获取 (`getNtpTimeString`)**：
  - 返回格式化时间字符串（`YY-MM-DD_HH-MM`），用于生成唯一文件名。
  - WiFi 未连接时回退到 `millis()` 值。
- **信号指示 (`rssiIndicator`)**：
  - 将 RSSI 数值转换为可视化信号强度符号（`[###]`、`[## ]`、`[#  ]`）。

## 关联模块
- **被调用方**：`ModeWiFiScan.ino` 调用扫描结果处理、凭据加载和连接函数。
- **服务提供**：为 `UtilsWebServer.ino` 提供底层网络层支持，为 `UtilsData.ino`（保存错题时）提供时间戳。
