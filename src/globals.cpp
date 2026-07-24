/**
 * globals.cpp - 全局变量定义
 */

#include "globals.h"

// ---------- 核心状态 ----------
AppMode appMode = MODE_SPLASH;
AppMode previousMode = MODE_FILE_SELECT;

M5Canvas canvas(&M5Cardputer.Display);
const int visibleLines = 4;
int soundVolume = 192;
bool wifiConnected = false;

StudyLanguage currentLanguage = LANG_JP;
String currentWordRoot = "/words_study/jp/word";
String currentAudioRoot = "/words_study/jp/audio";
String currentSource = "";
String vocabLabel = "";

std::vector<String> langItems = {"日语", "英语"};
int langIndex = 0;
std::vector<String> classifyItems = {"按词源分类", "按Score分类"};
int classifyIndex = 0;

unsigned long volumeMessageDeadline = 0;

// ---------- 自动保存 ----------
bool scoresDirty = false;
int dirtyCount = 0;
int autoSaveThreshold = 5;

// ---------- 自动亮度管理 ----------
bool userAction = false;
unsigned long lastActivityTime = 0;
bool isDimmed = false;
unsigned long idleTimeout = 60000;
uint8_t normalBrightness = 200;
uint8_t dimBrightness = 40;
int loopDelay = 30;

// ---------- 词库数据 ----------
std::vector<Word> words;
int wordIndex = 0;

// ---------- 听写 ----------
std::vector<DictError> dictErrors;
std::vector<DictationReviewEntry> dictationReviewEntries;
int dictationReviewIndex = 0;
String dictationReviewTitle = "错误回顾";

// ---------- WiFi 配置 ----------
std::vector<WiFiCredential> savedWiFiList;

// --- ModeDictation ---
std::vector<int> dictOrder;
int dictPos = 0;
String commitText = "";
String romajiBuffer = "";
String candidateKana = "";
String dictEnInput = "";
int correctCount = 0;
int wrongCount = 0;
bool dictShowSummary = false;
bool useKatakana = false;

// --- ModeEscMenu ---
std::vector<String> escRootItems = {
    "学习统计",
    "当前词表",
    "词库相关 >",
    "模式切换 >",
    "查看过往错题",
    "按键帮助",
    "WiFi 连接",
};
std::vector<String> escVocabItems = {
    "重新选择语言",
    "重新选择分类",
    "重新选择词源",
};
std::vector<String> escModeItems = {
    "进入学习模式",
    "进入听读模式",
    "进入听写模式",
};
EscMenuGroup escMenuGroup = ESC_MENU_ROOT;
int escRootIndex = 0;
int escRootScroll = 0;
int escVocabIndex = 0;
int escVocabScroll = 0;
int escModeIndex = 0;
int escModeScroll = 0;

// --- ModeKeyHelp ---
std::vector<HelpSectionData> helpSections = {
    {
        "按键帮助",
        {
            {
                { "; / .",   "切换帮助分类" },
                { ", / /",   "当前分类翻页" },
            }
        }
    },
    {
        "通用",
        {
            {
                { "ESC(`)",  "打开/关闭菜单" },
            }
        }
    },
    {
        "学习模式",
        {
            {
                { "BtnA",    "单词页翻卡/例句中切换中译" },
                { "Enter",   "记住(score+1)" },
                { "Del",     "不熟(score-1)" },
            },
            {
                { ", /",     "左右循环切换页面" },
                { "; / .",   "音量加/减" },
                { "Fn",      "播放发音" },
            }
        }
    },
    {
        "听写模式",
        {
            {
                { "字母键",  "输入答案" },
                { "Enter",   "提交答案" },
                { "Del",     "删除字符" },
            },
            {
                { "Shift",   "平/片假名切换" },
                { ";",       "确认当前假名" },
                { "Fn",      "重播当前发音" },
            },
            {
                { "; / .",   "音量加/减" },
                { "Fn",      "播放发音" },
            }
        }
    },
    {
        "当前词表",
        {
            {
                { "; / .",   "切换分数组" },
                { ", / /",   "当前分组翻页" },
            }
        }
    },
    {
        "错题回顾",
        {
            {
                { ", / /",   "切换错题" },
                { "Fn",      "重播正确发音" },
            }
        }
    },
};
int helpSectionIndex = 0;
int helpPageIndex = 0;

// --- ModeListen ---
int listenPlayCount = 0;
unsigned long listenNextActionTime = 0;
const unsigned long listenRepeatInterval = 1200;
const unsigned long listenNextWordDelay = 600;

// --- ModeScoreSelect ---
int scoreLevel = 0;
int scoreListIndex = 0;
int scoreListScroll = 0;
int groupListIndex = 0;
int groupListScroll = 0;
int selectedScore = 0;
int scoreWordCounts[6] = {0};

// --- ModeSourceSelect ---
std::vector<String> files;
std::vector<bool> fileExpandable;
String selectedSource = "";
String selectedChapter = "";
int fileIndex = 0;
int fileScroll = 0;

// --- ModeStats ---
int statsTotal = 0;
float statsAvg = 0;
float statsMedian = 0;
int statsCounts[6] = {0, 0, 0, 0, 0, 0};
String statsLevel = "";
int statsPage = 0;

// --- ModeStudy ---
int studyPage = 0;
bool showMeaning = false;
bool showSentenceZh = false;
bool showAnkiSideA = true;
bool showRoots = true;

// --- ModeWiFiScan ---
WiFiScanState wifiScanState = WIFI_SCANNING;
std::vector<String> wifiSSIDs;
std::vector<String> wifiRawSSIDs;
int wifiListIndex = 0;
int wifiListScroll = 0;
String wifiSelectedSSID = "";
String wifiPasswordInput = "";
bool wifiConnectSuccess = false;
int wifiPage = 0;

// --- ModeWordTable ---
int wordTableScore = 1;
int wordTablePage = 0;
const int wordTableRowsPerPage = 3;
std::vector<int> wordTableFilteredIndices;

// --- UtilsWebServer ---
WebServer server(80);
bool webServerRunning = false;
File uploadFile;
String uploadTempPath = "";
String uploadTargetSource = "";
String uploadTargetChapter = "";
String uploadError = "";
int uploadImportedCount = 0;
