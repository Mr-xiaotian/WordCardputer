/**
 * AudioUtils.ino - 音频播放工具
 *
 * 提供从 SD 卡读取 WAV 文件并通过 M5Cardputer 扬声器播放的功能。
 * 支持 8/16 位 PCM 格式、单声道/立体声，采用三重缓冲流式播放以降低内存占用。
 */

/**
 * 流式播放 SD 卡上的 WAV 音频文件
 *
 * 解析 WAV 文件头（RIFF/fmt/data 块），验证格式后使用三重缓冲（3 x 1024 字节）
 * 逐块读取音频数据并送入 M5 Speaker 播放。支持 8 位和 16 位 PCM 编码，
 * 自动识别采样率和声道数。此实现风格与 M5Stack 官方 demo 一致。
 *
 * @param path SD 卡上 WAV 文件的完整路径
 * @return true 播放成功；false 文件无法打开、格式不支持或找不到 data 块
 */
// ------------------- 官方风格流式 WAV 播放 -------------------
bool playWavStream(const String& path) {
    File file = SD.open(path);
    if (!file) {
        Serial.printf("无法打开音频文件: %s\n", path.c_str());
        return false;
    }

    // ----- WAV Header 定义（与官方一致） -----
    struct __attribute__((packed)) WavHeader {
        char RIFF[4];
        uint32_t chunkSize;
        char WAVEfmt[8];
        uint32_t fmt_chunk_size;
        uint16_t audiofmt;
        uint16_t channel;
        uint32_t sample_rate;
        uint32_t byte_per_sec;
        uint16_t block_size;
        uint16_t bit_per_sample;
    } header;

    struct __attribute__((packed)) SubChunk {
        char id[4];
        uint32_t size;
        uint8_t data[1];
    } sub;

    // -------- 读头 --------
    file.read((uint8_t*)&header, sizeof(header));

    // -------- 验证 WAV 格式（与官方一致） --------
    if (memcmp(header.RIFF, "RIFF", 4)
        || memcmp(header.WAVEfmt, "WAVEfmt ", 8)
        || header.audiofmt != 1
        || header.bit_per_sample < 8
        || header.bit_per_sample > 16
        || header.channel == 0
        || header.channel > 2)
    {
        Serial.println("非 PCM WAV 或格式不支持");
        file.close();
        return false;
    }

    // -------- 跳到 fmt 之后的 chunk --------
    file.seek(offsetof(WavHeader, audiofmt) + header.fmt_chunk_size);

    // -------- 查找 "data" 块 --------
    file.read((uint8_t*)&sub, 8);

    while (memcmp(sub.id, "data", 4)) {
        if (!file.seek(sub.size, SeekCur)) break;
        file.read((uint8_t*)&sub, 8);
    }

    if (memcmp(sub.id, "data", 4)) {
        Serial.println("找不到 data 块");
        file.close();
        return false;
    }

    // -------- 播放 data 区 --------
    int32_t remain = sub.size;
    bool is16bit = (header.bit_per_sample == 16);

    static constexpr size_t BUF_NUM = 3;    // 官方 demo 用 3 块
    static constexpr size_t BUF_SIZE = 1024;
    static uint8_t buf[BUF_NUM][BUF_SIZE];

    size_t idx = 0;

    while (remain > 0) {
        size_t len = remain < BUF_SIZE ? remain : BUF_SIZE;
        len = file.read(buf[idx], len);
        if (len == 0) break;
        remain -= len;

        if (is16bit) {
            M5.Speaker.playRaw(
                (const int16_t*)buf[idx],
                len >> 1,
                header.sample_rate,
                header.channel > 1,
                1,   // 官方 demo 的参数：repeat = 1
                0
            );
        } else {
            M5.Speaker.playRaw(
                (const uint8_t*)buf[idx],
                len,
                header.sample_rate,
                header.channel > 1,
                1,   // 官方 demo 的参数
                0
            );
        }

        idx = (idx + 1) % BUF_NUM;
    }

    file.close();
    return true;
}

/**
 * 播放指定单词的发音音频
 *
 * 根据单词文本在 currentAudioRoot 目录下查找对应的 WAV 文件（格式：单词.wav）。
 * 若音频文件不存在则播放一个短促的提示音（880Hz）作为反馈。
 * 播放前会等待上一段音频播放完毕，避免声音重叠。
 * 若 WAV 文件解析失败则播放低频提示音（440Hz）提醒用户。
 *
 * @param jpWord 要播放发音的单词文本（日语假名或英语单词）
 */
void playAudioForWord(const String& jpWord) {
    if (jpWord.length() == 0)
        return;
    String path = currentAudioRoot + "/" + jpWord + ".wav";

    // 检查文件是否存在
    if (!SD.exists(path)) {
        Serial.printf("无音频文件: %s\n", path.c_str());
        M5.Speaker.tone(880, 80);  // 提示音
        return;
    }

    // 如果正在播放旧音频则停止
    while (M5.Speaker.isPlaying()) {
        delay(1);
    }
    M5.Speaker.stop(); // 再停一次更安全

    // 播放 SD 卡上的 WAV 文件
    bool ok = playWavStream(path);
    if (!ok) {
        M5.Speaker.tone(440, 100);
    }
}
