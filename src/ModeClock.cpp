/**
 * @file ModeClock.cpp
 * @brief 时间显示页面
 *
 * 联网时显示 NTP 同步的完整日期时间（蓝色），
 * 断网时显示设备运行时长（红色）。
 * 页面每秒自动刷新，ESC 返回菜单。
 */

#include "globals.h"

static unsigned long clockLastDraw = 0;
static bool clockShowNtp = false;  // BtnA 切换：使用时间 / 当前时间


void drawConnectTime()
{
    // 联网：显示完整日期时间
    drawTopLeftString(canvas, "当前时间", TFT_DARKGREY, 1.0);

    struct tm t;
    if (getLocalTime(&t, 10))
    {
        char dateBuf[16];
        snprintf(dateBuf, sizeof(dateBuf), "%04d-%02d-%02d",
                 t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
        canvas.setTextDatum(middle_center);
        canvas.setTextColor(CYAN);
        canvas.setTextSize(1.4);
        canvas.drawString(dateBuf, canvas.width() / 2, canvas.height() / 2 - 15);

        char timeBuf[16];
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d",
                 t.tm_hour, t.tm_min, t.tm_sec);
        canvas.setTextSize(2.5);
        canvas.drawString(timeBuf, canvas.width() / 2, canvas.height() / 2 + 20);
    }
    else
    {
        // NTP 时间尚未同步
        canvas.setTextDatum(middle_center);
        canvas.setTextColor(WHITE);
        canvas.setTextSize(1.4);
        canvas.drawString("时间同步中...", canvas.width() / 2, canvas.height() / 2);
    }
}

void drawDisconnectTime()
{
    // 断网：显示运行时长
    drawTopLeftString(canvas, "运行时长", TFT_DARKGREY, 1.0);

    unsigned long sec = millis() / 1000;
    int h = (sec / 3600) % 24;
    int m = (sec / 60) % 60;
    int s = sec % 60;

    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", h, m, s);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(RED);
    canvas.setTextSize(2.5);
    canvas.drawString(buf, canvas.width() / 2, canvas.height() / 2);
}

static void drawClockPage()
{
    canvas.fillSprite(BLACK);
    canvas.setFont(&fonts::efontCN_16);

    if (clockShowNtp && wifiConnected)
    {
        drawConnectTime();
    }
    else
    {
        drawDisconnectTime();
    }

    canvas.pushSprite(0, 0);
}

void initClockMode()
{
    clockLastDraw = 0;
    clockShowNtp = false;
    drawClockPage();
}

void loopClockMode()
{
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed())
    {
        auto st = M5Cardputer.Keyboard.keysState();
        userAction = true;

        for (auto c : st.word)
        {
            if (c == '`')
            {
                previousMode = MODE_CLOCK;
                appMode = MODE_ESC_MENU;
                initEscMenuMode();
                return;
            }
        }
    }

    // BtnA 切换使用时间 / 当前时间（仅联网时有效）
    if (wifiConnected && M5Cardputer.BtnA.wasPressed())
    {
        userAction = true;
        clockShowNtp = !clockShowNtp;
        drawClockPage();
    }

    // 每秒刷新
    unsigned long now = millis();
    if (now - clockLastDraw >= 1000)
    {
        clockLastDraw = now;
        drawClockPage();
    }
}
