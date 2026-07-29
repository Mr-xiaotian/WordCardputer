# Subagent Prompt: Mode 模式文档

## 区域说明

负责所有 `src/Mode*.cpp` / `src/Mode*.h` 的 `docs/zh-CN/` 文档同步。

## 审计重点

### 枚举依赖

Mode 文件严重依赖 `src/globals.h` 中 `AppMode` 枚举。当收到近期变更信息时，务必检查 `AppMode` 是否有新增值（如 `MODE_CLASSIFY_SELECT`, `MODE_WORD_TABLE`, `MODE_CLOCK` 等），并同步到对应文档的上下文描述中。

### ESC 菜单联动

ESC 菜单 (`ModeEscMenu.cpp`) 的菜单项结构、分组逻辑变化时，需要同步检查 `src/globals.cpp` 中的 `escRootItems`, `escVocabItems`, `escModeItems` 数组和 `EscMenuGroup` 枚举。

### 键盘帮助联动

`ModeKeyHelp.cpp` 引用 `src/globals.cpp` 中的 `helpSections` 数组。任何模式的按键操作变化都需要反映到对应帮助分类中。

### Mode 间跳转关系

当前 Mode 跳转链路（简化）：
```
MODE_SPLASH → MODE_LANG_SELECT → MODE_CLASSIFY_SELECT
  ├─ 按词源 → MODE_FILE_SELECT → MODE_STUDY
  └─ 按Score → MODE_SCORE_SELECT → MODE_STUDY
MODE_ESC_MENU → 子菜单分发到各模式
```

审计时需要关注：`previousMode` 的设置和恢复逻辑是否正确反映在文档中。

## 特殊文件注意事项

| 文件 | 易遗漏项 |
|------|---------|
| ModeStudy.cpp | 三页系统（单词/例句/词根词缀）；`studyPage`, `showMeaning`, `showSentenceZh`, `showRoots` 等状态变量的交互 |
| ModeDictationReview.cpp | 两种视图（错误对照/单词详情）；`reviewShowDetail` 状态；Del 删除功能 |
| ModeEscMenu.cpp | 三层分组（ROOT/VOCAB/MODE）、分组独立光标、分组间导航（`/` 进、`,` 退、`ESC` 退） |
| ModeKeyHelp.cpp | 动态分类系统：`HelpSectionData` 结构、`helpSections` 数组定义在 `globals.cpp` 中 |
| ModeListen.cpp | 当前界面仅显示 `en` + `phonetic`（英语），`jp` + `kanji`（日语），不再显示 `pos` |
