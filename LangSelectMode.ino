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

void setLanguage(StudyLanguage lang)
{
    currentLanguage = lang;
    if (currentLanguage == LANG_JP)
    {
        currentWordRoot = "/words_study/jp/word";
        currentAudioRoot = "/words_study/jp/audio";
    }
    else
    {
        currentWordRoot = "/words_study/en/word";
        currentAudioRoot = "/words_study/en/audio";
    }
    currentDir = currentWordRoot;
    selectedFilePath = "";
    words.clear();
}

void initLanguageSelectMode()
{
    langIndex = 0;
    drawLanguageSelect();
}

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
                canvas.fillSprite(BLACK);
                canvas.setTextDatum(middle_center);
                canvas.setTextColor(RED);
                canvas.drawString("未找到词库文件夹", canvas.width() / 2, canvas.height() / 2);
                canvas.pushSprite(0, 0);
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
