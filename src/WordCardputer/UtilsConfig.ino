/**
 * UtilsConfig.ino - 设备配置持久化
 *
 * 统一管理 SD 卡上的 `/words_study/config.json`，包括：
 * - WiFi 凭据列表
 * - 音量与亮度
 * - 默认语言
 * - 自动保存阈值
 * - 自动熄屏超时与暗屏亮度
 *
 * 同时兼容旧版 `/words_study/wifi.json`，首次读取时会自动迁移。
 */

static const char *kConfigFilePath = "/words_study/config.json";

/**
 * 从 JSON 数组读取 WiFi 凭据
 *
 * @param arr WiFi 数组
 */
void loadWiFiArray(JsonArray arr) {
    savedWiFiList.clear();
    for (JsonObject obj : arr) {
        WiFiCredential c;
        c.ssid = obj["ssid"].as<String>();
        c.pass = obj["pass"].as<String>();
        if (!c.ssid.isEmpty()) {
            savedWiFiList.push_back(c);
        }
    }
}

/**
 * 读取旧版 wifi.json 并迁移到当前内存结构
 *
 * @return true 读取到旧数据；false 未找到或解析失败
 */
bool loadLegacyWiFiConfig() {
    File f = SD.open("/words_study/wifi.json");
    if (!f) {
        return false;
    }

    DynamicJsonDocument doc(4096);
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err || !doc.is<JsonArray>()) {
        return false;
    }

    loadWiFiArray(doc.as<JsonArray>());
    return true;
}

/**
 * 保存当前配置到 config.json
 *
 * 配置采用统一对象结构：
 * {
 *   "version": 1,
 *   "settings": {...},
 *   "wifi": [...]
 * }
 *
 * @return true 保存成功；false 写文件失败
 */
bool saveAppConfig() {
    DynamicJsonDocument doc(8192);
    doc["version"] = 1;

    JsonObject settings = doc.createNestedObject("settings");
    settings["volume"] = soundVolume;
    settings["language"] = (currentLanguage == LANG_JP) ? "jp" : "en";
    settings["brightness"] = normalBrightness;
    settings["dim_brightness"] = dimBrightness;
    settings["idle_timeout_ms"] = idleTimeout;
    settings["auto_save_threshold"] = autoSaveThreshold;

    JsonArray wifi = doc.createNestedArray("wifi");
    for (auto &c : savedWiFiList) {
        JsonObject item = wifi.createNestedObject();
        item["ssid"] = c.ssid;
        item["pass"] = c.pass;
    }

    SD.remove(kConfigFilePath);
    File f = SD.open(kConfigFilePath, FILE_WRITE);
    if (!f) {
        return false;
    }
    serializeJson(doc, f);
    f.close();

    // 写入新配置后清理旧文件，避免继续分叉
    if (SD.exists("/words_study/wifi.json")) {
        SD.remove("/words_study/wifi.json");
    }
    return true;
}

/**
 * 从 config.json 加载配置
 *
 * 若 config.json 不存在，则尝试迁移旧版 wifi.json。
 * 仅在以下情况会回写统一配置文件：
 * - `config.json` 不存在或解析失败，需要用默认值/迁移值重建
 * - 仍检测到旧版 `wifi.json`，需要在迁移后收敛到 `config.json`
 */
void loadAppConfig() {
    bool loaded = false;
    File f = SD.open(kConfigFilePath);
    if (f) {
        DynamicJsonDocument doc(8192);
        DeserializationError err = deserializeJson(doc, f);
        f.close();
        if (!err && doc.is<JsonObject>()) {
            JsonObject root = doc.as<JsonObject>();
            JsonObject settings = root["settings"];
            if (!settings.isNull()) {
                soundVolume = constrain(settings["volume"] | soundVolume, 0, 255);

                String lang = settings["language"] | String((currentLanguage == LANG_JP) ? "jp" : "en");
                currentLanguage = (lang == "en") ? LANG_EN : LANG_JP;

                normalBrightness = constrain(settings["brightness"] | normalBrightness, 10, 255);
                dimBrightness = constrain(settings["dim_brightness"] | dimBrightness, 1, 255);
                dimBrightness = min(dimBrightness, normalBrightness);

                idleTimeout = max<unsigned long>(1000UL, settings["idle_timeout_ms"] | idleTimeout);
                autoSaveThreshold = max(1, settings["auto_save_threshold"] | autoSaveThreshold);
            }

            JsonArray wifi = root["wifi"];
            if (!wifi.isNull()) {
                loadWiFiArray(wifi);
            } else {
                savedWiFiList.clear();
            }
            loaded = true;
        }
    }

    if (!loaded) {
        if (!loadLegacyWiFiConfig()) {
            savedWiFiList.clear();
        }
        saveAppConfig();
    } else if (SD.exists("/words_study/wifi.json")) {
        saveAppConfig();
    }
}
