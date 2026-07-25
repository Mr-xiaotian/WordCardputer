/**
 * @file ModeDictationReview.cpp
 * @brief 独立的听写错误回顾页面
 *
 * 支持两种入口：
 * - 听写结束后查看本轮错题
 * - 从 ESC 菜单进入，浏览当前语言下的历史错题
 *
 * 页面支持翻页、删除错题、Fn 重播语音、BtnA 切换单词详情。
 */

#include "globals.h"
#include "UtilsAudio.h"
#include "UtilsString.h"

// ===== 内部状态 =====
static bool reviewShowDetail = false;  // BtnA 切换：显示单词详情 / 错误对照

// ===== 内部辅助函数 =====

/**
 * 在 words 列表中按 dbId 查找单词
 *
 * @return 找到返回指针，未找到返回 nullptr
 */
static const Word *findWordByDbId(int dbId)
{
    for (const auto &w : words)
    {
        if (w.dbId == dbId)
            return &w;
    }
    return nullptr;
}

/**
 * 绘制单词详情视图（复用听读模式的布局风格）
 */
static void drawReviewWordDetail()
{
    canvas.fillSprite(BLACK);

    DictationReviewEntry &entry = dictationReviewEntries[dictationReviewIndex];
    const Word *w = findWordByDbId(entry.wordDbId);

    drawTopLeftString(canvas, dictationReviewTitle, TFT_DARKGREY, 1.0);
    drawTopRightString(canvas, entry.createdAt, TFT_DARKGREY, 1.0);

    canvas.setTextDatum(middle_center);

    if (!w)
    {
        // 词库未加载，仅显示正确答案
        canvas.setFont(&fonts::efontCN_16);
        canvas.setTextColor(CYAN);
        drawAutoFitString(canvas, entry.correct, canvas.width() / 2, canvas.height() / 2, 2.0);
    }
    else if (currentLanguage == LANG_EN)
    {
        canvas.setFont(&fonts::efontCN_16);
        canvas.setTextColor(CYAN);
        drawAutoFitString(canvas, w->en, canvas.width() / 2, canvas.height() / 2 - 25, 2.2);

        String sub = asciiPhonetic(w->phonetic);
        if (sub.length() > 0)
        {
            canvas.setTextColor(GREEN);
            canvas.setTextSize(1.3);
            drawAutoFitString(canvas, sub, canvas.width() / 2, canvas.height() / 2 + 10, 1.2);
        }

        canvas.setFont(&fonts::efontCN_16);
        canvas.setTextColor(YELLOW);
        drawAutoFitString(canvas, w->zh, canvas.width() / 2, canvas.height() / 2 + 40, 1.5);
    }
    else
    {
        canvas.setFont(&fonts::efontJA_16);
        canvas.setTextColor(CYAN);
        drawAutoFitString(canvas, w->jp, canvas.width() / 2, canvas.height() / 2 - 25, 2.2);

        if (w->kanji.length() > 0)
        {
            canvas.setTextColor(ORANGE);
            canvas.setTextSize(1.4);
            canvas.drawString(w->kanji, canvas.width() / 2, canvas.height() / 2 + 10);
        }

        canvas.setFont(&fonts::efontCN_16);
        canvas.setTextColor(YELLOW);
        drawAutoFitString(canvas, w->zh, canvas.width() / 2, canvas.height() / 2 + 40, 1.5);
    }

    // 翻页指示
    canvas.setTextDatum(bottom_center);
    canvas.setFont(&fonts::efontCN_16);
    canvas.setTextSize(1.0);
    canvas.setTextColor(TFT_DARKGREY);
    canvas.drawString(
        String(dictationReviewIndex + 1) + "/" + String(dictationReviewEntries.size()),
        canvas.width() / 2,
        canvas.height() - 1);

    canvas.pushSprite(0, 0);
}

/**
 * 绘制单词错误对照页面
 */
void drawReviewWordError()
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

// ===== 核心函数（init / draw / loop） =====

/**
 * 绘制错误回顾页面
 *
 * 默认显示错误对照（正确答案 vs 错误输入），
 * 按 BtnA 可切换到单词详情视图。
 */
void drawDictationReviewPage()
{
    if (reviewShowDetail)
    {
        drawReviewWordDetail();
        return;
    }

    else
    {
        drawReviewWordError();
    }
}

/**
 * 用本轮听写结果初始化错误回顾页
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
    reviewShowDetail = false;
    drawDictationReviewPage();
}

/**
 * 从数据库加载历史错题并初始化错误回顾页
 */
void initDictationReviewHistoryMode()
{
    dictationReviewTitle = "历史错题";
    dictationReviewIndex = 0;
    reviewShowDetail = false;
    if (!loadDictationReviewEntriesFromDB(dictationReviewEntries))
    {
        dictationReviewEntries.clear();
        drawCenterString(canvas, "错题加载失败", RED, 1.2);
        return;
    }

    // 同步加载错题对应的单词数据，确保详情视图可用
    std::vector<int> ids;
    for (const auto &e : dictationReviewEntries)
    {
        if (e.wordDbId > 0)
            ids.push_back(e.wordDbId);
    }
    loadWordsByIds(ids);

    drawDictationReviewPage();
}

/**
 * 错误回顾页主循环
 *
 * 支持：
 * - ESC 返回菜单
 * - , / / 翻页
 * - BtnA 切换错误对照 / 单词详情
 * - Del 删除当前错题
 * - Fn 播放正确答案语音
 */
void loopDictationReviewMode()
{
    // BtnA 切换错误对照 / 单词详情
    if (M5Cardputer.BtnA.wasPressed())
    {
        reviewShowDetail = !reviewShowDetail;
        drawDictationReviewPage();
    }

    // 键盘操作
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

        // Del 键删除当前错题
        if (st.del)
        {
            int errorId = dictationReviewEntries[dictationReviewIndex].errorId;
            deleteDictationError(errorId);
            dictationReviewEntries.erase(
                dictationReviewEntries.begin() + dictationReviewIndex);

            if (dictationReviewEntries.empty())
            {
                dictationReviewIndex = 0;
            }
            else if (dictationReviewIndex >= (int)dictationReviewEntries.size())
            {
                dictationReviewIndex = dictationReviewEntries.size() - 1;
            }

            drawDictationReviewPage();
        }

        if (st.fn)
        {
            playAudioForWord(dictationReviewEntries[dictationReviewIndex].correct);
        }
    }
}
