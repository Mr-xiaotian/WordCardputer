/**
 * @file ModeEscMenu.ino
 * @brief ESC 菜单页面
 *
 * 提供应用内全局菜单功能，用户可通过 ESC 键（`）随时呼出。
 * 菜单支持一级菜单与二级子菜单，用于收纳词库相关和模式切换相关入口。
 */

// ========== ESC 菜单页面 ==========

enum EscMenuGroup {
    ESC_MENU_ROOT = 0,
    ESC_MENU_VOCAB = 1,
    ESC_MENU_MODE = 2,
};

std::vector<String> escRootItems = {
    "学习统计",
    "当前词表",
    "词库相关 >",
    "模式切换 >",
    "查看过往错题",
    "按键帮助",
    "WiFi 连接",
};

std::vector<String> escVocabItems = {
    "当前词表",
    "重新选择词源",
    "重新选择语言",
};

std::vector<String> escModeItems = {
    "进入学习模式",
    "进入听读模式",
    "进入听写模式",
};

EscMenuGroup escMenuGroup = ESC_MENU_ROOT;
int escRootIndex = 0;
int escRootScroll = 0;
int escVocabIndex = 0;
int escVocabScroll = 0;
int escModeIndex = 0;
int escModeScroll = 0;

std::vector<String> &currentEscItems() {
    if (escMenuGroup == ESC_MENU_VOCAB) {
        return escVocabItems;
    }
    if (escMenuGroup == ESC_MENU_MODE) {
        return escModeItems;
    }
    return escRootItems;
}

int &currentEscIndex() {
    if (escMenuGroup == ESC_MENU_VOCAB) {
        return escVocabIndex;
    }
    if (escMenuGroup == ESC_MENU_MODE) {
        return escModeIndex;
    }
    return escRootIndex;
}

int &currentEscScroll() {
    if (escMenuGroup == ESC_MENU_VOCAB) {
        return escVocabScroll;
    }
    if (escMenuGroup == ESC_MENU_MODE) {
        return escModeScroll;
    }
    return escRootScroll;
}

String escMenuTitle() {
    if (escMenuGroup == ESC_MENU_VOCAB) {
        return "词库相关";
    }
    if (escMenuGroup == ESC_MENU_MODE) {
        return "模式切换";
    }
    return "菜单";
}

void returnFromEscMenu() {
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
    else if (previousMode == MODE_DICTATION_REVIEW) {
        drawDictationReviewPage();
    }
}

/**
 * 初始化 ESC 菜单模式
 *
 * 将菜单光标和滚动位置重置为起始状态，然后绘制菜单界面。
 */
void initEscMenuMode() {
    escMenuGroup = ESC_MENU_ROOT;
    escRootIndex = 0;
    escRootScroll = 0;
    escVocabIndex = 0;
    escVocabScroll = 0;
    escModeIndex = 0;
    escModeScroll = 0;

    drawEscMenu();
}

/**
 * 绘制 ESC 菜单界面
 *
 * 调用通用菜单绘制函数 drawTextMenu，以列表形式展示所有菜单选项，
 * 高亮当前选中项，并处理列表滚动显示。
 */
void drawEscMenu() {
    std::vector<String> &items = currentEscItems();
    int &index = currentEscIndex();
    int &scroll = currentEscScroll();

    drawTextMenu(
        canvas,
        escMenuTitle(),
        items,
        index,
        scroll,
        visibleLines   
    );
}

/**
 * ESC 菜单模式的主循环函数，处理菜单输入和功能分发
 *
 * 处理以下键盘操作：
 * - ESC 键（`）在子菜单中返回上一级，在根菜单中自动保存后返回先前模式
 * - 逗号键（,）在子菜单中返回上一级
 * - 斜杠键（/）在可展开项上进入子菜单
 * - 分号键（;）向上移动光标，句号键（.）向下移动光标
 * - Enter 键执行当前菜单项；根菜单中的部分项会进入二级菜单
 */
void loopEscMenuMode() {
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        auto st = M5Cardputer.Keyboard.keysState();
        userAction = true;
        std::vector<String> &items = currentEscItems();
        int &index = currentEscIndex();
        int &scroll = currentEscScroll();

        for (auto c : st.word) {
            if (c == '`') {  // ESC 键
                if (escMenuGroup != ESC_MENU_ROOT) {
                    escMenuGroup = ESC_MENU_ROOT;
                    drawEscMenu();
                } else {
                    returnFromEscMenu();
                }
                return;
            }

            if (c == ',') {
                if (escMenuGroup != ESC_MENU_ROOT) {
                    escMenuGroup = ESC_MENU_ROOT;
                    drawEscMenu();
                    return;
                }
            }

            if (c == '/') {
                if (escMenuGroup == ESC_MENU_ROOT) {
                    if (escRootIndex == 2) {
                        escMenuGroup = ESC_MENU_VOCAB;
                        drawEscMenu();
                        return;
                    }
                    if (escRootIndex == 3) {
                        escMenuGroup = ESC_MENU_MODE;
                        drawEscMenu();
                        return;
                    }
                }
            }

            if (c == ';') {  // 上
                navigateMenu(index, scroll, items.size(), visibleLines, true);
                drawEscMenu();
            }
            if (c == '.') {  // 下
                navigateMenu(index, scroll, items.size(), visibleLines, false);
                drawEscMenu();
            }
        }

        if (st.enter) {
            if (escMenuGroup == ESC_MENU_ROOT) {
                if (escRootIndex == 0) {
                    appMode = MODE_STATS;
                    initStatsMode();
                    return;
                }
                else if (escRootIndex == 1) {
                    appMode = MODE_WORD_TABLE;
                    initWordTableMode();
                    return;
                }
                else if (escRootIndex == 2) {
                    escMenuGroup = ESC_MENU_VOCAB;
                    drawEscMenu();
                    return;
                }
                else if (escRootIndex == 3) {
                    escMenuGroup = ESC_MENU_MODE;
                    drawEscMenu();
                    return;
                }
                else if (escRootIndex == 4) {
                    appMode = MODE_DICTATION_REVIEW;
                    initDictationReviewHistoryMode();
                    return;
                }
                else if (escRootIndex == 5) {
                    appMode = MODE_KEY_HELP;
                    initKeyHelpMode();
                    return;
                }
                else if (escRootIndex == 6) {
                    appMode = MODE_WIFI_SCAN;
                    initWiFiScanMode();
                    return;
                }
            }
            else if (escMenuGroup == ESC_MENU_VOCAB) {
                if (escVocabIndex == 0) {
                    appMode = MODE_WORD_TABLE;
                    initWordTableMode();
                    return;
                }
                else if (escVocabIndex == 1) {
                    autoSaveIfNeeded();
                    currentDir = currentWordRoot;
                    appMode = MODE_FILE_SELECT;
                    initFileSelectMode();
                    return;
                }
                else if (escVocabIndex == 2) {
                    autoSaveIfNeeded();
                    appMode = MODE_LANG_SELECT;
                    initLanguageSelectMode();
                    return;
                }
            }
            else if (escMenuGroup == ESC_MENU_MODE) {
                if (escModeIndex == 0) {
                    appMode = MODE_STUDY;
                    drawStudyWord();
                    return;
                }
                else if (escModeIndex == 1) {
                    appMode = MODE_LISTEN;
                    initListenMode();
                    return;
                }
                else if (escModeIndex == 2) {
                    appMode = MODE_DICTATION;
                    initDictationMode();
                    return;
                }
            }
        }
    }
}
