/**
 * @file ModeKeyHelp.cpp
 * @brief 按键帮助页面模块
 *
 * 提供按"分类 + 分类内分页"组织的帮助说明界面。
 * 用户可通过 ';' 和 '.' 切换不同分类，通过 ',' 和 '/'
 * 在当前分类内翻页，按 '`' 键退出帮助。
 */

#include "globals.h"

// ========== 按键帮助页面 ==========

// ===== 内部辅助函数 =====

/**
 * 获取按键帮助中的分类总数。
 *
 * @return 当前帮助数据源中的分类数量
 */
static int helpSectionCount()
{
    return (int)helpSections.size();
}

/**
 * 获取当前分类下的总页数。
 *
 * @return 当前分类的分页数量
 */
static int helpPageCountForCurrentSection()
{
    return (int)helpSections[helpSectionIndex].pages.size();
}

/**
 * 绘制当前按键帮助页面
 *
 * 左上角显示页面名称，右上角显示"分类 + 分类内页码"，
 * 例如"通用1/1""听写模式2/2"。
 */
static void drawKeyHelpPage() {
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

// ===== 核心函数（init / loop） =====

/**
 * 初始化按键帮助模式
 *
 * 默认进入"按键帮助"分类第一页。
 */
void initKeyHelpMode() {
    helpSectionIndex = 0;
    helpPageIndex = 0;
    drawKeyHelpPage();
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
