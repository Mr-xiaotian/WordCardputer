/**
 * @file ModeWiFiScan.ino
 * @brief WiFi 扫描与连接模式
 *
 * 提供 WiFi 网络扫描、选择和连接功能。
 * 扫描附近的 WiFi 网络并以列表形式展示，用户选择 SSID 后
 * 通过密码输入覆盖层完成连接。连接成功后自动启动 Web 控制台
 * 并同步 NTP 时间。
 */

#include <WiFi.h>

enum WiFiScanState {
    WIFI_SCANNING,
    WIFI_LIST,
    WIFI_PASSWORD,
    WIFI_CONNECTING,
    WIFI_RESULT,
};

WiFiScanState wifiScanState = WIFI_SCANNING;

std::vector<String> wifiSSIDs;       // 显示用（含信号指示）
std::vector<String> wifiRawSSIDs;    // 纯 SSID（连接用）
int wifiListIndex = 0;
int wifiListScroll = 0;

String wifiSelectedSSID = "";
String wifiPasswordInput = "";

bool wifiConnectSuccess = false;
String wifiResultMessage = "";

/**
 * 绘制扫描中提示
 */
void drawWiFiScanning() {
    drawCenterMessage(canvas, "扫描中...", CYAN);
}

/**
 * 绘制 WiFi 网络列表
 */
void drawWiFiList() {
    drawTextMenu(
        canvas,
        "WiFi 网络",
        wifiSSIDs,
        wifiListIndex,
        wifiListScroll,
        visibleLines,
        "未找到网络"
    );
}

/**
 * 绘制密码输入覆盖层
 *
 * 先绘制 WiFi 列表作为背景，然后在下半部分覆盖密码输入界面。
 * 显示选中的 SSID、密码（星号遮盖）和操作提示。
 */
void drawPasswordOverlay() {
    // 先画背景列表
    drawWiFiList();

    // 覆盖下半部分
    canvas.fillRect(0, 60, canvas.width(), canvas.height() - 60, BLACK);
    canvas.drawLine(4, 62, canvas.width() - 4, 62, TFT_DARKGREY);

    canvas.setTextFont(&fonts::efontCN_16);
    canvas.setTextDatum(top_left);

    // SSID
    canvas.setTextColor(CYAN);
    canvas.setTextSize(1.0);
    canvas.drawString(wifiSelectedSSID, 8, 70);

    // 密码标签
    canvas.setTextColor(TFT_DARKGREY);
    canvas.drawString("密码:", 8, 94);

    // 密码内容（星号遮盖 + 光标）
    canvas.setTextColor(WHITE);
    String masked = "";
    for (int i = 0; i < (int)wifiPasswordInput.length(); i++) {
        masked += "*";
    }
    masked += "_";
    canvas.drawString(masked, 56, 94);

    // 底部提示
    canvas.setTextDatum(bottom_center);
    canvas.setTextColor(TFT_DARKGREY);
    canvas.setTextSize(0.9);
    canvas.drawString("Enter 连接    ` 返回", canvas.width() / 2, canvas.height() - 4);

    canvas.pushSprite(0, 0);
}

/**
 * 绘制连接结果页面
 *
 * 成功时显示 IP 地址和 Web 控制台启动信息，
 * 失败时显示错误码。按任意键返回菜单。
 */
void drawWiFiResult() {
    canvas.fillSprite(BLACK);
    canvas.setTextFont(&fonts::efontCN_16);
    canvas.setTextDatum(middle_center);

    if (wifiConnectSuccess) {
        canvas.setTextColor(GREEN);
        canvas.setTextSize(1.2);
        canvas.drawString("连接成功!", canvas.width() / 2, canvas.height() / 2 - 30);

        canvas.setTextColor(CYAN);
        canvas.setTextSize(1.0);
        canvas.drawString("IP: " + WiFi.localIP().toString(), canvas.width() / 2, canvas.height() / 2);

        canvas.setTextColor(TFT_DARKGREY);
        canvas.setTextSize(0.9);
        canvas.drawString("Web 控制台已启动", canvas.width() / 2, canvas.height() / 2 + 25);
    } else {
        canvas.setTextColor(RED);
        canvas.setTextSize(1.2);
        canvas.drawString("连接失败", canvas.width() / 2, canvas.height() / 2 - 15);

        canvas.setTextColor(TFT_DARKGREY);
        canvas.setTextSize(0.9);
        canvas.drawString(wifiResultMessage, canvas.width() / 2, canvas.height() / 2 + 15);
    }

    canvas.setTextDatum(bottom_center);
    canvas.setTextColor(TFT_DARKGREY);
    canvas.setTextSize(0.9);
    canvas.drawString("按任意键返回", canvas.width() / 2, canvas.height() - 6);

    canvas.pushSprite(0, 0);
}

/**
 * 初始化 WiFi 扫描模式
 *
 * 重置所有状态变量，显示扫描提示，执行阻塞式网络扫描，
 * 然后显示扫描结果列表。
 */
void initWiFiScanMode() {
    wifiSSIDs.clear();
    wifiRawSSIDs.clear();
    wifiListIndex = 0;
    wifiListScroll = 0;
    wifiPasswordInput = "";
    wifiSelectedSSID = "";
    wifiConnectSuccess = false;
    wifiResultMessage = "";

    wifiScanState = WIFI_SCANNING;
    drawWiFiScanning();

    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
    delay(100);

    int count = WiFi.scanNetworks();
    processWiFiScanResults(count);
}

/**
 * WiFi 扫描模式的主循环函数
 *
 * 根据当前子状态分发键盘输入：
 * - LIST：浏览和选择网络
 * - PASSWORD：输入密码
 * - RESULT：任意键返回菜单
 */
void loopWiFiScanMode() {
    if (!(M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()))
        return;

    auto st = M5Cardputer.Keyboard.keysState();
    userAction = true;

    // --- 结果页：任意键返回菜单 ---
    if (wifiScanState == WIFI_RESULT) {
        appMode = MODE_ESC_MENU;
        initEscMenuMode();
        return;
    }

    // --- 网络列表 ---
    if (wifiScanState == WIFI_LIST) {
        for (auto c : st.word) {
            if (c == '`') {
                appMode = MODE_ESC_MENU;
                initEscMenuMode();
                return;
            }
            if (c == ';') {
                navigateMenu(wifiListIndex, wifiListScroll,
                             wifiSSIDs.size(), visibleLines, true);
                drawWiFiList();
            }
            if (c == '.') {
                navigateMenu(wifiListIndex, wifiListScroll,
                             wifiSSIDs.size(), visibleLines, false);
                drawWiFiList();
            }
        }

        if (st.enter && !wifiSSIDs.empty()) {
            wifiSelectedSSID = wifiRawSSIDs[wifiListIndex];
            wifiPasswordInput = "";
            wifiScanState = WIFI_PASSWORD;
            drawPasswordOverlay();
        }
        return;
    }

    // --- 密码输入 ---
    if (wifiScanState == WIFI_PASSWORD) {
        for (auto c : st.word) {
            if (c == '`') {
                wifiScanState = WIFI_LIST;
                drawWiFiList();
                return;
            }
            if (c >= ' ' && c <= '~') {
                wifiPasswordInput += c;
                drawPasswordOverlay();
            }
        }

        if (st.del && wifiPasswordInput.length() > 0) {
            wifiPasswordInput.remove(wifiPasswordInput.length() - 1);
            drawPasswordOverlay();
        }

        if (st.enter) {
            attemptWiFiConnect();
        }
        return;
    }
}
