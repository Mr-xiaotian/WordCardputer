// ========== ESC 菜单页面 ==========

M5Canvas escCanvas(&M5Cardputer.Display);

String selectedFilePath = "";

std::vector<String> escItems = {
    "保存进度（写入JSON）",
    "重新选择词库",
    "进入听写模式(building)",
    "返回学习页面"
};

int escIndex = 0;
int escScoll = 0;

void initEscMenuMode() {
    escIndex = 0;

    escCanvas.createSprite(
        M5Cardputer.Display.width(), 
        M5Cardputer.Display.height()
    );
    escCanvas.setTextFont(&fonts::efontCN_16);
    escCanvas.setTextSize(1.2);

    drawEscMenu();
}

// ---------- 绘制 ESC 菜单 ----------
void drawEscMenu() {
    // 直接复用通用菜单绘制！
    drawTextMenu(
        escCanvas,
        "菜单",        // 标题
        escItems,      // 项目
        escIndex,      // 当前选中
        escScoll,      
        visibleLines   
    );
}

// ---------- ESC 菜单逻辑 ----------
void loopEscMenuMode() {
    M5Cardputer.update();

    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        auto st = M5Cardputer.Keyboard.keysState();

        for (auto c : st.word) {
            if (c == ';') {  // 上
                escIndex = (escIndex - 1 + escItems.size()) % escItems.size();
                if (escIndex == escItems.size() - 1)
                {
                    // ✅ 从第一行上翻到最后一行
                    escScoll = max(0, (int)escItems.size() - visibleLines);
                }
                else if (escIndex < escScoll)
                {
                    escScoll = escIndex;
                }
                drawEscMenu();
            }
            if (c == '.') {  // 下
                escIndex = (escIndex + 1) % escItems.size();
                if (escIndex == 0)
                {
                    // ✅ 从最后一行下翻回到第一行
                    escScoll = 0;
                }
                else if (escIndex >= escScoll + visibleLines)
                {
                    escScoll = escIndex - visibleLines + 1;
                }
                drawEscMenu();
            }
        }

        if (st.enter) {
            if (escIndex == 0) {
                // 保存到 JSON
                if (saveWordsToJSON(selectedFilePath)) {
                    // 显示保存成功
                    escCanvas.fillSprite(BLACK);
                    escCanvas.setTextColor(GREEN);
                    escCanvas.drawString("保存成功！", escCanvas.width()/2, escCanvas.height()/2);
                    escCanvas.pushSprite(0, 0);
                    delay(600);
                } else {
                    escCanvas.fillSprite(BLACK);
                    escCanvas.setTextColor(RED);
                    escCanvas.drawString("保存失败！", escCanvas.width()/2, escCanvas.height()/2);
                    escCanvas.pushSprite(0, 0);
                    delay(800);
                }
                // 保存后仍停留在 ESC 菜单
                drawEscMenu();
                return;
            }
            if (escIndex == 1) {
                // 进入词库选择
                appMode = MODE_FILE_SELECT;
                initFileSelectMode();
                return;
            }
            else if (escIndex == 2) {
                // 进入听写模式（占位）
                // appMode = MODE_DICTATION;
                // initDictationMode();  // 未来实现
                // return;
            }
            else if (escIndex == 3) {
                // 返回学习页面
                appMode = MODE_STUDY;
                drawWord();  // 刷新学习界面
                return;
            }
        }
    }
}
