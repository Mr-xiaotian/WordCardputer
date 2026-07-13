/**
 * @file ModeFileSelect.ino
 * @brief 文件选择模式
 *
 * 提供词库数据库中的 source / chapter 浏览和选择功能。
 * 根层展示 source，进入后展示 chapter；无 chapter 的 source 直接加载。
 */

// --------- 文件选择模式变量 ---------
std::vector<String> files;
std::vector<bool> fileExpandable;
int fileIndex = 0;
int fileScroll = 0;

/**
 * 初始化文件选择模式，从数据库构建 source / chapter 列表
 *
 * currentDir 用作虚拟目录：
 * - currentWordRoot：source 列表
 * - currentWordRoot/<source>：chapter 列表
 */
void initFileSelectMode()
{
    files.clear();
    fileExpandable.clear();
    fileIndex = 0;
    fileScroll = 0;
    if (!currentDir.startsWith(currentWordRoot))
    {
        currentDir = currentWordRoot;
    }

    if (currentDir == currentWordRoot) {
        if (!loadSourceList(files)) {
            Serial.println("读取 source 列表失败");
            return;
        }
        fileExpandable.reserve(files.size());
        for (const auto &source : files) {
            fileExpandable.push_back(sourceHasChapters(source));
        }
    } else {
        String source = currentDir.substring(currentWordRoot.length() + 1);
        if (!loadChapterList(source, files)) {
            Serial.println("读取 chapter 列表失败");
            return;
        }
        fileExpandable.assign(files.size(), false);
    }

    drawFileSelect();
}

/**
 * 绘制文件选择菜单页面
 *
 * 调用通用菜单绘制函数 drawTextMenu，以列表形式展示当前层级的
 * source 或 chapter。该模式不再直接浏览 SD 卡真实文件，而是浏览
 * 数据库中的逻辑结构。根层标题显示“选择词源”，chapter 层显示
 * “选择章节”。若列表为空则显示提示文字。
 */
void drawFileSelect()
{
    String title = (currentDir == currentWordRoot) ? "选择词源" : "选择章节";
    std::vector<String> displayItems = files;
    if (currentDir == currentWordRoot) {
        for (size_t i = 0; i < displayItems.size() && i < fileExpandable.size(); i++) {
            if (fileExpandable[i]) {
                displayItems[i] += " >";
            }
        }
    }
    drawTextMenu(
        canvas,
        title,           // 标题
        displayItems,    // 项目列表
        fileIndex,       // 当前选中
        fileScroll,      // 当前滚动起点
        visibleLines,    // 一屏行数
        "没有词库数据"    // 空列表提示（可选）
    );
}

/**
 * 文件选择模式的主循环函数，处理文件浏览输入
 *
 * 处理以下键盘操作：
 * - 分号键（;）向上移动光标
 * - 句号键（.）向下移动光标
 * - 斜杠键（/）在可展开项上进入 chapter 子层
 * - Enter 键选中当前项：source 可进入 chapter 或直接加载；chapter 则加载词库
 * - 逗号键（,）和 ESC 键（`）从 chapter 子层返回 source 根层
 *
 * `currentDir` 在这里是“虚拟目录”：
 * - 根层：`currentWordRoot`
 * - 子层：`currentWordRoot/<source>`
 *
 * `selectedFilePath` 仍保留旧名字，但现在只作为显示标签使用，
 * 真实加载依据是 `selectedSource` 和 `selectedChapter`。
 */
void loopFileSelectMode()
{
    if (files.empty()) {
        drawCenterString(canvas, "无词库数据", RED, 1.2);
        delay(200);
        return;
    }
    
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed())
    {
        auto status = M5Cardputer.Keyboard.keysState();
        userAction = true;

        for (auto c : status.word)
        {
            if (c == '`' || c == ',')
            {
                if (currentDir != currentWordRoot) {
                    currentDir = currentWordRoot;
                    initFileSelectMode();
                    return;
                }
            }

            if (c == '/')
            {
                if (currentDir == currentWordRoot &&
                    fileIndex >= 0 &&
                    fileIndex < (int)fileExpandable.size() &&
                    fileExpandable[fileIndex]) {
                    currentDir = currentWordRoot + "/" + files[fileIndex];
                    initFileSelectMode();
                    return;
                }
            }

            if (c == ';')
            {
                navigateMenu(fileIndex, fileScroll, files.size(), visibleLines, true);
            }

            if (c == '.')
            {
                navigateMenu(fileIndex, fileScroll, files.size(), visibleLines, false);
            }
        }

        if (status.enter)
        {
            String item = files[fileIndex];

            if (currentDir == currentWordRoot) {
                bool canExpand = fileIndex >= 0 &&
                                 fileIndex < (int)fileExpandable.size() &&
                                 fileExpandable[fileIndex];
                if (canExpand) {
                    currentDir = currentWordRoot + "/" + item;
                    initFileSelectMode();
                    return;
                }

                selectedSource = item;
                selectedChapter = "";
                selectedFilePath = item;
            } else {
                selectedSource = currentDir.substring(currentWordRoot.length() + 1);
                selectedChapter = item;
                selectedFilePath = selectedSource;
                if (!selectedChapter.isEmpty()) {
                    selectedFilePath += "/" + selectedChapter;
                }
            }

            appMode = MODE_STUDY;
            initStudyMode();
            return;
        }

        drawFileSelect();
    }
}
