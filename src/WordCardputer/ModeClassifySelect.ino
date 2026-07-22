/**
 * @file ModeClassifySelect.ino
 * @brief 分类方式选择模式
 *
 * 在语言选择和词源/章节选择之间提供一层"分类方式"选择。
 * 当前仅支持"按词源分类"一种方式，选中后进入词源选择模式。
 * 后续可在此层扩展更多分类方式（如按难度、按词性等），
 * 每种分类方式使用独立的 Mode 文件实现具体选择逻辑。
 */

// ===== 核心函数（init / draw / loop） =====

/**
 * 初始化分类方式选择模式
 *
 * 将分类方式选择索引重置为 0（当前唯一选项），并绘制菜单。
 */
void initClassifySelectMode()
{
    classifyIndex = 0;
    drawClassifySelect();
}

/**
 * 绘制分类方式选择菜单
 *
 * 使用 drawTextMenu 在屏幕上渲染分类方式列表，
 * 显示标题"选择分类方式"及所有可选分类项，高亮当前选中项。
 */
void drawClassifySelect()
{
    drawTextMenu(
        canvas,
        "选择分类方式",
        classifyItems,
        classifyIndex,
        0,
        visibleLines,
        "无可选项",
        false,
        false
    );
}

/**
 * 分类方式选择模式的主循环处理函数
 *
 * 监听键盘输入，处理以下操作：
 * - ';' 键：向上移动选择项
 * - '.' 键：向下移动选择项
 * - Enter 键：确认当前分类方式，进入对应选择模式
 *
 * 当前仅"按词源分类"一个选项，选中后直接进入词源选择模式。
 * 后续新增分类方式时，在此处根据 classifyIndex 分发到不同子模式。
 */
void loopClassifySelectMode()
{
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed())
    {
        auto st = M5Cardputer.Keyboard.keysState();
        userAction = true;

        for (auto c : st.word)
        {
            if (c == ';')
            {
                classifyIndex = (classifyIndex - 1 + classifyItems.size()) % classifyItems.size();
                drawClassifySelect();
            }
            else if (c == '.')
            {
                classifyIndex = (classifyIndex + 1) % classifyItems.size();
                drawClassifySelect();
            }
        }

        if (st.enter)
        {
            // 当前仅"按词源分类"一项，直接进入词源选择
            appMode = MODE_FILE_SELECT;
            initFileSelectMode();
            return;
        }
    }
}
