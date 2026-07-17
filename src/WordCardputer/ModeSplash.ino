/**
 * @file ModeSplash.ino
 * @brief 启动画面——ASCII Logo 展示
 *
 * 设备开机后首先展示 WordCardputer 的 ASCII 艺术 Logo，
 * 按任意键后进入语言选择界面。
 */

static const char *LOGO_LINES[] = {
    " _       __               __",
    "| |     / /___  _________/ /",
    "| | /| / / __ \\/ ___/ __  /",
    "| |/ |/ / /_/ / /  / /_/ /",
    "|__/|__/\\____/_/   \\__,_/",
};
static const int LOGO_LINE_COUNT = sizeof(LOGO_LINES) / sizeof(LOGO_LINES[0]);

void initSplashMode()
{
    drawSplash();
}

void drawSplash()
{
    canvas.fillScreen(BLACK);
    canvas.setTextColor(WHITE);
    canvas.setTextDatum(middle_center);

    int cw = canvas.width();
    int ch = canvas.height();

    // Logo 区域（5 行）
    float logoSize = 1.0;
    canvas.setTextSize(logoSize);
    int lineH = canvas.fontHeight();
    int logoTotalH = lineH * LOGO_LINE_COUNT;
    int logoStartY = (ch - logoTotalH) / 2 - 12;

    for (int i = 0; i < LOGO_LINE_COUNT; i++)
    {
        int y = logoStartY + i * lineH;
        canvas.drawString(LOGO_LINES[i], cw / 2, y);
    }

    // 底部提示
    canvas.setTextSize(1.0);
    canvas.setTextColor(0x7BEF);  // 暗绿色
    canvas.drawString("Press any key", cw / 2, ch - 24);

    canvas.pushSprite(0, 0);
}

void loopSplashMode()
{
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed())
    {
        userAction = true;
        appMode = MODE_LANG_SELECT;
        initLanguageSelectMode();
    }
}
