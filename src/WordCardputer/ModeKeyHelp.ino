/**
 * @file ModeKeyHelp.ino
 * @brief 按键帮助页面模块
 *
 * 提供按“分类 + 分类内分页”组织的帮助说明界面。
 * 用户可通过 ';' 和 '.' 切换不同分类，通过 ',' 和 '/'
 * 在当前分类内翻页，按 '`' 键退出帮助。
 */

// ========== 按键帮助页面 ==========

struct HelpSectionData {
    const char *section;
    std::vector<std::vector<std::vector<String>>> pages;
};

std::vector<HelpSectionData> helpSections = {
    {
        "按键帮助",
        {
            {
                { "; / .",   "切换帮助分类" },
                { ", / /",   "当前分类翻页" },
            }
        }
    },
    {
        "通用",
        {
            {
                { "ESC(`)",  "打开/关闭菜单" },
            }
        }
    },
    {
        "学习模式",
        {
            {
                { "BtnA",    "单词页翻卡/例句中切换中译" },
                { "Enter",   "记住(score+1)" },
                { "Del",     "不熟(score-1)" },
            },
            {
                { ", /",     "左右循环切换页面" },
                { "; / .",   "音量加/减" },
                { "Fn",      "播放发音" },
            }
        }
    },
    {
        "听写模式",
        {
            {
                { "字母键",  "输入答案" },
                { "Enter",   "提交答案" },
                { "Del",     "删除字符" },
            },
            {
                { "Shift",   "平/片假名切换" },
                { ";",       "确认当前假名" },
                { "Fn",      "重播当前发音" },
            },
            {
                { "; / .",   "音量加/减" },
                { "Fn",      "播放发音" },
            }
        }
    },
    {
        "当前词表",
        {
            {
                { "; / .",   "切换分数组" },
                { ", / /",   "当前分组翻页" },
            }
        }
    },
    {
        "错题回顾",
        {
            {
                { ", / /",   "切换错题" },
                { "Fn",      "重播正确发音" },
            }
        }
    },
};

int helpSectionIndex = 0;
int helpPageIndex = 0;

// ===== 工具函数 =====

/**
 * 获取按键帮助中的分类总数。
 *
 * @return 当前帮助数据源中的分类数量
 */
int helpSectionCount()
{
    return (int)helpSections.size();
}

/**
 * 获取当前分类下的总页数。
 *
 * @return 当前分类的分页数量
 */
int helpPageCountForCurrentSection()
{
    return (int)helpSections[helpSectionIndex].pages.size();
}

// ===== 核心函数（init / draw / loop） =====

/**
 * 初始化按键帮助模式
 *
 * 默认进入“按键帮助”分类第一页。
 */
void initKeyHelpMode() {
    helpSectionIndex = 0;
    helpPageIndex = 0;
    drawKeyHelpPage();
}

/**
 * 绘制当前按键帮助页面
 *
 * 左上角显示页面名称，右上角显示“分类 + 分类内页码”，
 * 例如“通用1/1”“听写模式2/2”。
 */
void drawKeyHelpPage() {
    canvas.fillSprite(BLACK);
    canvas.setFont(&fonts::efontCN_16);

    drawTopLeftString(canvas, "按键帮助", GREEN, 1.2);
    drawTopRightString(
        canvas,
        String(helpSections[helpSectionIndex].section) + " " +
            String(helpPageIndex + 1) + "/" + String(helpPageCountForCurrentSection()),
        TFT_DARKGREY,
        1.0
    );

    std::vector<String> headers = { "按键", "功能" };
    drawSimpleTable(canvas, headers, helpSections[helpSectionIndex].pages[helpPageIndex]);

    canvas.pushSprite(0, 0);
}

/**
 * 按键帮助模式的主循环函数
 *
 * 处理以下键盘操作：
 * - '`' 键（ESC）：返回 ESC 菜单
 * - ';' / '.'：切换分类（循环）
 * - ',' / '/'：在当前分类内翻页（循环）
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
            if (c == ';') {
                helpSectionIndex = (helpSectionIndex - 1 + helpSectionCount()) % helpSectionCount();
                helpPageIndex = 0;
                nav = true;
            }
            if (c == '.') {
                helpSectionIndex = (helpSectionIndex + 1) % helpSectionCount();
                helpPageIndex = 0;
                nav = true;
            }
            if (c == ',') {
                int pageCount = helpPageCountForCurrentSection();
                helpPageIndex = (helpPageIndex - 1 + pageCount) % pageCount;
                nav = true;
            }
            if (c == '/') {
                int pageCount = helpPageCountForCurrentSection();
                helpPageIndex = (helpPageIndex + 1) % pageCount;
                nav = true;
            }
        }

        if (nav) {
            drawKeyHelpPage();
        }
    }
}
