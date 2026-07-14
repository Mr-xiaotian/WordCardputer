/**
 * StudyMode.ino - 闪卡学习模式
 *
 * 实现类似 Anki 的双面闪卡学习功能。支持日语和英语两种语言的卡片显示，
 * 卡片正面/反面随机切换（Side A: 外语优先；Side B: 中文优先）。
 * 学习模式包含“单词页”和“例句页”两张循环页面，用户可通过键盘控制翻卡、
 * 例句显示、调节音量、播放发音，以及调整熟练度评分。
 */

// 学习模式公共变量
int studyPage = 0;             // 0=单词页, 1=例句页
bool showMeaning = false;      // 是否显示释义（单词页面）
bool showSentenceZh = false;   // 是否显示中文例句（例句页面）
bool showAnkiSideA = true;     // true=先显示外语（Side A），false=先显示中文（Side B）

bool studyHasExample(const Word &w);
int studyPageCount(const Word &w);
void drawStudySentence(const Word &w);
void drawEnglishSentence(const Word &w);
void drawJapaneseSentence(const Word &w);

// ===== 核心函数（init / draw / loop） =====

/**
 * 初始化学习模式并从数据库加载词库
 *
 * 加载新词库前先自动保存旧词库进度，然后根据当前选中的
 * `selectedSource` / `selectedChapter` 从数据库读取词条。
 * 加载成功后通过加权随机算法选取第一个单词并绘制闪卡。
 */
void initStudyMode()
{
    autoSaveIfNeeded();

    bool ok = loadWordsFromDB(selectedSource, selectedChapter);
    if (!ok)
    {
        drawCenterString(canvas, "词库加载失败", RED, 1.2);
        return;
    }
    else if (words.empty())
    {
        drawCenterString(canvas, "词库为空", RED, 1.2);
        return;
    }

    wordIndex = pickWeightedRandom();
    showMeaning = false;
    studyPage = 0;
    showSentenceZh = false;
    if (currentLanguage == LANG_EN)
        showAnkiSideA = true;
    drawStudyMode();
}

void drawStudyMode()
{ 
    canvas.fillSprite(BLACK);
    canvas.setTextDatum(middle_center);

    if (words.empty())
    {
        drawCenterString(canvas, "未找到单词数据", RED, 1.2);
        return;
    }

    Word &w = words[wordIndex];

    if (studyPage == 0)
    {
        drawStudyWord(w);
    }
    else if (studyPage == 1)
    {
        drawStudySentence(w);
    }

    // 熟练度提示
    drawTopLeftString(canvas, "Score: " + String(words[wordIndex].score), TFT_DARKGREY, 1.0);

    // HUD 显示音量变化
    if (millis() < volumeMessageDeadline)
    {
        drawTopRightString(canvas, String(soundVolume), TFT_DARKGREY, 1.0);
    }

    canvas.pushSprite(0, 0);
}

/**
 * 学习模式主循环 - 处理键盘输入
 *
 * 支持以下操作：
 * - BtnA（物理按钮）：单词页切换释义；例句页切换例句/中文
 * - ` (ESC)：打开 ESC 菜单
 * - `,` / `/`：左右循环切换 单词页 / 例句页
 * - `;` / `.`：增大/减小音量
 * - Fn：播放当前单词发音
 * - Enter：标记为"记住"，提升熟练度并跳到下一个单词
 * - Del：标记为"不熟"，降低熟练度并跳到下一个单词
 *
 * Enter/Del 操作后会随机切换 Side A/B 并重新抽词。
 */
void loopStudyMode()
{
    // BtnA键 → 单词页翻卡 / 例句页切换中译
    if (M5Cardputer.BtnA.wasPressed())
    {
        if (studyPage == 1 && studyHasExample(words[wordIndex]))
        {
            showSentenceZh = !showSentenceZh;
        }
        else
        {
            showMeaning = !showMeaning;
        }
        drawStudyMode();
        userAction = true;
    }

    // 键盘操作
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed())
    {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
        userAction = true;

        // 检测 esc 键
        for (auto c : status.word)
        {
            if (c == '`')
            {                           // ESC 键
                previousMode = appMode; // 记录当前模式
                appMode = MODE_ESC_MENU;
                initEscMenuMode();
                return;
            }
            else if (c == ';' || c == '.')
            {
                if (adjustVolume(c)) {
                    drawStudyMode();
                }
            }
            else if (c == ',' || c == '/')
            {
                if (studyHasExample(words[wordIndex]))
                {
                    int pageCount = studyPageCount(words[wordIndex]);
                    if (c == '/')
                    {
                        studyPage = (studyPage + 1) % pageCount;
                    }
                    else
                    {
                        studyPage = (studyPage - 1 + pageCount) % pageCount;
                    }
                    drawStudyMode();
                }
            }
        }

        // fn = 播放音频
        if (status.fn)
        {
            if (currentLanguage == LANG_JP)
            {
                playAudioForWord(words[wordIndex].jp);
            }
            else if (words[wordIndex].en.length() > 0)
            {
                playAudioForWord(words[wordIndex].en);
            }
        }

        // Enter = 记住,提升熟练度
        if (status.enter)
        {
            words[wordIndex].score = min(5, words[wordIndex].score + 1);
            markScoreDirty();
        }
        // Del = 不熟,降低熟练度
        else if (status.del)
        {
            words[wordIndex].score = max(1, words[wordIndex].score - 1);
            markScoreDirty();
        }
        if (status.enter || status.del)
        {
            wordIndex = pickWeightedRandom();
            // studyPage = 0;
            showMeaning = false;
            showSentenceZh = false;
            showAnkiSideA = random(2);
            drawStudyMode();
        }
    }
}


// ===== 工具函数 =====

/**
 * 判断当前单词是否包含可展示的例句信息。
 *
 * 只要例句原文或例句中文任一存在，就允许学习模式切换到例句页。
 *
 * @param w 待检查的单词对象
 * @return true 表示存在可展示的例句内容；false 表示仅显示单词页
 */
bool studyHasExample(const Word &w)
{
    return !w.sentence.isEmpty() || !w.sentenceZh.isEmpty();
}

/**
 * 获取当前单词在学习模式中的总页数。
 *
 * 无例句时仅显示单词页；有例句时显示“单词页 + 例句页”共两页。
 *
 * @param w 当前单词对象
 * @return 当前单词对应的学习页总数
 */
int studyPageCount(const Word &w)
{
    return studyHasExample(w) ? 2 : 1;
}

// ===== 核心绘制子函数 =====

/**
 * 绘制当前闪卡的完整画面
 *
 * 清屏后根据当前页面状态绘制单词页或例句页：
 * - 单词页：根据 currentLanguage 调用对应语言的闪卡绘制函数
 * - 例句页：显示例句原文或中文释义
 *
 * 同时左上角显示当前单词的熟练度评分，顶部居中显示页码，
 * 右上角在音量调节后短暂显示音量值。若词库为空则显示错误提示。
 */
void drawStudyWord(const Word &w)
{
    if (currentLanguage == LANG_EN)
    {
        drawEnglishWord(w);
    }
    else if (currentLanguage == LANG_JP)
    {
        drawJapaneseWord(w);
    }
}

/**
 * 绘制英语单词闪卡
 *
 * Side A（showAnkiSideA=true）：主显英语单词和音标，翻卡后显示中文释义。
 * Side B（showAnkiSideA=false）：主显中文释义和词性，翻卡后显示英语原文。
 * 文字大小自动适配屏幕宽度，颜色编码区分不同信息层级。
 *
 * @param w 要显示的单词引用
 */
void drawEnglishWord(const Word &w)
{
    if (showAnkiSideA)
    {
        // === 模式1：显示英语,隐藏中文 ===
        canvas.setTextFont(&fonts::efontCN_16);
        canvas.setTextColor(CYAN);
        drawAutoFitString(canvas, w.en, canvas.width() / 2, canvas.height() / 2 - 25, 2.2);

        canvas.setTextColor(GREEN);
        canvas.setTextSize(1.3);
        {
            String phon = asciiPhonetic(w.phonetic);
            canvas.drawString(phon, canvas.width() / 2, canvas.height() / 2 + 5);
        }

        if (showMeaning)
        {
            canvas.setTextFont(&fonts::efontCN_16);
            canvas.setTextColor(YELLOW);
            drawAutoFitString(canvas, w.zh, canvas.width() / 2, canvas.height() / 2 + 40, 1.5);
        }
    }
    else 
    {
        // === 模式2：显示中文,隐藏英语 ===
        canvas.setTextFont(&fonts::efontCN_16);
        canvas.setTextColor(YELLOW);
        drawAutoFitString(canvas, w.zh, canvas.width() / 2, canvas.height() / 2 - 25, 2.0); // 显示中文释义主行

        if (w.pos.length() > 0)
        {
            canvas.setTextFont(&fonts::efontCN_16);
            canvas.setTextColor(ORANGE);
            canvas.setTextSize(1.4);
            canvas.drawString(w.pos, canvas.width() / 2, canvas.height() / 2 + 5);
        }

        if (showMeaning)
        {
            canvas.setTextFont(&fonts::efontCN_16);
            canvas.setTextColor(CYAN);
            drawAutoFitString(canvas, w.en, canvas.width() / 2, canvas.height() / 2 + 40, 1.8); // 显示英语原文
        }
    }
}

/**
 * 绘制日语单词闪卡
 *
 * Side A（showAnkiSideA=true）：主显日语假名和声调，翻卡后显示中文释义。
 * Side B（showAnkiSideA=false）：主显中文释义和汉字，翻卡后显示日语假名。
 * 文字大小自动适配屏幕宽度，颜色编码区分不同信息层级。
 *
 * @param w 要显示的单词引用
 */
void drawJapaneseWord(const Word &w)
{
    if (showAnkiSideA)
    {
        // === 模式1：显示日语,隐藏中文 ===
        canvas.setTextFont(&fonts::efontJA_16);
        canvas.setTextColor(CYAN);
        drawAutoFitString(canvas, w.jp, canvas.width() / 2, canvas.height() / 2 - 25, 2.2); // 自动适配

        canvas.setTextColor(GREEN);
        canvas.setTextSize(1.3);
        canvas.drawString("Tone: " + String(w.tone), canvas.width() / 2, canvas.height() / 2 + 5);

        if (showMeaning)
        {
            canvas.setTextFont(&fonts::efontCN_16);
            canvas.setTextColor(YELLOW);
            drawAutoFitString(canvas, w.zh, canvas.width() / 2, canvas.height() / 2 + 40, 1.5); // 显示中文释义
        }
    }
    else
    {
        // === 模式2：显示中文,隐藏日语 ===
        canvas.setTextFont(&fonts::efontCN_16);
        canvas.setTextColor(YELLOW);
        drawAutoFitString(canvas, w.zh, canvas.width() / 2, canvas.height() / 2 - 25, 2.0); // 显示中文释义主行

        if (w.kanji.length() > 0)
        {
            canvas.setTextFont(&fonts::efontJA_16);
            canvas.setTextColor(ORANGE);
            canvas.setTextSize(1.4);
            canvas.drawString(w.kanji, canvas.width() / 2, canvas.height() / 2 + 5);
        }

        if (showMeaning)
        {
            canvas.setTextFont(&fonts::efontJA_16);
            canvas.setTextColor(CYAN);
            drawAutoFitString(canvas, w.jp, canvas.width() / 2, canvas.height() / 2 + 40, 1.8); // 显示日语原文
        }
    }
}


/**
 * 绘制学习模式的例句页。
 *
 * 根据当前语言分发到英语或日语的例句绘制函数，
 * 让例句页的结构与单词页保持一致。
 *
 * @param w 当前要显示例句的单词对象
 */
void drawStudySentence(const Word &w)
{
    if (currentLanguage == LANG_EN)
    {
        drawEnglishSentence(w);
    }
    else if (currentLanguage == LANG_JP)
    {
        drawJapaneseSentence(w);
    }
}

/**
 * 绘制英语学习卡片的例句页。
 *
 * 页面上方显示英文单词，下方显示英文例句或对应中文释义。
 * 正文区域会根据宽度自动换行，并在高度不足时自动缩小字号。
 *
 * @param w 当前要显示例句的英语单词对象
 */
void drawEnglishSentence(const Word &w)
{
    if (showSentenceZh)
    {
        canvas.setTextFont(&fonts::efontCN_16);
        canvas.setTextColor(YELLOW);
        canvas.setTextSize(1.2);
        canvas.drawString(w.zh, canvas.width() / 2, 43);

        canvas.setTextFont(&fonts::efontCN_16);
        canvas.setTextColor(WHITE);
        drawWrappedTextBlock(
            canvas,
            w.sentenceZh,
            10,
            64,
            canvas.width() - 20,
            canvas.height() - 96,
            1.45,
            1.0f,
            4
        );

    }
    else
    {
        canvas.setTextFont(&fonts::efontCN_16);
        canvas.setTextColor(CYAN);
        canvas.setTextSize(1.2);
        canvas.drawString(w.en, canvas.width() / 2, 43);

        canvas.setTextFont(&fonts::efontCN_16);
        canvas.setTextColor(WHITE);
        drawWrappedTextBlock(
            canvas,
            w.sentence,
            10,
            64,
            canvas.width() - 20,
            canvas.height() - 96,
            1.45,
            1.0f,
            4
        );
    }
}

/**
 * 绘制日语学习卡片的例句页。
 *
 * 页面上方显示日语词条，下方显示日语例句或对应中文释义。
 * 日语原句使用日文字体，中文释义使用中文字体。
 *
 * @param w 当前要显示例句的日语单词对象
 */
void drawJapaneseSentence(const Word &w)
{
    if (showSentenceZh)
    {
        canvas.setTextFont(&fonts::efontJA_16);
        canvas.setTextColor(YELLOW);
        canvas.setTextSize(1.2);
        canvas.drawString(w.zh, canvas.width() / 2, 43);

        canvas.setTextFont(&fonts::efontCN_16);
        canvas.setTextColor(WHITE);
        drawWrappedTextBlock(
            canvas,
            w.sentenceZh,
            10,
            64,
            canvas.width() - 20,
            canvas.height() - 96,
            1.45,
            1.0f,
            4
        );

    }
    else
    {
        canvas.setTextFont(&fonts::efontJA_16);
        canvas.setTextColor(CYAN);
        canvas.setTextSize(1.2);
        canvas.drawString(w.jp, canvas.width() / 2, 43);

        canvas.setTextFont(&fonts::efontJA_16);
        canvas.setTextColor(WHITE);
        drawWrappedTextBlock(
            canvas,
            w.sentence,
            10,
            64,
            canvas.width() - 20,
            canvas.height() - 96,
            1.45,
            1.0f,
            4
        );
    }
}
