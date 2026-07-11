# Web API 规范

> 最后更新日期: 2026/07/11

## 作用

本文档完整定义 WordCardputer 设备在 WiFi 连接成功后提供的 **HTTP API**。浏览器前端通过这套 API 实现词库数据库管理、学习统计查看、设备设置调节和运行状态查询。

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

### 词库管理

#### `GET /api/files`

浏览词库数据库的 source / chapter 结构。

**参数：**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `path` | string | 否 | 虚拟路径，默认 `currentWordRoot` |

**示例请求：**

```bash
# 浏览根层（列出所有 source）
GET /api/files?path=/words_study/en/word

# 浏览 source 层（列出 Chapter）
GET /api/files?path=/words_study/en/word/Demo_Basics
```

**根层响应：**

```json
{
  "path": "/words_study/en/word",
  "root": "/words_study/en/word",
  "language": "en",
  "items": [
    { "name": "Demo_Basics", "kind": "source", "isDir": true, "wordCount": 50, "chapterCount": 3 },
    { "name": "N5", "kind": "source", "isDir": false, "wordCount": 120, "chapterCount": 0 }
  ]
}
```

**source 层响应：**

```json
{
  "path": "/words_study/en/word/Demo_Basics",
  "root": "/words_study/en/word",
  "language": "en",
  "items": [
    { "name": "全部", "kind": "chapter", "isDir": false, "wordCount": 50 },
    { "name": "Unit_1", "kind": "chapter", "isDir": false, "wordCount": 25 }
  ]
}
```

#### `POST /api/files/upload`

上传 JSON 词库文件并自动导入到 SQLite 数据库。

**参数：**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `path` | string | 否 | 目标虚拟路径，影响 source/chapter 推导 |

**请求体：** multipart/form-data，`file` 字段。

**示例请求：**

```bash
# 上传到根层：文件名 stem 作为 source
curl -X POST 'http://192.168.1.105/api/files/upload?path=/words_study/en/word' \
  -F 'file=@Demo_Basics.json'

# 上传到 source 层：文件名 stem 作为 chapter
curl -X POST 'http://192.168.1.105/api/files/upload?path=/words_study/en/word/Demo_Basics' \
  -F 'file=@Unit_2.json'
```

**响应：**

```json
{ "ok": true, "source": "Demo_Basics", "chapter": "", "imported": 50 }
```

#### `DELETE /api/files`

删除指定 source 或 chapter（数据库操作，事务保证）。

**参数：**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `path` | string | 是 | 要删除的虚拟路径 |

**示例请求：**

```bash
DELETE /api/files?path=/words_study/en/word/Demo_Basics/Unit_1
```

**响应：**

```json
{ "ok": true, "deleted": 25 }
```

#### `GET /api/files/download`

将 source 或 chapter 导出为 JSON 文件下载。

**参数：**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `path` | string | 是 | 要导出的虚拟路径 |

**示例请求：**

```bash
GET /api/files/download?path=/words_study/en/word/Demo_Basics
```

响应为流式 JSON 数组，文件名格式为 `{source}_{chapter}.json`。

---

### 学习统计

#### `GET /api/stats`

返回当前已加载词库的统计信息。

**示例响应：**

```json
{
  "file": "Unit_1",
  "label": "Demo_Basics/Unit_1",
  "source": "Demo_Basics",
  "chapter": "Unit_1",
  "total": 25,
  "avg": 3.25,
  "median": 3,
  "level": "掌握中",
  "counts": {
    "1": 0,
    "2": 1,
    "3": 20,
    "4": 3,
    "5": 1
  }
}
```

**新增字段：**
| 字段 | 说明 |
|------|------|
| `label` | 完整词库标签（`source/chapter` 或 `source`） |
| `source` | 词库来源标识 |
| `chapter` | 章节标识，空字符串表示整个 source |

---

### 设备设置

#### `GET /api/settings`

获取当前设置。

**示例响应：**

```json
{
  "volume": 192,
  "brightness": 200,
  "autoSaveThreshold": 5,
  "language": "en",
  "loadedFile": "Unit_1",
  "loadedVocab": "Demo_Basics/Unit_1",
  "wifi": true
}
```

**新增字段：**
| 字段 | 说明 |
|------|------|
| `autoSaveThreshold` | 自动保存触发次数阈值 |
| `loadedVocab` | 当前词库完整标签 |

#### `POST /api/settings`

修改设备设置，修改后自动持久化到 `config.json`。

**请求体：**

| 字段 | 类型 | 范围 | 说明 |
|------|------|------|------|
| `volume` | int | 0~255 | 扬声器音量 |
| `brightness` | int | 10~255 | 屏幕亮度 |
| `autoSaveThreshold` | int | ≥1 | 自动保存触发次数（新增） |

**示例请求：**

```bash
curl -X POST http://192.168.1.105/api/settings \
  -H 'Content-Type: application/json' \
  -d '{"volume":220,"brightness":255,"autoSaveThreshold":10}'
```

**响应：**

```json
{ "ok": true, "volume": 220, "brightness": 200, "autoSaveThreshold": 10 }
```

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

所有涉及 SD 卡路径的 API 都通过 `parseVocabPath()` 校验虚拟路径格式。

## 注意事项

- 词库操作（浏览、上传、删除、导出）均通过 SQLite 数据库执行，不再直接操作 SD 卡文件系统中的 JSON 文件。
- 上传大文件时采用 multipart 分块，先写入临时文件再导入数据库，导入成功后自动清理。
- `/api/stats` 在词库未加载时返回 `total: 0`。
- Web 服务器在 `loop()` 末尾非阻塞处理请求，频繁刷新浏览器不会明显影响设备交互。
- 删除操作使用 SQLite 事务（BEGIN IMMEDIATE → DELETE → COMMIT），保证数据一致性。
