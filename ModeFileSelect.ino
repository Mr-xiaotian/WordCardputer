/**
 * @file ModeFileSelect.ino
 * @brief 文件选择模式
 *
 * 提供 SD 卡上词库文件的浏览和选择功能。
 * 扫描当前目录下的文件夹和 JSON 文件，以列表菜单形式展示，
 * 支持进入子目录、返回上级目录以及选中 JSON 文件加载词库。
 */

// --------- 文件选择模式变量 ---------
std::vector<String> files;
int fileIndex = 0;
int fileScroll = 0;

/**
 * 初始化文件选择模式，扫描 SD 卡目录并构建文件列表
 *
 * 清空文件列表，打开当前目录（若不在词库根目录下则重置为根目录），
 * 遍历目录中的文件夹和 JSON 文件加入列表。
 * 列表按文件夹优先、文件名字母序排列，然后绘制选择界面。
 */
void initFileSelectMode()
{
    files.clear();
    fileIndex = 0;
    fileScroll = 0;
    selectedFilePath = "";
    if (!currentDir.startsWith(currentWordRoot))
    {
        currentDir = currentWordRoot;
    }

    File dir = SD.open(currentDir);
    if (!dir) {
        Serial.printf("无法打开目录: %s\n", currentDir.c_str());
        return;
    }

    while (true)
    {
        File entry = dir.openNextFile();
        if (!entry) break;

        String name = entry.name();

        // 跳过系统项
        if (name == "." || name == "..") {
            entry.close();
            continue;
        }

        // 文件夹
        if (entry.isDirectory()) {
            // 顶层显示文件夹
            files.push_back(name + "/");
        }
        // JSON 文件
        else if (name.endsWith(".json")) {
            // 显示本层的 JSON
            files.push_back(name);
        }

        entry.close();
    }
    dir.close();

    // 排序：文件夹优先
    std::sort(files.begin(), files.end(), [](const String &a, const String &b) {
        bool aDir = a.endsWith("/");
        bool bDir = b.endsWith("/");
        if (aDir != bDir) return aDir;  // 文件夹排前面
        return strcmp(a.c_str(), b.c_str()) < 0;
    });

    drawFileSelect();
}

/**
 * 绘制文件选择菜单页面
 *
 * 调用通用菜单绘制函数 drawTextMenu，以列表形式展示当前目录下的
 * 文件和文件夹。文件夹名称以"/"结尾显示。若列表为空则显示提示文字。
 */
void drawFileSelect()
{
    drawTextMenu(
        canvas,
        "选择词库文件",   // 标题
        files,           // 项目列表
        fileIndex,       // 当前选中
        fileScroll,      // 当前滚动起点
        visibleLines,    // 一屏行数
        "没有词库文件"    // 空列表提示（可选）
    );
}

/**
 * 文件选择模式的主循环函数，处理文件浏览输入
 *
 * 处理以下键盘操作：
 * - 分号键（;）向上移动光标，支持循环滚动
 * - 句号键（.）向下移动光标，支持循环滚动
 * - Enter 键选中当前项：文件夹则进入子目录，JSON 文件则加载词库并进入学习模式
 * - Delete 键返回上级目录（不超过词库根目录）
 * 若文件列表为空则显示"无词库文件"提示。
 */
void loopFileSelectMode()
{
    // 如果没有任何文件,直接显示提示并返回
    if (files.empty()) {
        drawCenterString(canvas, "无词库文件", RED, 1.2);
        delay(200);
        return;
    }
    
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed())
    {
        auto status = M5Cardputer.Keyboard.keysState();
        userAction = true;

        for (auto c : status.word)
        {
            if (c == ';')
            {
                navigateMenu(fileIndex, fileScroll, files.size(), visibleLines, true);
            }

            if (c == '.')
            {
                navigateMenu(fileIndex, fileScroll, files.size(), visibleLines, false);
            }
        }

        // 选择
        if (status.enter)
        {
            String item = files[fileIndex];

            // 文件夹 → 进入该目录
            if (item.endsWith("/")) {
                currentDir = currentDir + "/" + item.substring(0, item.length() - 1);
                initFileSelectMode();
                return;
            }

            // JSON 文件 → 加载词库
            selectedFilePath = currentDir + "/" + item;
            appMode = MODE_STUDY;
            startStudyMode(selectedFilePath);
            return;
        }
        
        // 返回
        if (status.del) {
            if (currentDir != currentWordRoot) {
                int pos = currentDir.lastIndexOf('/');
                currentDir = currentDir.substring(0, pos);
                initFileSelectMode();
            }
        }

        drawFileSelect();
    }
}
