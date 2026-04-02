/**
 * @file ModeEscMenu.ino
 * @brief ESC 菜单页面
 *
 * 提供应用内全局菜单功能，用户可通过 ESC 键（`）随时呼出。
 * 菜单选项包括：保存进度、学习统计、重新选择词库/语言、
 * 切换学习/听读/听写模式、查看按键帮助等。
 */

// ========== ESC 菜单页面 ==========

std::vector<String> escItems = {
    "保存进度",
    "学习统计",
    "重新选择词库",
    "重新选择语言",
    "进入学习模式",
    "进入听读模式",
    "进入听写模式",
    "按键帮助",
    "Web 控制台",
};

int escIndex = 0;
int escScoll = 0;

/**
 * 初始化 ESC 菜单模式
 *
 * 将菜单光标和滚动位置重置为起始状态，然后绘制菜单界面。
 */
void initEscMenuMode() {
    escIndex = 0;
    escScoll = 0;

    drawEscMenu();
}

/**
 * 绘制 ESC 菜单界面
 *
 * 调用通用菜单绘制函数 drawTextMenu，以列表形式展示所有菜单选项，
 * 高亮当前选中项，并处理列表滚动显示。
 */
void drawEscMenu() {
    // 直接复用通用菜单绘制！
    drawTextMenu(
        canvas,
        "菜单",        // 标题
        escItems,      // 项目
        escIndex,      // 当前选中
        escScoll,      
        visibleLines   
    );
}

/**
 * ESC 菜单模式的主循环函数，处理菜单输入和功能分发
 *
 * 处理以下键盘操作：
 * - ESC 键（`）返回先前模式（自动保存进度）
 * - 分号键（;）向上移动光标，句号键（.）向下移动光标
 * - Enter 键执行选中的菜单项：
 *   0=保存进度  1=学习统计  2=重新选择词库  3=重新选择语言
 *   4=学习模式  5=听读模式  6=听写模式  7=按键帮助  8=Web控制台
 */
void loopEscMenuMode() {
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        auto st = M5Cardputer.Keyboard.keysState();
        userAction = true;

        for (auto c : st.word) {
            if (c == '`') {  // ESC 键
                autoSaveIfNeeded();
                appMode = previousMode;
                if (previousMode == MODE_STUDY) {
                    drawStudyWord();
                }
                else if (previousMode == MODE_LISTEN) {
                    initListenMode();
                }
                else if (previousMode == MODE_DICTATION) {
                    initDictationMode();
                }
                return;
            }

            if (c == ';') {  // 上
                navigateMenu(escIndex, escScoll, escItems.size(), visibleLines, true);
                drawEscMenu();
            }
            if (c == '.') {  // 下
                navigateMenu(escIndex, escScoll, escItems.size(), visibleLines, false);
                drawEscMenu();
            }
        }

        if (st.enter) {
            if (escIndex == 0) {
                // 保存到 JSON
                if (saveListToJSON(selectedFilePath, words)) {
                    scoresDirty = false;
                    dirtyCount = 0;
                    drawCenterMessage(canvas, "保存成功！", GREEN);
                    delay(600);
                } else {
                    drawCenterMessage(canvas, "保存失败！");
                    delay(800);
                }
                // 保存后仍停留在 ESC 菜单
                drawEscMenu();
                return;
            }
            else if (escIndex == 1) {
                appMode = MODE_STATS;
                initStatsMode();
                return;
            }
            else if (escIndex == 2) {
                // 进入词库选择（先自动保存）
                autoSaveIfNeeded();
                appMode = MODE_FILE_SELECT;
                initFileSelectMode();
                return;
            }
            else if (escIndex == 3) {
                // 重新选择语言（先自动保存）
                autoSaveIfNeeded();
                appMode = MODE_LANG_SELECT;
                initLanguageSelectMode();
                return;
            }
            else if (escIndex == 4) {
                // 进入学习页面
                appMode = MODE_STUDY;
                drawStudyWord();
                return;
            }
            else if (escIndex == 5) {
                // 进入听读模式
                appMode = MODE_LISTEN;
                initListenMode();
                return;
            }
            else if (escIndex == 6) {
                // 进入听写模式
                appMode = MODE_DICTATION;
                initDictationMode();
                return;
            }
            else if (escIndex == 7) {
                // 按键帮助
                showKeyHelp();
                drawEscMenu();
                return;
            }
            else if (escIndex == 8) {
                // Web 控制台 —— 显示 IP 地址
                canvas.fillSprite(BLACK);
                canvas.setTextFont(&fonts::efontCN_16);
                canvas.setTextDatum(middle_center);
                if (wifiConnected) {
                    canvas.setTextColor(GREEN);
                    canvas.setTextSize(1.2);
                    canvas.drawString("Web 控制台", canvas.width() / 2, canvas.height() / 2 - 28);
                    canvas.setTextColor(CYAN);
                    canvas.setTextSize(1.0);
                    String url = "http://" + WiFi.localIP().toString();
                    canvas.drawString(url, canvas.width() / 2, canvas.height() / 2 + 8);
                    canvas.setTextColor(TFT_DARKGREY);
                    canvas.setTextSize(0.9);
                    canvas.drawString("在浏览器中打开以上地址", canvas.width() / 2, canvas.height() / 2 + 38);
                } else {
                    canvas.setTextColor(RED);
                    canvas.setTextSize(1.2);
                    canvas.drawString("WiFi 未连接", canvas.width() / 2, canvas.height() / 2);
                }
                canvas.pushSprite(0, 0);
                // 等待任意键返回
                while (true) {
                    M5Cardputer.update();
                    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
                        userAction = true;
                        break;
                    }
                    delay(30);
                }
                drawEscMenu();
                return;
            }
        }
    }
}
