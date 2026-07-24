/**
 * UtilsAudio.h - 音频播放工具（声明）
 */
#pragma once
#include "globals.h"

bool adjustVolume(char c);
bool playWavStream(const String& path);
void playAudioForWord(const String& jpWord);
