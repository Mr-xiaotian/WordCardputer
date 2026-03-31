# UtilsData.ino

## 概述
`UtilsData.ino` 是项目的**数据持久化与词库管理工具**模块，封装了与 SD 卡文件系统和 JSON 数据处理相关的所有核心操作。

## 核心内容
- **词库加载与解析 (`loadWordsFromJSON`)**：
  - 从 SD 卡读取 JSON 文件，通过 `ArduinoJson` 解析内容。
  - 根据当前的语言模式（日语或英语），动态加载对应的字段。
  - 初始化单词的熟练度（`score`）值。
- **词库保存与序列化 (`saveListToJSON`)**：
  - 将内存中的 `words` 列表序列化为格式化（Pretty）的 JSON 字符串。
  - 将包含最新学习进度（score）的数据写回 SD 卡，持久化用户学习成果。
- **抽词算法 (`pickWeightedRandom`)**：
  - 实现了基于熟练度的加权随机算法。
  - 核心逻辑：熟练度（score）越低的单词，其权重越高，被抽中复习的概率越大，从而实现针对性强化。
- **自动保存机制 (`markScoreDirty`, `autoSaveIfNeeded`)**：
  - 追踪用户评分的变更次数，当修改次数达到设定阈值时，自动将内存数据同步到 SD 卡，防止进度丢失。
- **错题本导出 (`saveDictationMistakesAsWordList`)**：
  - 在听写模式结束后，将答错的单词提取出来。
  - 在 SD 卡自动创建包含时间戳的独立错题 JSON 文件（例如存入 Mistake 文件夹），方便后续专门复习。

## 关联模块
- 为所有的学习模式（`ModeStudy`, `ModeListen`, `ModeDictation`）提供基础数据支撑。
- 依赖全局数据结构定义（在 `WordCardputer.ino` 中）。