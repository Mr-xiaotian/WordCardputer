// --------- 文件选择模式变量 ---------
std::vector<String> files;
int fileIndex = 0;
int fileScroll = 0;

// --------- 初始化文件选择模式 ---------
void initFileSelectMode()
{
    files.clear();
    fileIndex = 0;
    fileScroll = 0;
    selectedFilePath = "";

    File root = SD.open("/jp_words_study/word");
    while (true)
    {
        File entry = root.openNextFile();
        if (!entry)
            break;
        String name = entry.name();
        if (name.endsWith(".json"))
            files.push_back(name);
        entry.close();
    }
    root.close();

    // 排序（字典序）
    std::sort(files.begin(), files.end(), [](const String &a, const String &b) {
        return strcmp(a.c_str(), b.c_str()) < 0;
    });
}

// --------- 绘制文件选择页面 ---------
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

// --------- 文件选择逻辑 ---------
void loopFileSelectMode()
{
    // 如果没有任何文件，直接显示提示并返回
    if (files.empty()) {
        canvas.fillSprite(BLACK);
        canvas.setTextDatum(middle_center);
        canvas.setTextColor(RED);
        canvas.drawString("无词库文件", canvas.width()/2, canvas.height()/2);
        canvas.pushSprite(0, 0);
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
                fileIndex = (fileIndex - 1 + files.size()) % files.size();
                if (fileIndex == files.size() - 1)
                {
                    // ✅ 从第一行上翻到最后一行
                    fileScroll = max(0, (int)files.size() - visibleLines);
                }
                else if (fileIndex < fileScroll)
                {
                    fileScroll = fileIndex;
                }
            }

            if (c == '.')
            {
                fileIndex = (fileIndex + 1) % files.size();
                if (fileIndex == 0)
                {
                    // ✅ 从最后一行下翻回到第一行
                    fileScroll = 0;
                }
                else if (fileIndex >= fileScroll + visibleLines)
                {
                    fileScroll = fileIndex - visibleLines + 1;
                }
            }
        }

        // 选择
        if (status.enter)
        {
            selectedFilePath = "/jp_words_study/word/" + files[fileIndex];
            appMode = MODE_STUDY;
            startStudyMode(selectedFilePath);
            return;
        }
    }

    drawFileSelect();
}
