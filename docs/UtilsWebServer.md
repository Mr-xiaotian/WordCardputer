# UtilsWebServer.ino

## 概述
`UtilsWebServer.ino` 实现了运行在 ESP32 上的**轻量级 HTTP Web 控制面板服务器**。当设备成功连接 WiFi 时，该模块会启动，允许用户通过局域网内浏览器直接管理设备和数据。

## 核心内容
- **静态资源托管**：将 SD 卡中的 `index.html` 前端页面流式传输给客户端浏览器。
- **文件管理 API**：
  - `GET /api/files`：读取 SD 卡指定目录下的文件和文件夹列表。
  - `POST /api/files/upload`：支持分块上传文件到 SD 卡，解决大文件传输的内存瓶颈。
  - `DELETE /api/files`：删除 SD 卡上的指定文件。
  - `GET /api/files/download`：从 SD 卡下载文件到本地设备。
- **数据统计 API**：
  - `GET /api/stats`：调用 `computeStatsFromWords()` 动态生成并返回当前词库的学习进度统计数据（平均分、中位数、等级分布等）。
- **设备控制 API**：
  - `GET /api/settings`：获取当前设备的音量、亮度、已加载文件及 WiFi 状态。
  - `POST /api/settings`：接收 JSON 请求以远程调整设备的扬声器音量和屏幕亮度，并在设备端即时生效。
  - `GET /api/device`：返回系统状态，如 IP 地址、可用堆内存（Heap）和系统运行时间。
- **安全与配置**：
  - 包含 SD 卡路径校验函数（防止目录穿越攻击）。
  - 支持跨域资源共享（CORS）配置。

## 关联模块
- **网络连接**：依赖 `UtilsWiFi.ino` 建立的 WiFi 连接。
- **数据处理**：与 `UtilsData.ino` 和 `ModeStats.ino` 交互以获取并处理统计数据。