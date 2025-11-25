struct RomajiMap {
    const char* romaji;
    const char* kana;
};

static const RomajiMap ROMAJI_TABLE[] = {
    // 清音组合
    {"a", "あ"},  {"i", "い"},  {"u", "う"},  {"e", "え"},  {"o", "お"},
    {"ka", "か"}, {"ki", "き"}, {"ku", "く"}, {"ke", "け"}, {"ko", "こ"},
    {"sa", "さ"}, {"si", "し"}, {"su", "す"}, {"se", "せ"}, {"so", "そ"},
    {"ta", "た"}, {"ti", "ち"}, {"tu", "つ"}, {"te", "て"}, {"to", "と"},
    {"na", "な"}, {"ni", "に"}, {"nu", "ぬ"}, {"ne", "ね"}, {"no", "の"},
    {"ha", "は"}, {"hi", "ひ"}, {"hu", "ふ"}, {"he", "へ"}, {"ho", "ほ"},
    {"ma", "ま"}, {"mi", "み"}, {"mu", "む"}, {"me", "め"}, {"mo", "も"},
    {"ya", "や"}, {"yu", "ゆ"}, {"yo", "よ"},
    {"ra", "ら"}, {"ri", "り"}, {"ru", "る"}, {"re", "れ"}, {"ro", "ろ"},
    {"wa", "わ"}, {"wo", "を"}, {"n", "ん"}, 

    // 浊音组合
    {"ga", "が"}, {"gi", "ぎ"}, {"gu", "ぐ"}, {"ge", "げ"}, {"go", "ご"},
    {"za", "ざ"}, {"zi", "じ"}, {"zu", "ず"}, {"ze", "ぜ"}, {"zo", "ぞ"},
    {"da", "だ"}, {"di", "ぢ"}, {"du", "づ"}, {"de", "で"}, {"do", "ど"},
    {"ba", "ば"}, {"bi", "び"}, {"bu", "ぶ"}, {"be", "べ"}, {"bo", "ぼ"},

    // 半浊音组合
    {"pa", "ぱ"}, {"pi", "ぴ"}, {"pu", "ぷ"}, {"pe", "ぺ"}, {"po", "ぽ"},

    // 拗音组合
    {"kya", "きゃ"}, {"kyu", "きゅ"}, {"kyo", "きょ"},
    {"sha", "しゃ"}, {"shu", "しゅ"}, {"sho", "しょ"},
    {"cha", "ちゃ"}, {"chu", "ちゅ"}, {"cho", "ちょ"},
    {"nya", "にゃ"}, {"nyu", "にゅ"}, {"nyo", "にょ"},
    {"hya", "ひゃ"}, {"hyu", "ひゅ"}, {"hyo", "ひょ"},
    {"mya", "みゃ"}, {"myu", "みゅ"}, {"myo", "みょ"},
    {"rya", "りゃ"}, {"ryu", "りゅ"}, {"ryo", "りょ"},
    {"gya", "ぎゃ"}, {"gyu", "ぎゅ"}, {"gyo", "ぎょ"},
    {"ja", "じゃ"},  {"ju", "じゅ"},  {"jo", "じょ"},
    {"bya", "びゃ"}, {"byu", "びゅ"}, {"byo", "びょ"},
    {"pya", "ぴゃ"}, {"pyu", "ぴゅ"}, {"pyo", "ぴょ"},
};

static const int ROMAJI_TABLE_SIZE = sizeof(ROMAJI_TABLE) / sizeof(ROMAJI_TABLE[0]);

// ---------- 罗马音转换表 ----------
// 根据当前 romajiBuffer 返回“候选假名”
// 仅当 buffer 完全匹配某一条 romaji 时才返回假名，否则返回 ""。
// 不修改 buffer，本函数只是“看一眼”。
String matchRomaji(const String &buffer) {
  if (buffer.length() == 0) return "";

  for (int i = 0; i < ROMAJI_TABLE_SIZE; ++i) {
    if (buffer.equalsIgnoreCase(ROMAJI_TABLE[i].romaji)) {
      return String(ROMAJI_TABLE[i].kana);
    }
  }
  return "";
}

// 删除 UTF-8 字符串末尾的一个“完整字符”
void removeLastUTF8Char(String &s) {
  int len = s.length();
  if (len == 0) return;

  // UTF-8 末尾字节判断（10xxxxxx = continuation）
  int i = len - 1;
  while (i >= 0 && ((s[i] & 0b11000000) == 0b10000000)) {
    i--;
  }

  // i 现在停在字符的起始字节
  s.remove(i);
}
