/**
 * @file ModeLangSelect.ino
 * @brief 语言选择模式
 *
 * 提供语言选择界面，允许用户在日语和英语之间切换学习语言。
 * 选择语言后会更新对应的词库路径和音频路径，然后进入文件选择模式。
 */

/**
 * 绘制语言选择菜单
 *
 * 使用 drawTextMenu 在屏幕上渲染语言选择列表，
 * 显示标题"选择语言"以及所有可选语言项，并高亮当前选中项。
 */
void drawLanguageSelect()
{
    drawTextMenu(
        canvas,
        "选择语言",
        langItems,
        langIndex,
        0,
        visibleLines,
        "无可选项",
        false,
        false
    );
}

/**
 * 初始化语言选择模式
 *
 * 将语言选择索引重置为 0（默认选中第一项），并绘制语言选择菜单。
 */
void initLanguageSelectMode()
{
    langIndex = 0;
    drawLanguageSelect();
}

/**
 * 语言选择模式的主循环处理函数
 *
 * 监听键盘输入，处理以下操作：
 * - ';' 键：向上移动选择项
 * - '.' 键：向下移动选择项
 * - Enter 键：确认选择语言，检查词库文件夹是否存在，
 *   若存在则设置语言并切换到文件选择模式，否则显示错误提示
 */
void loopLanguageSelectMode()
{
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed())
    {
        auto st = M5Cardputer.Keyboard.keysState();
        userAction = true;

        for (auto c : st.word)
        {
            if (c == ';')
            {
                langIndex = (langIndex - 1 + langItems.size()) % langItems.size();
                drawLanguageSelect();
            }
            else if (c == '.')
            {
                langIndex = (langIndex + 1) % langItems.size();
                drawLanguageSelect();
            }
        }

        if (st.enter)
        {
            StudyLanguage lang = (langIndex == 0) ? LANG_JP : LANG_EN;
            String root = (lang == LANG_JP) ? "/words_study/jp" : "/words_study/en";
            if (!SD.exists(root))
            {
                drawCenterString(canvas, "未找到词库文件夹", RED, 1.2);
                delay(600);
                drawLanguageSelect();
                return;
            }
            setLanguage(lang);
            appMode = MODE_FILE_SELECT;
            initFileSelectMode();
            return;
        }
    }
}
