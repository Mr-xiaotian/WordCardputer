# Subagent Prompt: Utils 工具文档

## 区域说明

负责所有 `src/Utils*.cpp` / `src/Utils*.h` 的 `docs/zh-CN/` 文档同步。

## 审计重点

### UtilsDb — 数据库访问层（变更最频繁）

`UtilsDb.cpp` 是项目变更最频繁的文件，每次审计必须额外关注：

1. **SQL 查询结构**：若有新增/删除列，需同步更新 `DataFormat.md`（即便不在本子任务范围，也应在报告中注明）。
2. **关联表**：`en_word_roots`、`en_word_affixes`、`en_roots`、`en_affixes` 四个表的 DDL 不在 C++ 源码中，仅可从 SQL 查询中推断。遇到不确定的字段约束时标注 `⚠️ 待确认`。
3. **journal 模式**：项目要求 DELETE 模式（单片机 sqlite 库不支持 WAL），文档中绝对不能出现 WAL 相关描述。
4. **函数重命名历史**：`loadWordsFromDB` → `loadWordsBySource`（源 code/chapter 加载）、`loadWordsByScore`（按分数加载）。确保文档使用当前函数名。

### UtilsTable — 独立模块

`drawSimpleTable()` 已从 `UtilsMenu` 中提取为独立模块 `UtilsTable`。审计 `UtilsMenu` 时确保文档中不再残留表格相关说明。

### UtilsConfig — 配置持久化

`config.json` 结构相对稳定。审计时关注：
- 是否有新增配置字段
- `loadAppConfig()` 的旧版兼容逻辑是否仍有残留
- 旧 `wifi.json` 迁移是否还在代码中

### 全局结构体字段变更

如果 `src/globals.h` 中 `Word` 结构体的字段变化（如新增 `sentence`, `sentenceZh`, `root`, `affix`），需要检查以下 Utils 是否受影响：
- `UtilsData.cpp` — 加权抽词、统计计算
- `UtilsDb.cpp` — SQL 查询中的列映射

## 特殊文件注意事项

| 文件 | 易遗漏项 |
|------|---------|
| UtilsDb.cpp | 函数 `loadWordsByIds`（按 ID 列表加载）、`deleteDictationError`（删除错题）、`loadRootAffixNames`（词根词缀名称查询）；`populateRootAffixFromJunction` 内部函数 |
| UtilsTable.cpp | 列宽均衡算法（水位线 + 截断降级）、单元格裁剪策略、`_rebalanceSimpleTableWidths` 的四种情况 |
| UtilsMenu.cpp | `navigateMenu` 和 `drawTextMenu` 两个核心函数；确认不再包含表格绘制逻辑 |
| UtilsString.cpp | `drawWrappedTextBlock`（自动换行文本块，含自动缩放逻辑）；`normalizeEnglishAnswer`（英语答案规范化） |
| UtilsWebServer.cpp | 路由注册、CORS 配置、文件上传/下载、路径解析函数 |