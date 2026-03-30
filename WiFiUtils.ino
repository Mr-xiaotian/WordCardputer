#include <WiFi.h>
#include <time.h>

// 从 SD 卡 /words_study/.env 读取 WIFI_SSID 和 WIFI_PASS 并连接
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

// 获取当前时间字符串，格式: "26-03-30_16-28"（年-月-日_时-分）
// 如果未联网或时间未同步，返回 millis() 作为 fallback
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
