void drawAutoFitString(M5Canvas &cv, const String &text,
                       int x, int y, int maxWidth,
                       float baseSize = 2.0, float minSize = 0.8) {
    if (text.length() == 0) return;

    float size = baseSize;
    cv.setTextSize(size);
    int width = cv.textWidth(text);

    while (width > maxWidth && size > minSize) {
        size -= 0.1;
        cv.setTextSize(size);
        width = cv.textWidth(text);
    }

    cv.setTextDatum(middle_center);
    cv.drawString(text, x, y);
}