/**
 * @file WiFiUtils.ino
 * @brief WiFi 连接与时间同步工具
 *
 * 提供从 SD 卡读取 WiFi 凭据并自动连接的功能，
 * 以及通过 NTP 获取格式化时间字符串的功能。
 * WiFi 连接成功后会自动同步 NTP 时间（UTC+8）。
 */

#include <WiFi.h>
#include <time.h>

/**
 * 从 SD 卡读取 WiFi 凭据并连接网络
 *
 * 读取 /words_study/.env 文件，解析其中的 WIFI_SSID 和 WIFI_PASS 字段，
 * 然后尝试连接 WiFi。连接超时时间为 8 秒。
 * 连接成功后会自动配置 NTP 时间同步（UTC+8 时区）。
 *
 * .env 文件格式示例：
 *   WIFI_SSID=MyNetwork
 *   WIFI_PASS=MyPassword
 */
void connectWiFiFromEnv() {
    File envFile = SD.open("/words_study/.env");
    if (!envFile) {
        Serial.println("[WiFi] .env 文件未找到，跳过WiFi连接");
        return;
    }

    String ssid = "";
    String pass = "";

    while (envFile.available()) {
        String line = envFile.readStringUntil('\n');
        line.trim();
        if (line.startsWith("WIFI_SSID=")) {
            ssid = line.substring(10);
            ssid.trim();
        } else if (line.startsWith("WIFI_PASS=")) {
            pass = line.substring(10);
            pass.trim();
        }
    }
    envFile.close();

    if (ssid.isEmpty()) {
        Serial.println("[WiFi] WIFI_SSID 未设置，跳过");
        return;
    }

    Serial.printf("[WiFi] 正在连接 %s ...\n", ssid.c_str());
    M5Cardputer.Display.printf("WiFi: %s ...\n", ssid.c_str());

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());

    unsigned long startMs = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startMs < 8000) {
        delay(300);
    }

    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        Serial.printf("[WiFi] 已连接, IP: %s\n", WiFi.localIP().toString().c_str());
        M5Cardputer.Display.printf("OK: %s\n", WiFi.localIP().toString().c_str());

        // 同步 NTP 时间 (UTC+8)
        configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
        Serial.println("[WiFi] NTP 时间同步中...");
    } else {
        Serial.println("[WiFi] 连接超时，继续离线运行");
        M5Cardputer.Display.println("WiFi 连接超时");
    }
    delay(500);
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
