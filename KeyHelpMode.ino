// ========== 按键帮助页面 ==========

const int helpTotalPages = 3;

const char* helpTitles[] = { "通用", "学习模式", "听写模式" };

std::vector<std::vector<String>> helpPage0 = {
    { "ESC(`)",  "打开/关闭菜单" },
    { "; / .",   "音量加/减" },
    { "Fn",      "播放发音" },
    { ", / /",   "翻页(左/右)" },
};

std::vector<std::vector<String>> helpPage1 = {
    { "BtnA",    "显示/隐藏释义" },
    { "Enter",   "记住(score+1)" },
    { "Del",     "不熟(score-1)" },
};

std::vector<std::vector<String>> helpPage2 = {
    { "字母键",  "输入答案" },
    { "Enter",   "提交答案" },
    { "Del",     "删除字符" },
    { "Shift",   "平/片假名切换" },
    { ";",       "确认当前假名" },
};

void drawKeyHelpPage(int page) {
    canvas.fillSprite(BLACK);
    canvas.setTextFont(&fonts::efontCN_16);

    drawTopLeftString(canvas, helpTitles[page]);
    drawTopRightString(canvas, String(page + 1) + "/" + String(helpTotalPages));

    std::vector<String> headers = { "按键", "功能" };
    std::vector<int> colXs = { 8, 90 };

    std::vector<std::vector<String>>* rows;
    if (page == 0)      rows = &helpPage0;
    else if (page == 1) rows = &helpPage1;
    else                rows = &helpPage2;

    drawSimpleTable(canvas, 28, 18, headers, colXs, 1.0, 1.0, *rows);

    canvas.pushSprite(0, 0);
}

void showKeyHelp() {
    int helpPage = 0;
    drawKeyHelpPage(helpPage);

    while (true) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto st = M5Cardputer.Keyboard.keysState();
            userAction = true;

            bool nav = false;
            for (auto c : st.word) {
                if (c == '`') return;
                if (c == ',') {
                    helpPage = (helpPage - 1 + helpTotalPages) % helpTotalPages;
                    nav = true;
                }
                if (c == '/') {
                    helpPage = (helpPage + 1) % helpTotalPages;
                    nav = true;
                }
            }

            if (nav) {
                drawKeyHelpPage(helpPage);
            }
        }
        delay(30);
    }
}
