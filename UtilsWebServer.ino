/**
 * @file UtilsWebServer.ino
 * @brief Web 控制面板服务器
 *
 * 在 ESP32 上运行轻量级 HTTP 服务器，提供浏览器端的设备管理面板。
 * 功能包括：词库数据库浏览、JSON 导入导出、词库删除、学习统计查看、音量亮度调节。
 * 仅在 WiFi 连接成功后启动，前端页面从 SD 卡加载。
 */

#include <WebServer.h>

WebServer server(80);
bool webServerRunning = false;
File uploadFile;
String uploadTempPath = "";
String uploadTargetSource = "";
String uploadTargetChapter = "";
String uploadError = "";
int uploadImportedCount = 0;

/**
 * 将指定 source / chapter 导出为 JSON 数组
 */
bool streamWordListAsJson(const String &source, const String &chapter, const String &downloadName) {
    sqlite3 *db = nullptr;
    sqlite3_stmt *stmt = nullptr;

    if (!openVocabularyDb(&db)) {
        return false;
    }

    String sql;
    if (currentLanguage == LANG_JP) {
        sql = "SELECT jp, zh, kanji, romaji, tone, score FROM jp_words "
              "WHERE source = ?1 AND (?2 = '' OR chapter = ?2) ORDER BY id";
    } else {
        sql = "SELECT en, zh, pos, phonetic, score FROM en_words "
              "WHERE source = ?1 AND (?2 = '' OR chapter = ?2) ORDER BY id";
    }

    if (!prepareStatement(db, sql, &stmt)) {
        sqlite3_close(db);
        return false;
    }

    sqlite3_bind_text(stmt, 1, source.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, chapter.c_str(), -1, SQLITE_TRANSIENT);

    server.sendHeader("Content-Disposition", "attachment; filename=\"" + downloadName + "\"");
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "application/json", "");

    bool first = true;
    server.sendContent("[");
    while (true) {
        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            DynamicJsonDocument item(512);
            if (currentLanguage == LANG_JP) {
                item["jp"] = sqliteColumnText(stmt, 0);
                item["zh"] = sqliteColumnText(stmt, 1);
                item["kanji"] = sqliteColumnText(stmt, 2);
                item["romaji"] = sqliteColumnText(stmt, 3);
                item["tone"] = sqlite3_column_int(stmt, 4);
                item["score"] = normalizeScoreValue(sqlite3_column_int(stmt, 5));
            } else {
                item["en"] = sqliteColumnText(stmt, 0);
                item["zh"] = sqliteColumnText(stmt, 1);
                item["pos"] = sqliteColumnText(stmt, 2);
                item["phonetic"] = sqliteColumnText(stmt, 3);
                item["score"] = normalizeScoreValue(sqlite3_column_int(stmt, 4));
            }

            String line;
            serializeJson(item, line);
            if (!first) {
                server.sendContent(",");
            }
            server.sendContent(line);
            first = false;
            continue;
        }

        if (rc != SQLITE_DONE) {
            Serial.printf("[Web] 导出词库失败: %s\n", sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return false;
        }
        break;
    }

    server.sendContent("]");
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return true;
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

// ========== 词库管理 API ==========

/**
 * GET /api/files?path=<virtual-path> — 列出数据库中的 source / chapter
 */
void handleApiFiles() {
    sendCorsHeaders();
    String path = server.arg("path");
    if (path.isEmpty()) {
        path = currentWordRoot;
    }
    if (!isValidVocabPath(path)) {
        path = currentWordRoot;
    }

    DynamicJsonDocument doc(4096);
    doc["path"] = path;
    doc["root"] = currentWordRoot;
    doc["language"] = (currentLanguage == LANG_JP) ? "jp" : "en";
    JsonArray items = doc.createNestedArray("items");
    bool isRoot = false;
    String source;
    String chapter;
    if (!parseVocabPath(path, isRoot, source, chapter)) {
        sendJsonError(400, "Invalid path");
        return;
    }

    sqlite3 *db = nullptr;
    sqlite3_stmt *stmt = nullptr;
    if (!openVocabularyDb(&db)) {
        sendJsonError(500, "Open database failed");
        return;
    }

    if (isRoot) {
        String sql = String("SELECT source, COUNT(*), COUNT(DISTINCT NULLIF(chapter, '')) ")
                   + "FROM " + currentWordTable()
                   + " GROUP BY source ORDER BY source COLLATE NOCASE";
        if (!prepareStatement(db, sql, &stmt)) {
            sqlite3_close(db);
            sendJsonError(500, "Query failed");
            return;
        }

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            JsonObject item = items.createNestedObject();
            int chapterCount = sqlite3_column_int(stmt, 2);
            item["name"] = sqliteColumnText(stmt, 0);
            item["kind"] = "source";
            item["isDir"] = chapterCount > 0;
            item["wordCount"] = sqlite3_column_int(stmt, 1);
            item["chapterCount"] = chapterCount;
        }
    } else if (chapter.isEmpty()) {
        String totalSql = String("SELECT COUNT(*) FROM ") + currentWordTable() + " WHERE source = ?1";
        if (!prepareStatement(db, totalSql, &stmt)) {
            sqlite3_close(db);
            sendJsonError(500, "Query failed");
            return;
        }
        sqlite3_bind_text(stmt, 1, source.c_str(), -1, SQLITE_TRANSIENT);
        int totalCount = 0;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            totalCount = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
        stmt = nullptr;

        String chapterSql = String("SELECT chapter, COUNT(*) FROM ") + currentWordTable()
                          + " WHERE source = ?1 AND chapter <> '' GROUP BY chapter ORDER BY chapter COLLATE NOCASE";
        if (!prepareStatement(db, chapterSql, &stmt)) {
            sqlite3_close(db);
            sendJsonError(500, "Query failed");
            return;
        }
        sqlite3_bind_text(stmt, 1, source.c_str(), -1, SQLITE_TRANSIENT);

        JsonObject allItem = items.createNestedObject();
        allItem["name"] = "全部";
        allItem["kind"] = "chapter";
        allItem["isDir"] = false;
        allItem["wordCount"] = totalCount;

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            JsonObject item = items.createNestedObject();
            item["name"] = sqliteColumnText(stmt, 0);
            item["kind"] = "chapter";
            item["isDir"] = false;
            item["wordCount"] = sqlite3_column_int(stmt, 1);
        }
    }

    if (stmt) {
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);

    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

/**
 * POST /api/files/upload?path=<virtual-path> — JSON 导入完成后的响应处理
 */
void handleApiFileUpload() {
    sendCorsHeaders();
    if (!uploadError.isEmpty()) {
        sendJsonError(400, uploadError);
        return;
    }

    DynamicJsonDocument doc(256);
    doc["ok"] = true;
    doc["source"] = uploadTargetSource;
    doc["chapter"] = uploadTargetChapter;
    doc["imported"] = uploadImportedCount;
    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

/**
 * 文件上传分块数据处理
 *
 * 处理 multipart 上传的各阶段：开始时写入临时文件、结束后导入数据库。
 * 导入目标由当前虚拟路径推导：
 * - 根层上传：文件 stem 作为 source
 * - source 层上传：当前 source + 文件 stem 作为 chapter
 */
void handleApiFileUploadData() {
    HTTPUpload &upload = server.upload();
    String dir = server.arg("path");
    if (dir.isEmpty()) dir = currentWordRoot;

    if (upload.status == UPLOAD_FILE_START) {
        uploadError = "";
        uploadImportedCount = 0;
        uploadTargetSource = "";
        uploadTargetChapter = "";
        uploadTempPath = "";

        if (!deriveUploadTarget(dir, upload.filename, uploadTargetSource, uploadTargetChapter)) {
            uploadError = "无效导入位置";
            return;
        }

        uploadTempPath = "/words_study/.upload_" + String(millis()) + ".json";
        uploadFile = SD.open(uploadTempPath, FILE_WRITE);
        if (!uploadFile) {
            uploadError = "无法创建临时文件";
            return;
        }
        Serial.printf("[Web] Import start: %s -> %s/%s\n",
                      upload.filename.c_str(),
                      uploadTargetSource.c_str(),
                      uploadTargetChapter.c_str());
    }
    else if (upload.status == UPLOAD_FILE_WRITE) {
        if (uploadFile) {
            uploadFile.write(upload.buf, upload.currentSize);
        }
    }
    else if (upload.status == UPLOAD_FILE_END) {
        if (uploadFile) {
            uploadFile.close();
        }

        if (uploadError.isEmpty()) {
            if (!importJsonFileToDb(uploadTempPath, uploadTargetSource, uploadTargetChapter, uploadImportedCount, uploadError)) {
                Serial.printf("[Web] Import failed: %s\n", uploadError.c_str());
            } else {
                Serial.printf("[Web] Import done: %d words\n", uploadImportedCount);
            }
        }

        if (!uploadTempPath.isEmpty()) {
            SD.remove(uploadTempPath);
        }
    }
    else if (upload.status == UPLOAD_FILE_ABORTED) {
        if (uploadFile) {
            uploadFile.close();
        }
        if (!uploadTempPath.isEmpty()) {
            SD.remove(uploadTempPath);
        }
        uploadError = "上传已中止";
        Serial.println("[Web] Upload aborted");
    }
}

/**
 * DELETE /api/files?path=<virtual-path> — 删除 source 或 chapter
 */
void handleApiFileDelete() {
    sendCorsHeaders();
    String path = server.arg("path");
    bool isRoot = false;
    String source;
    String chapter;
    if (!parseVocabPath(path, isRoot, source, chapter) || isRoot || source.isEmpty()) {
        sendJsonError(400, "Invalid path");
        return;
    }

    sqlite3 *db = nullptr;
    sqlite3_stmt *stmt = nullptr;
    if (!openVocabularyDb(&db)) {
        sendJsonError(500, "Open database failed");
        return;
    }

    String sql = String("DELETE FROM ") + currentWordTable()
               + " WHERE source = ?1 AND (?2 = '' OR chapter = ?2)";
    if (!prepareStatement(db, sql, &stmt)) {
        sqlite3_close(db);
        sendJsonError(500, "Delete failed");
        return;
    }

    sqlite3_bind_text(stmt, 1, source.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, chapter.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        sendJsonError(500, "Delete failed");
        return;
    }

    int changes = sqlite3_changes(db);
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    DynamicJsonDocument doc(128);
    doc["ok"] = true;
    doc["deleted"] = changes;
    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

/**
 * GET /api/files/download?path=<virtual-path> — 将 source / chapter 导出为 JSON
 */
void handleApiFileDownload() {
    sendCorsHeaders();
    String path = server.arg("path");
    bool isRoot = false;
    String source;
    String chapter;
    if (!parseVocabPath(path, isRoot, source, chapter) || isRoot || source.isEmpty()) {
        sendJsonError(400, "Invalid path");
        return;
    }

    String downloadName = source;
    if (!chapter.isEmpty()) {
        downloadName += "_" + chapter;
    }
    downloadName += ".json";

    if (!streamWordListAsJson(source, chapter, downloadName)) {
        sendJsonError(500, "Export failed");
        return;
    }
}

// ========== 统计 API ==========

/**
 * GET /api/stats — 返回当前已加载词库的学习统计数据
 */
void handleApiStats() {
    sendCorsHeaders();
    computeStatsFromWords();

    DynamicJsonDocument doc(512);
    doc["file"] = statsFileName(selectedFilePath);
    doc["label"] = selectedFilePath;
    doc["source"] = selectedSource;
    doc["chapter"] = selectedChapter;
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
    doc["autoSaveThreshold"] = autoSaveThreshold;
    doc["language"] = (currentLanguage == LANG_JP) ? "jp" : "en";
    doc["loadedFile"] = statsFileName(selectedFilePath);
    doc["loadedVocab"] = selectedFilePath;
    doc["wifi"] = wifiConnected;

    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

/**
 * POST /api/settings — 修改设备设置
 *
 * 接受 JSON body，支持：
 * - "volume"(0-255)
 * - "brightness"(10-255)
 * - "autoSaveThreshold"(>=1)
 * 修改后会立即写回 `config.json`。
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
        normalBrightness = constrain(doc["brightness"].as<int>(), 10, 255);
        dimBrightness = min(dimBrightness, normalBrightness);
        if (!isDimmed) {
            M5Cardputer.Display.setBrightness(normalBrightness);
        }
    }
    if (doc.containsKey("autoSaveThreshold")) {
        autoSaveThreshold = max(1, doc["autoSaveThreshold"].as<int>());
    }
    saveAppConfig();

    // 返回当前值
    DynamicJsonDocument res(192);
    res["ok"] = true;
    res["volume"] = soundVolume;
    res["brightness"] = (int)normalBrightness;
    res["autoSaveThreshold"] = autoSaveThreshold;
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
