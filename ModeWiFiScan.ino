/**
 * @file ModeWiFiScan.ino
 * @brief WiFi 扫描与连接模式
 *
 * 提供 WiFi 网络扫描、选择和连接功能。
 * 扫描附近的 WiFi 网络并以列表形式展示，用户选择 SSID 后
 * 通过密码输入覆盖层完成连接。连接成功后自动启动 Web 控制台
 * 并同步 NTP 时间。WiFi 已连接时显示双页界面（列表 + 状态页），
 * 可通过 ,/. 翻页随时查看 IP 地址。
 */

#include <WiFi.h>

enum WiFiScanState {
    WIFI_SCANNING,
    WIFI_LIST,
    WIFI_PASSWORD,
    WIFI_CONNECTING,
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

int wifiPage = 0;  // 0=列表页, 1=状态页（仅 WiFi 已连接时可用）

/**
 * 绘制扫描中提示
 */
void drawWiFiScanning() {
    drawCenterString(canvas, "扫描中...", CYAN, 1.2);
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
 * 绘制密码输入界面。
 * 显示选中的 SSID、密码（星号遮盖）和操作提示。
 */
void drawPasswordOverlay() {
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
    canvas.setTextSize(1.0);
    canvas.drawString("Enter 连接    ` 返回", canvas.width() / 2, canvas.height() - 4);

    canvas.pushSprite(0, 0);
}

/**
 * 绘制 WiFi 状态页
 *
 * 显示当前 WiFi 连接信息：SSID、IP 地址和 Web 控制台状态。
 * 底部提示可通过翻页返回网络列表。
 */
void drawWiFiStatusPage() {
    canvas.fillSprite(BLACK);
    canvas.setTextFont(&fonts::efontCN_16);

    drawTopLeftString(canvas, "WiFi 状态", GREEN, 1.2);

    canvas.setTextDatum(middle_center);

    canvas.setTextColor(CYAN);
    canvas.setTextSize(1.2);
    canvas.drawString(WiFi.SSID(), canvas.width() / 2, canvas.height() / 2 - 25);

    canvas.setTextColor(WHITE);
    canvas.setTextSize(1.0);
    canvas.drawString("IP: " + WiFi.localIP().toString(), canvas.width() / 2, canvas.height() / 2 + 5);

    if (webServerRunning) {
        canvas.setTextColor(GREEN);
        canvas.setTextSize(1.0);
        canvas.drawString("Web 控制台运行中", canvas.width() / 2, canvas.height() / 2 + 30);
    }

    // 底部页码
    canvas.setTextDatum(bottom_center);
    canvas.setTextColor(TFT_DARKGREY);
    canvas.setTextSize(1.0);
    canvas.drawString("2/2", canvas.width() / 2, canvas.height() - 6);

    canvas.pushSprite(0, 0);
}

/**
 * 初始化 WiFi 扫描模式
 *
 * 若 WiFi 已连接，加载凭据并扫描网络后跳到状态页。
 * 否则正常扫描并显示网络列表。
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

    loadSavedWiFiCredentials();

    if (wifiConnected) {
        // WiFi 已连接：后台扫描填充列表，直接显示状态页
        wifiScanState = WIFI_LIST;
        wifiPage = 1;
        drawWiFiStatusPage();

        // 异步扫描填充列表供翻页使用
        int count = WiFi.scanNetworks(false, false, false, 300);
        processWiFiScanResults(count);
        wifiPage = 1;  // processWiFiScanResults 会设为 WIFI_LIST，保持在状态页
    } else {
        // 未连接：正常扫描流程
        wifiPage = 0;
        wifiScanState = WIFI_SCANNING;
        drawWiFiScanning();

        WiFi.mode(WIFI_STA);
        WiFi.disconnect(true);
        delay(100);

        int count = WiFi.scanNetworks();
        processWiFiScanResults(count);
    }
}

/**
 * WiFi 扫描模式的主循环函数
 *
 * 根据当前子状态分发键盘输入：
 * - LIST（page 0）：浏览和选择网络，WiFi 已连接时可翻到状态页
 * - LIST（page 1）：状态页，可翻回列表页
 * - PASSWORD：输入密码
 */
void loopWiFiScanMode() {
    if (!(M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()))
        return;

    auto st = M5Cardputer.Keyboard.keysState();
    userAction = true;

    // --- 状态页（page 1） ---
    if (wifiScanState == WIFI_LIST && wifiPage == 1) {
        for (auto c : st.word) {
            if (c == '`') {
                appMode = MODE_ESC_MENU;
                initEscMenuMode();
                return;
            }
            if (c == ',' || c == '/') {
                wifiPage = 0;
                drawWiFiList();
                return;
            }
        }
        return;
    }

    // --- 网络列表（page 0） ---
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
            if (c == ',' || c == '/') {
                if (wifiConnected) {
                    wifiPage = 1;
                    drawWiFiStatusPage();
                    return;
                }
            }
        }

        if (st.enter && !wifiSSIDs.empty()) {
            wifiSelectedSSID = wifiRawSSIDs[wifiListIndex];
            String savedPass = findSavedPassword(wifiSelectedSSID);
            if (savedPass.length() > 0) {
                wifiPasswordInput = savedPass;
                attemptWiFiConnect();
                if (wifiConnectSuccess) {
                    wifiPage = 1;
                    wifiScanState = WIFI_LIST;
                    drawWiFiStatusPage();
                }
            } else {
                wifiPasswordInput = "";
                wifiScanState = WIFI_PASSWORD;
                drawPasswordOverlay();
            }
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
            if (wifiConnectSuccess) {
                wifiPage = 1;
                wifiScanState = WIFI_LIST;
                drawWiFiStatusPage();
            }
        }
        return;
    }
}
