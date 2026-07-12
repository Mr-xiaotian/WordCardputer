/**
 * @file ModeKeyHelp.ino
 * @brief 按键帮助页面模块
 *
 * 提供分页的按键帮助说明界面，共包含三个页面：
 * 通用操作、学习模式操作和听写模式操作。
 * 用户可通过 ',' 和 '/' 键翻页浏览，按 '`' 键退出帮助。
 */

// ========== 按键帮助页面 ==========

struct HelpPageData {
    const char *section;
    std::vector<std::vector<String>> rows;
};

std::vector<HelpPageData> helpPages = {
    {
        "通用",
        {
            { "ESC(`)",  "打开/关闭菜单" },
            { "; / .",   "音量加/减" },
            { "Fn",      "播放发音" },
        }
    },
    {
        "通用",
        {
            { ", / /",   "翻页(左/右)" },
        }
    },
    {
        "学习模式",
        {
            { "BtnA",    "显示/隐藏释义" },
            { "Enter",   "记住(score+1)" },
            { "Del",     "不熟(score-1)" },
        }
    },
    {
        "听写模式",
        {
            { "字母键",  "输入答案" },
            { "Enter",   "提交答案" },
            { "Del",     "删除字符" },
        }
    },
    {
        "听写模式",
        {
            { "Shift",   "平/片假名切换" },
            { ";",       "确认当前假名" },
        }
    },
};

/** 帮助页面总数 */
const int helpTotalPages = 5;

int helpPage = 0;

/**
 * 绘制指定页码的按键帮助页面
 *
 * 使用表格形式渲染帮助内容，左上角显示页面名称，右上角显示子分类和页码，
 * 表格包含"按键"和"功能"两列，数据来自对应页码的帮助数组。
 *
 * @param page 要显示的页码索引（0 ~ helpTotalPages-1）
 */
void drawKeyHelpPage(int page) {
    canvas.fillSprite(BLACK);
    canvas.setTextFont(&fonts::efontCN_16);

    drawTopLeftString(canvas, "按键帮助", GREEN, 1.2);
    drawTopRightString(
        canvas,
        String(helpPages[page].section) + " " + String(page + 1) + "/" + String(helpTotalPages),
        TFT_DARKGREY,
        1.0
    );

    std::vector<String> headers = { "按键", "功能" };
    drawSimpleTable(canvas, headers, helpPages[page].rows);

    canvas.pushSprite(0, 0);
}

/**
 * 初始化按键帮助模式
 *
 * 将页码重置为第一页并绘制帮助页面。
 */
void initKeyHelpMode() {
    helpPage = 0;
    drawKeyHelpPage(helpPage);
}

/**
 * 按键帮助模式的主循环函数
 *
 * 处理以下键盘操作：
 * - '`' 键（ESC）：返回 ESC 菜单
 * - ',' 键：切换到上一页（循环翻页）
 * - '/' 键：切换到下一页（循环翻页）
 */
void loopKeyHelpMode() {
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        auto st = M5Cardputer.Keyboard.keysState();
        userAction = true;

        bool nav = false;
        for (auto c : st.word) {
            if (c == '`') {
                appMode = MODE_ESC_MENU;
                initEscMenuMode();
                return;
            }
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
}
