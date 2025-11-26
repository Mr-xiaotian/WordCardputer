// =============== 听写模式（带 Level1 日语输入法） ===============

std::vector<int> dictOrder;  // 随机顺序
int dictPos = 0;             // 当前是第几个单词

// 输入法相关：
// commitText      = 已经“确认提交”的假名（真正作为答案的一部分）
// romajiBuffer    = 当前正在输入的罗马音
// candidateKana   = 当前 romajiBuffer 对应的候选假名（还未提交）
String commitText = "";
String romajiBuffer = "";
String candidateKana = "";

int correctCount = 0;
int wrongCount = 0;

bool dictShowSummary = false;

// ---------- 初始化听写模式 ----------
void initDictationMode() {
  dictOrder.clear();
  for (int i = 0; i < (int)words.size(); i++) dictOrder.push_back(i);

  // 随机顺序
  for (int i = 0; i < (int)dictOrder.size(); i++) {
    int j = random(dictOrder.size());
    std::swap(dictOrder[i], dictOrder[j]);
  }

  dictPos = 0;

  commitText = "";
  romajiBuffer = "";
  candidateKana = "";

  correctCount = 0;
  wrongCount = 0;
  dictShowSummary = false;

  drawDictationInput();
  playAudioForWord(words[dictOrder[dictPos]].jp);
}

// ---------- 绘制答题中的画面 ----------
void drawDictationInput() {
  canvas.fillSprite(BLACK);

  // 标题（中文）
  canvas.setTextFont(&fonts::efontCN_16);
  canvas.setTextDatum(top_left);
  canvas.setTextColor(TFT_DARKGREY);
  canvas.setTextSize(1.0);
  canvas.drawString("听写模式", 8, 8);

  // 主输入显示：已提交假名 + 当前罗马音（大字号，居中）
  canvas.setTextFont(&fonts::efontJA_16);
  canvas.setTextDatum(middle_center);
  canvas.setTextColor(WHITE);
  canvas.setTextSize(2.0);
  String mainLine = commitText + romajiBuffer;
  canvas.drawString(mainLine, canvas.width() / 2, canvas.height() / 2 - 10);

  // 候选假名（小一号字，显示在下面）
  canvas.setTextSize(1.4);
  canvas.setTextColor(TFT_CYAN);
  if (candidateKana.length() > 0) {
    canvas.drawString(candidateKana, canvas.width() / 2, canvas.height() / 2 + 20);
  }

  // 进度提示
  canvas.setTextDatum(bottom_center);
  canvas.setTextColor(TFT_DARKGREY);
  canvas.setTextSize(1.0);
  canvas.drawString(
      String(dictPos + 1) + "/" + String(dictOrder.size()),
      canvas.width() / 2,
      canvas.height() - 10);

  canvas.pushSprite(0, 0);
}

// ---------- 绘制总结界面 ----------
void drawDictationSummary() {
  canvas.fillSprite(BLACK);
  canvas.setTextFont(&fonts::efontCN_16);

  canvas.setTextDatum(top_left);
  canvas.setTextColor(TFT_DARKGREY);
  canvas.setTextSize(1.0);
  canvas.drawString("听写结束", 8, 8);

  canvas.setTextDatum(middle_center);
  canvas.setTextSize(1.3);

  canvas.setTextColor(GREEN);
  canvas.drawString("正确: " + String(correctCount),
                    canvas.width() / 2, canvas.height() / 2);
  canvas.setTextColor(RED);
  canvas.drawString("错误: " + String(wrongCount),
                    canvas.width() / 2, canvas.height() / 2 + 40);

  canvas.pushSprite(0, 0);
}

// 帮助函数：提交当前候选假名（由 ';' 或 Enter 触发）
void commitCandidate() {
  if (candidateKana.length() > 0) {
    commitText += candidateKana;
    romajiBuffer = "";
    candidateKana = "";
  }
}

// ---------- 听写模式逻辑 ----------
void loopDictationMode() {
  if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
    auto st = M5Cardputer.Keyboard.keysState();
    userAction = true;

    // -------- ESC 返回上级菜单 --------
    for (auto c : st.word) {
      if (c == '`') {  // ESC
        previousMode = appMode; // 记录当前模式
        appMode = MODE_ESC_MENU;
        initEscMenuMode();
        return;
      }
    }

    // -------- 显示总结界面 --------
    if (dictShowSummary) {
      if (st.enter) {
        appMode = MODE_ESC_MENU;
        initEscMenuMode();
      }
      return;
    }

    // -------- 字符输入（字母、确认键等）--------
    for (auto c : st.word) {
      // ';' 用来“确认”候选假名
      if (c == ';') {
        commitCandidate();
        drawDictationInput();
        continue;
      }

      // 字母输入 → romajiBuffer
      if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
        char lc = (char)tolower(c);
        romajiBuffer += lc;

        // 重新计算候选假名
        candidateKana = matchRomaji(romajiBuffer);
        drawDictationInput();
      }
    }

    // -------- 删除键处理 --------
    if (st.del) {
      if (romajiBuffer.length() > 0) {
        // 优先删 romajiBuffer 的尾字母
        romajiBuffer.remove(romajiBuffer.length() - 1);
        candidateKana = matchRomaji(romajiBuffer);
      } else if (commitText.length() > 0) {
        removeLastUTF8Char(commitText);
      }
      drawDictationInput();
    }

    // -------- ENTER 检查答案 --------
    if (st.enter) {
      // 先把当前候选假名也提交掉（方便操作）
      commitCandidate();

      String userAnswer = commitText;                 // 用户最终提交的假名
      String correct = words[dictOrder[dictPos]].jp;  // 正确日语（假名）

      if (userAnswer == correct) correctCount++;
      else wrongCount++;

      dictPos++;

      if (dictPos >= (int)dictOrder.size()) {
        dictShowSummary = true;
        drawDictationSummary();
        return;
      }

      // 进入下一个单词
      commitText = "";
      romajiBuffer = "";
      candidateKana = "";
      drawDictationInput();
      playAudioForWord(words[dictOrder[dictPos]].jp);
    }

    // -------- Fn 键：重复播放当前单词音频 --------
    if (st.fn) {
      // 这里用当前听写单词，而不是 wordIndex
      playAudioForWord(words[dictOrder[dictPos]].jp);
    }
  }
}
