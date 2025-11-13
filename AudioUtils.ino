// ------------------- 音频播放 -------------------
bool playWavFile(const String& path) {
    if (!SD.exists(path)) {
        Serial.printf("文件不存在: %s\n", path.c_str());
        M5.Speaker.tone(880, 80);
        return false;
    }

    File f = SD.open(path, FILE_READ);
    if (!f) {
        Serial.printf("无法打开: %s\n", path.c_str());
        M5.Speaker.tone(440, 100);
        return false;
    }

    size_t len = f.size();
    if (len == 0) {
        Serial.printf("空文件: %s\n", path.c_str());
        f.close();
        return false;
    }

    // 为短音频一次性分配缓冲；若文件可能偏大，建议改成分块 playRaw 流式
    uint8_t* buf = (uint8_t*)malloc(len);
    if (!buf) {
        Serial.println("内存分配失败");
        f.close();
        return false;
    }

    size_t readn = f.read(buf, len);
    f.close();
    if (readn == 0) {
        Serial.println("读取失败");
        free(buf);
        return false;
    }

    // 停掉正在播放的声音，避免叠加
    if (M5.Speaker.isPlaying()) M5.Speaker.stop();

    bool ok = M5.Speaker.playWav(buf, readn, 1, -1, true); // 传入数据指针
    if (!ok) {
        Serial.println("playWav 调用失败");
        free(buf);
        return false;
    }

    // 阻塞等待播放结束，再释放内存（关键！不要提前 free）
    while (M5.Speaker.isPlaying()) { delay(10); }
    free(buf);
    return true;
}

void playAudioForWord(const String& jpWord) {
    String path = "/jp_words_study/audio/" + jpWord + ".wav";

    // 检查文件是否存在
    if (!SD.exists(path)) {
        Serial.printf("无音频文件: %s\n", path.c_str());
        M5.Speaker.tone(880, 80);  // 提示音
        return;
    }

    // 如果正在播放旧音频则停止
    if (M5.Speaker.isPlaying()) {
        M5.Speaker.stop();
    }

    // 播放 SD 卡上的 WAV 文件
    bool ok = playWavFile(path);
    if (!ok) {
        M5.Speaker.tone(440, 100);
    }
}