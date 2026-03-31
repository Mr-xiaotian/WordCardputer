/**
 * @file UtilsWebServer.ino
 * @brief Web 控制面板服务器
 *
 * 在 ESP32 上运行轻量级 HTTP 服务器，提供浏览器端的设备管理面板。
 * 功能包括：SD 卡文件浏览/上传/删除、学习统计查看、音量亮度调节。
 * 仅在 WiFi 连接成功后启动，前端页面从 SD 卡加载。
 */

#include <WebServer.h>

WebServer server(80);
bool webServerRunning = false;
File uploadFile;

/**
 * 校验 SD 卡路径是否合法
 *
 * 路径必须以 /words_study/ 开头且不包含 ".."，防止目录穿越攻击。
 *
 * @param path 待校验的路径
 * @return true 路径合法，false 路径非法
 */
bool isValidSDPath(const String &path) {
    return path.startsWith("/words_study/") && path.indexOf("..") == -1;
}

/**
 * 发送 JSON 格式的错误响应
 *
 * @param code HTTP 状态码
 * @param msg 错误描述信息
 */
void sendJsonError(int code, const String &msg) {
    server.send(code, "application/json", "{\"ok\":false,\"error\":\"" + msg + "\"}");
}

/**
 * 添加 CORS 响应头
 */
void sendCorsHeaders() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET,POST,DELETE,OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

/**
 * 处理 OPTIONS 预检请求
 */
void handleOptions() {
    sendCorsHeaders();
    server.send(204);
}

// ========== 页面 ==========

/**
 * 处理根路径请求，从 SD 卡流式返回 index.html
 */
void handleRoot() {
    sendCorsHeaders();
    File f = SD.open("/words_study/www/index.html");
    if (!f) {
        server.send(200, "text/html",
            "<html><body style='background:#111;color:#fff;font-family:sans-serif;text-align:center;padding-top:80px'>"
            "<h2>WordCardputer</h2>"
            "<p>Please put index.html to /words_study/www/ on SD card.</p>"
            "</body></html>");
        return;
    }
    server.streamFile(f, "text/html");
    f.close();
}

// ========== 文件管理 API ==========

/**
 * GET /api/files?path=<dir> — 列出目录下的文件和文件夹
 */
void handleApiFiles() {
    sendCorsHeaders();
    String path = server.arg("path");
    if (path.isEmpty()) {
        path = currentWordRoot;
    }
    if (!isValidSDPath(path)) {
        sendJsonError(400, "Invalid path");
        return;
    }

    File dir = SD.open(path);
    if (!dir || !dir.isDirectory()) {
        sendJsonError(404, "Directory not found");
        if (dir) dir.close();
        return;
    }

    DynamicJsonDocument doc(4096);
    doc["path"] = path;
    JsonArray items = doc.createNestedArray("items");

    int count = 0;
    while (count < 100) {
        File entry = dir.openNextFile();
        if (!entry) break;

        String name = entry.name();
        if (name == "." || name == "..") {
            entry.close();
            continue;
        }

        JsonObject item = items.createNestedObject();
        item["name"] = name;
        item["isDir"] = entry.isDirectory();
        if (!entry.isDirectory()) {
            item["size"] = entry.size();
        }
        entry.close();
        count++;
    }
    dir.close();

    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

/**
 * POST /api/files/upload?path=<dir> — 文件上传完成后的响应处理
 */
void handleApiFileUpload() {
    sendCorsHeaders();
    server.send(200, "application/json", "{\"ok\":true}");
}

/**
 * 文件上传分块数据处理
 *
 * 处理 multipart 上传的各阶段：开始时创建文件、写入数据块、结束时关闭文件。
 * 上传目标路径由查询参数 path 指定。
 */
void handleApiFileUploadData() {
    HTTPUpload &upload = server.upload();
    String dir = server.arg("path");
    if (dir.isEmpty()) dir = currentWordRoot;

    if (upload.status == UPLOAD_FILE_START) {
        if (!isValidSDPath(dir + "/" + upload.filename)) {
            return;
        }
        String filePath = dir + "/" + upload.filename;
        uploadFile = SD.open(filePath, FILE_WRITE);
        Serial.printf("[Web] Upload start: %s\n", filePath.c_str());
    }
    else if (upload.status == UPLOAD_FILE_WRITE) {
        if (uploadFile) {
            uploadFile.write(upload.buf, upload.currentSize);
        }
    }
    else if (upload.status == UPLOAD_FILE_END) {
        if (uploadFile) {
            uploadFile.close();
            Serial.printf("[Web] Upload done: %d bytes\n", upload.totalSize);
        }
    }
    else if (upload.status == UPLOAD_FILE_ABORTED) {
        if (uploadFile) {
            uploadFile.close();
            Serial.println("[Web] Upload aborted");
        }
    }
}

/**
 * DELETE /api/files?path=<filepath> — 删除 SD 卡上的文件
 */
void handleApiFileDelete() {
    sendCorsHeaders();
    String path = server.arg("path");
    if (!isValidSDPath(path)) {
        sendJsonError(400, "Invalid path");
        return;
    }
    if (SD.remove(path)) {
        server.send(200, "application/json", "{\"ok\":true}");
    } else {
        sendJsonError(500, "Delete failed");
    }
}

/**
 * GET /api/files/download?path=<filepath> — 下载 SD 卡上的文件
 */
void handleApiFileDownload() {
    sendCorsHeaders();
    String path = server.arg("path");
    if (!isValidSDPath(path)) {
        sendJsonError(400, "Invalid path");
        return;
    }
    File f = SD.open(path);
    if (!f) {
        sendJsonError(404, "File not found");
        return;
    }
    server.streamFile(f, "application/octet-stream");
    f.close();
}

// ========== 统计 API ==========

/**
 * GET /api/stats — 返回当前词库的学习统计数据
 */
void handleApiStats() {
    sendCorsHeaders();
    computeStatsFromWords();

    DynamicJsonDocument doc(512);
    doc["file"] = statsFileName(selectedFilePath);
    doc["total"] = statsTotal;
    doc["avg"] = round(statsAvg * 100) / 100.0;
    doc["median"] = round(statsMedian * 100) / 100.0;
    doc["level"] = statsLevel;
    JsonObject counts = doc.createNestedObject("counts");
    for (int i = 1; i <= 5; i++) {
        counts[String(i)] = statsCounts[i];
    }

    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

// ========== 设置 API ==========

/**
 * GET /api/settings — 返回当前设备设置
 */
void handleApiSettingsGet() {
    sendCorsHeaders();
    DynamicJsonDocument doc(256);
    doc["volume"] = soundVolume;
    doc["brightness"] = normalBrightness;
    doc["language"] = (currentLanguage == LANG_JP) ? "jp" : "en";
    doc["loadedFile"] = statsFileName(selectedFilePath);
    doc["wifi"] = wifiConnected;

    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

/**
 * POST /api/settings — 修改音量或亮度设置
 *
 * 接受 JSON body，支持 "volume"(0-255) 和 "brightness"(10-255) 字段。
 */
void handleApiSettingsPost() {
    sendCorsHeaders();
    DynamicJsonDocument doc(256);
    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (err) {
        sendJsonError(400, "Invalid JSON");
        return;
    }

    if (doc.containsKey("volume")) {
        soundVolume = constrain(doc["volume"].as<int>(), 0, 255);
        M5.Speaker.setVolume(soundVolume);
    }
    if (doc.containsKey("brightness")) {
        uint8_t b = constrain(doc["brightness"].as<int>(), 10, 255);
        M5Cardputer.Display.setBrightness(b);
    }

    // 返回当前值
    DynamicJsonDocument res(128);
    res["ok"] = true;
    res["volume"] = soundVolume;
    res["brightness"] = (int)normalBrightness;
    String json;
    serializeJson(res, json);
    server.send(200, "application/json", json);
}

// ========== 设备信息 API ==========

/**
 * GET /api/device — 返回设备运行状态信息
 */
void handleApiDevice() {
    sendCorsHeaders();
    DynamicJsonDocument doc(256);
    doc["ip"] = WiFi.localIP().toString();
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["uptime"] = millis() / 1000;

    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

/**
 * 404 处理
 */
void handleNotFound() {
    sendCorsHeaders();
    sendJsonError(404, "Not found");
}

// ========== 初始化与循环 ==========

/**
 * 初始化 Web 服务器
 *
 * 注册所有 API 路由，启动 HTTP 服务器。
 * 仅在 WiFi 已连接时调用。启动后在串口输出访问地址。
 */
void initWebServer() {
    // 页面
    server.on("/", HTTP_GET, handleRoot);

    // 文件管理
    server.on("/api/files", HTTP_GET, handleApiFiles);
    server.on("/api/files/upload", HTTP_POST, handleApiFileUpload, handleApiFileUploadData);
    server.on("/api/files", HTTP_DELETE, handleApiFileDelete);
    server.on("/api/files/download", HTTP_GET, handleApiFileDownload);

    // 统计
    server.on("/api/stats", HTTP_GET, handleApiStats);

    // 设置
    server.on("/api/settings", HTTP_GET, handleApiSettingsGet);
    server.on("/api/settings", HTTP_POST, handleApiSettingsPost);

    // 设备信息
    server.on("/api/device", HTTP_GET, handleApiDevice);

    // CORS 预检
    server.on("/api/files", HTTP_OPTIONS, handleOptions);
    server.on("/api/files/upload", HTTP_OPTIONS, handleOptions);
    server.on("/api/files/download", HTTP_OPTIONS, handleOptions);
    server.on("/api/stats", HTTP_OPTIONS, handleOptions);
    server.on("/api/settings", HTTP_OPTIONS, handleOptions);
    server.on("/api/device", HTTP_OPTIONS, handleOptions);

    server.onNotFound(handleNotFound);
    server.begin();
    webServerRunning = true;

    Serial.printf("[Web] Server started: http://%s\n", WiFi.localIP().toString().c_str());
}

/**
 * Web 服务器循环处理
 *
 * 非阻塞地处理待处理的 HTTP 请求。在主 loop() 中每次迭代调用。
 */
void handleWebServer() {
    if (webServerRunning) {
        server.handleClient();
    }
}
