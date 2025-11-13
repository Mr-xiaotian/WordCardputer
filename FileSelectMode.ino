// --------- 文件选择模式变量 ---------
std::vector<String> files;
int fileIndex = 0;
int fileScroll = 0;

const int visibleLines = 4;
M5Canvas menuCanvas(&M5Cardputer.Display);

String selectedFilePath = "";

// --------- 初始化文件选择模式 ---------
void initFileSelectMode()
{
    files.clear();
    fileIndex = 0;
    fileScroll = 0;
    selectedFilePath = "";

    menuCanvas.createSprite(M5Cardputer.Display.width(), M5Cardputer.Display.height());
    menuCanvas.setTextFont(&fonts::efontCN_16);
    menuCanvas.setTextSize(1.2);

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
}

// --------- 绘制文件选择页面 ---------
void drawFileSelect()
{
    menuCanvas.fillSprite(BLACK);

    // 标题（左上角）
    menuCanvas.setTextDatum(top_left);
    menuCanvas.setTextColor(GREEN);
    menuCanvas.drawString("选择词库文件", 8, 8);

    // 电量（右上角）
    int batteryLevel = M5Cardputer.Power.getBatteryLevel();
    menuCanvas.setTextDatum(top_right);
    menuCanvas.setTextColor(TFT_DARKGREY);
    menuCanvas.setTextSize(1.0);
    menuCanvas.drawString(String(batteryLevel) + " %", menuCanvas.width() - 8, 8);

    // 绘制完右上角后恢复对齐方式
    menuCanvas.setTextDatum(top_left);
    menuCanvas.setTextColor(WHITE);

    int end = min(fileScroll + visibleLines, (int)files.size());
    for (int i = fileScroll; i < end; i++)
    {
        int y = 40 + (i - fileScroll) * 24;
        if (i == fileIndex)
        {
            menuCanvas.setTextColor(YELLOW);
            menuCanvas.drawString("> " + files[i], 8, y);
        }
        else
        {
            menuCanvas.setTextColor(WHITE);
            menuCanvas.drawString("  " + files[i], 8, y);
        }
    }

    if (files.size() > visibleLines)
    {
        menuCanvas.setTextColor(TFT_DARKGREY);
        menuCanvas.drawRightString(
            String(fileIndex + 1) + "/" + String(files.size()),
            menuCanvas.width() - 8,
            menuCanvas.height() - 24);
    }

    menuCanvas.pushSprite(0, 0);
}

// --------- 文件选择逻辑 ---------
void loopFileSelectMode()
{
    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed())
    {
        auto status = M5Cardputer.Keyboard.keysState();

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
