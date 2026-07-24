/**
 * @file ModeDictationReview.ino
 * @brief 独立的听写错误回顾页面
 *
 * 支持两种入口：
 * - 听写结束后查看本轮错题
 * - 从 ESC 菜单进入，浏览当前语言下的历史错题
 *
 * 页面支持左右翻页和 Fn 重播正确答案语音。
 */

// ===== 核心函数（init / draw / loop） =====

/**
 * 绘制错误回顾页面
 *
 * 显示标题、错误时间、正确答案、错误输入和页码。
 * 若当前没有任何可展示条目，则显示空状态提示。
 */
void drawDictationReviewPage()
{
    canvas.fillSprite(BLACK);
    canvas.setFont(&fonts::efontCN_16);

    if (dictationReviewEntries.empty())
    {
        drawTopLeftString(canvas, dictationReviewTitle, TFT_DARKGREY, 1.0);
        drawCenterString(canvas, "没有错误记录", TFT_DARKGREY, 1.2);
        return;
    }

    DictationReviewEntry &entry = dictationReviewEntries[dictationReviewIndex];

    drawTopLeftString(canvas, dictationReviewTitle, TFT_DARKGREY, 1.0);
    drawTopRightString(canvas, entry.createdAt, TFT_DARKGREY, 1.0);

    canvas.setTextDatum(middle_center);
    canvas.setFont(&fonts::efontCN_16);
    canvas.setTextColor(GREEN);
    drawAutoFitString(canvas, entry.correct, canvas.width() / 2, canvas.height() / 2 - 25, 2.0);

    canvas.setTextColor(RED);
    drawAutoFitString(canvas, entry.wrong, canvas.width() / 2, canvas.height() / 2 + 20, 2.0);

    canvas.setTextDatum(bottom_center);
    canvas.setTextSize(1.0);
    canvas.setTextColor(TFT_DARKGREY);
    canvas.drawString(
        String(dictationReviewIndex + 1) + "/" + String(dictationReviewEntries.size()),
        canvas.width() / 2,
        canvas.height() - 10);

    canvas.pushSprite(0, 0);
}

/**
 * 用本轮听写结果初始化错误回顾页
 *
 * 将当前内存中的 dictErrors 转换为统一的页面条目，
 * 这样听写结束后无需再次查询数据库即可直接回顾本轮错误。
 */
void initDictationReviewFromSession()
{
    dictationReviewEntries.clear();
    dictationReviewEntries.reserve(dictErrors.size());

    for (const auto &e : dictErrors)
    {
        if (e.wordIndex < 0 || e.wordIndex >= (int)words.size())
        {
            continue;
        }

        DictationReviewEntry item;
        item.wordDbId = e.wordDbId;
        item.correct = dictationPromptText(words[e.wordIndex]);
        item.wrong = e.wrong;
        item.createdAt = e.createdAt;
        dictationReviewEntries.push_back(item);
    }

    dictationReviewTitle = "本轮错题";
    dictationReviewIndex = 0;
    drawDictationReviewPage();
}

/**
 * 从数据库加载历史错题并初始化错误回顾页
 *
 * 供 ESC 菜单中的“查看过往错题”入口使用。
 */
void initDictationReviewHistoryMode()
{
    dictationReviewTitle = "历史错题";
    dictationReviewIndex = 0;
    if (!loadDictationReviewEntriesFromDB(dictationReviewEntries))
    {
        dictationReviewEntries.clear();
        drawCenterString(canvas, "错题加载失败", RED, 1.2);
        return;
    }
    drawDictationReviewPage();
}

/**
 * 独立错误回顾页主循环
 *
 * 支持：
 * - ESC 返回 ESC 菜单
 * - `,` / `/` 左右翻页
 * - Fn 重播当前正确答案的语音
 */
void loopDictationReviewMode()
{
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed())
    {
        auto st = M5Cardputer.Keyboard.keysState();
        userAction = true;

        for (auto c : st.word)
        {
            if (c == '`')
            {
                previousMode = MODE_DICTATION_REVIEW;
                appMode = MODE_ESC_MENU;
                initEscMenuMode();
                return;
            }
        }

        if (dictationReviewEntries.empty())
        {
            return;
        }

        for (auto c : st.word)
        {
            if (c == ',')
            {
                dictationReviewIndex =
                    (dictationReviewIndex - 1 + dictationReviewEntries.size()) % dictationReviewEntries.size();
                drawDictationReviewPage();
            }
            else if (c == '/')
            {
                dictationReviewIndex = (dictationReviewIndex + 1) % dictationReviewEntries.size();
                drawDictationReviewPage();
            }
        }

        if (st.fn)
        {
            playAudioForWord(dictationReviewEntries[dictationReviewIndex].correct);
        }
    }
}
