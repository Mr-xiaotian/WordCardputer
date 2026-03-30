import json
from pathlib import Path
from collections import Counter
from statistics import mean, median


def analyze_vocab_mastery(json_path: str | Path) -> dict:
    """
    分析词库 JSON 文件中的掌握程度（score）。

    :param json_path: JSON 文件路径
    :return: 统计结果 dict
    """
    json_path = Path(json_path)

    with json_path.open("r", encoding="utf-8") as f:
        words = json.load(f)

    scores = []
    for w in words:
        # 缺失 score → 默认 3
        score = w.get("score", 3)
        if not isinstance(score, (int, float)):
            score = 3
        scores.append(score)

    total = len(scores)
    if total == 0:
        return {
            "total_words": 0,
            "message": "词库为空"
        }

    counter = Counter(scores)

    avg_score = mean(scores)
    med_score = median(scores)

    # 各分值分布（1~5 都列出来,即使为 0）
    distribution = {
        s: {
            "count": counter.get(s, 0),
            "ratio": counter.get(s, 0) / total
        }
        for s in range(1, 6)
    }

    # 一个非常"直觉化"的总体评价
    if avg_score >= 4.5:
        level = "非常熟练"
    elif avg_score >= 3.8:
        level = "较为熟练"
    elif avg_score >= 3.0:
        level = "掌握中"
    elif avg_score >= 2.0:
        level = "不牢固"
    else:
        level = "需要重点复习"

    return {
        "file": str(json_path),
        "total_words": total,
        "average_score": round(avg_score, 3),
        "median_score": med_score,
        "distribution": distribution,
        "mastery_level": level,
    }
