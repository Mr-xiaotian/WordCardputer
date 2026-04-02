/**
 * @file UtilsWiFi.ino
 * @brief WiFi 连接与时间同步工具
 *
 * 提供从 SD 卡读取 WiFi 凭据并自动连接的功能，
 * 以及通过 NTP 获取格式化时间字符串的功能。
 * WiFi 连接成功后会自动同步 NTP 时间（UTC+8）。
 */

#include <WiFi.h>
#include <time.h>

// ---------- WiFi 凭据持久化 ----------
struct WiFiCredential {
    String ssid;
    String pass;
};
std::vector<WiFiCredential> savedWiFiList;

/**
 * 从 SD 卡加载已保存的 WiFi 凭据
 *
 * 读取 /words_study/wifi.json，解析 JSON 数组到 savedWiFiList。
 * 文件不存在或解析失败时静默忽略。
 */
void loadSavedWiFiCredentials() {
    savedWiFiList.clear();
    File f = SD.open("/words_study/wifi.json");
    if (!f) return;

    DynamicJsonDocument doc(4096);
    if (deserializeJson(doc, f) != DeserializationError::Ok) {
        f.close();
        return;
    }
    f.close();

    JsonArray arr = doc.as<JsonArray>();
    for (JsonObject obj : arr) {
        WiFiCredential c;
        c.ssid = obj["ssid"].as<String>();
        c.pass = obj["pass"].as<String>();
        if (c.ssid.length() > 0) {
            savedWiFiList.push_back(c);
        }
    }
}

/**
 * 保存一组 WiFi 凭据到 SD 卡
 *
 * 若同名 SSID 已存在则更新密码，否则追加新条目。
 * 将完整列表序列化写回 /words_study/wifi.json。
 *
 * @param ssid 网络名称
 * @param pass 密码
 */
void saveWiFiCredential(const String &ssid, const String &pass) {
    bool found = false;
    for (auto &c : savedWiFiList) {
        if (c.ssid == ssid) {
            c.pass = pass;
            found = true;
            break;
        }
    }
    if (!found) {
        savedWiFiList.push_back({ssid, pass});
    }

    SD.remove("/words_study/wifi.json");
    File f = SD.open("/words_study/wifi.json", FILE_WRITE);
    if (!f) return;

    DynamicJsonDocument doc(4096);
    JsonArray arr = doc.to<JsonArray>();
    for (auto &c : savedWiFiList) {
        JsonObject obj = arr.createNestedObject();
        obj["ssid"] = c.ssid;
        obj["pass"] = c.pass;
    }
    serializeJson(doc, f);
    f.close();
}

/**
 * 查找已保存的 WiFi 密码
 *
 * @param ssid 要查找的网络名称
 * @return 已保存的密码，未找到返回空字符串
 */
String findSavedPassword(const String &ssid) {
    for (auto &c : savedWiFiList) {
        if (c.ssid == ssid) return c.pass;
    }
    return "";
}

/**
 * 获取当前 NTP 时间的格式化字符串
 *
 * 返回格式为 "YY-MM-DD_HH-MM"（如 "26-03-30_16-28"）的时间字符串。
 * 如果 WiFi 未连接或 NTP 时间未同步，则使用 millis() 的值作为回退，
 * 返回开机以来的毫秒数字符串。
 *
 * @return 格式化的时间字符串，或 millis() 毫秒数字符串
 */
String getNtpTimeString() {
    struct tm t;
    if (wifiConnected && getLocalTime(&t, 100)) {
        char buf[20];
        snprintf(buf, sizeof(buf), "%02d-%02d-%02d_%02d-%02d",
                 t.tm_year % 100, t.tm_mon + 1, t.tm_mday,
                 t.tm_hour, t.tm_min);
        return String(buf);
    }
    return String(millis());
}

/**
 * 将 RSSI 信号强度转为可视指示符
 *
 * @param rssi 信号强度值（负数，越大越强）
 * @return 信号指示字符串
 */
String rssiIndicator(int rssi) {
    if (rssi > -50) return "[###]";
    if (rssi > -70) return "[## ]";
    return "[#  ]";
}

/**
 * 处理扫描结果，去重并按信号强度排序
 *
 * @param count WiFi.scanNetworks() 返回的网络数量
 */
void processWiFiScanResults(int count) {
    wifiSSIDs.clear();
    wifiRawSSIDs.clear();

    if (count <= 0) {
        wifiScanState = WIFI_LIST;
        drawWiFiList();
        return;
    }

    // 收集并去重
    struct WifiEntry {
        String ssid;
        int rssi;
    };
    std::vector<WifiEntry> entries;

    for (int i = 0; i < count; i++) {
        String ssid = WiFi.SSID(i);
        if (ssid.length() == 0) continue;

        // 去重：保留信号最强的
        bool dup = false;
        for (auto &e : entries) {
            if (e.ssid == ssid) {
                if (WiFi.RSSI(i) > e.rssi) e.rssi = WiFi.RSSI(i);
                dup = true;
                break;
            }
        }
        if (!dup) {
            entries.push_back({ssid, WiFi.RSSI(i)});
        }
    }

    // 按信号强度排序
    std::sort(entries.begin(), entries.end(), [](const WifiEntry &a, const WifiEntry &b) {
        return a.rssi > b.rssi;
    });

    // 构建显示列表
    for (auto &e : entries) {
        wifiRawSSIDs.push_back(e.ssid);
        String label = "";
        if (findSavedPassword(e.ssid).length() > 0) label += "★ ";
        label += e.ssid + " " + rssiIndicator(e.rssi);
        wifiSSIDs.push_back(label);
    }

    WiFi.scanDelete();

    wifiListIndex = 0;
    wifiListScroll = 0;
    wifiScanState = WIFI_LIST;
    drawWiFiList();
}

/**
 * 执行 WiFi 连接
 *
 * 使用选中的 SSID 和输入的密码尝试连接，超时 10 秒。
 * 连接成功后自动同步 NTP 时间、启用省电、保存凭据并启动 Web 控制台。
 * 连接失败时显示错误提示后返回列表。
 * 调用者负责处理成功后的页面跳转。
 */
void attemptWiFiConnect() {
    drawCenterString(canvas, "连接中...", YELLOW, 1.2);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
    delay(100);
    WiFi.begin(wifiSelectedSSID.c_str(), wifiPasswordInput.c_str());

    unsigned long startMs = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startMs < 10000) {
        delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        wifiConnectSuccess = true;

        // NTP 时间同步
        configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");

        // 启用 Modem Sleep 省电
        WiFi.setSleep(WIFI_PS_MIN_MODEM);

        // 保存凭据到 SD 卡
        saveWiFiCredential(wifiSelectedSSID, wifiPasswordInput);

        // 启动 Web 控制台
        if (!webServerRunning) {
            initWebServer();
        }
    } else {
        wifiConnectSuccess = false;

        // 显示失败提示后返回列表
        drawCenterString(canvas, "连接失败 (" + String(WiFi.status()) + ")", RED, 1.2);
        delay(1500);
        wifiScanState = WIFI_LIST;
        drawWiFiList();
    }
}
