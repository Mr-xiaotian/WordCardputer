# Web API 规范

> 最后更新日期: 2026/06/22

## 作用

本文档完整定义 WordCardputer 设备在 WiFi 连接成功后提供的 **HTTP API**。浏览器前端通过这套 API 实现文件管理、学习统计查看、设备设置调节和运行状态查询。

## 基础信息

| 项 | 值 |
|----|----|
| 协议 | HTTP |
| 端口 | 80 |
| Base URL | `http://<设备IP>` |
| CORS | 已启用，允许任意来源 |

## 通用响应格式

成功：

```json
{ "ok": true, ... }
```

错误：

```json
{ "ok": false, "error": "错误描述" }
```

## 路由详情

### 页面

#### `GET /`

返回 SD 卡 `/words_study/www/index.html`。若文件不存在，返回内置提示页面。

---

### 文件管理

#### `GET /api/files`

列出指定目录内容。

**参数：**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `path` | string | 否 | 目录路径，默认 `currentWordRoot` |

**示例请求：**

```bash
GET /api/files?path=/words_study/en/word
```

**示例响应：**

```json
{
  "path": "/words_study/en/word",
  "items": [
    { "name": "Demo_Basics.json", "isDir": false, "size": 1234 },
    { "name": "travel", "isDir": true }
  ]
}
```

#### `POST /api/files/upload`

上传文件到指定目录。

**参数：**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `path` | string | 否 | 目标目录，默认 `currentWordRoot` |

**请求体：** multipart/form-data，`file` 字段。

**示例请求：**

```bash
curl -X POST 'http://192.168.1.105/api/files/upload?path=/words_study/en/word' \
  -F 'file=@new_words.json'
```

**响应：**

```json
{ "ok": true }
```

#### `DELETE /api/files`

删除指定文件。

**参数：**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `path` | string | 是 | 要删除的文件路径 |

**示例请求：**

```bash
DELETE /api/files?path=/words_study/en/word/old.json
```

#### `GET /api/files/download`

下载指定文件。

**参数：**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `path` | string | 是 | 要下载的文件路径 |

**示例请求：**

```bash
GET /api/files/download?path=/words_study/en/word/Demo_Basics.json
```

---

### 学习统计

#### `GET /api/stats`

返回当前已加载词库的统计信息。

**示例响应：**

```json
{
  "file": "Demo_Basics.json",
  "total": 4,
  "avg": 3.25,
  "median": 3,
  "level": "掌握中",
  "counts": {
    "1": 0,
    "2": 1,
    "3": 2,
    "4": 1,
    "5": 0
  }
}
```

---

### 设备设置

#### `GET /api/settings`

获取当前设置。

**示例响应：**

```json
{
  "volume": 192,
  "brightness": 200,
  "language": "en",
  "loadedFile": "Demo_Basics.json",
  "wifi": true
}
```

#### `POST /api/settings`

修改音量或亮度。

**请求体：**

| 字段 | 类型 | 范围 | 说明 |
|------|------|------|------|
| `volume` | int | 0~255 | 扬声器音量 |
| `brightness` | int | 10~255 | 屏幕亮度 |

**示例请求：**

```bash
curl -X POST http://192.168.1.105/api/settings \
  -H 'Content-Type: application/json' \
  -d '{"volume":220,"brightness":255}'
```

**响应：**

```json
{ "ok": true, "volume": 220, "brightness": 200 }
```

> 注意：响应中的 `brightness` 当前返回的是 `normalBrightness` 常量，不是实时亮度。

---

### 设备信息

#### `GET /api/device`

获取设备运行状态。

**示例响应：**

```json
{
  "ip": "192.168.1.105",
  "freeHeap": 123456,
  "uptime": 3600
}
```

## CORS 预检

所有 `/api/*` 路由都支持 `OPTIONS` 预检请求，返回 204 并携带 CORS 头：

```
Access-Control-Allow-Origin: *
Access-Control-Allow-Methods: GET,POST,DELETE,OPTIONS
Access-Control-Allow-Headers: Content-Type
```

## 路径安全

所有涉及 SD 卡路径的 API 都通过 `isValidSDPath()` 校验：

- 路径必须以 `/words_study/` 开头。
- 路径中不能包含 `..`。

非法路径会返回：

```json
{ "ok": false, "error": "Invalid path" }
```

## 注意事项

- 上传大文件时采用 multipart 分块，ESP32 使用 1024 字节缓冲写入 SD 卡，避免内存溢出。
- `/api/stats` 在词库未加载时返回 `total: 0`。
- Web 服务器在 `loop()` 末尾非阻塞处理请求，频繁刷新浏览器不会明显影响设备交互。
