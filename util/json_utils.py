import json
import math
from typing import List
from pathlib import Path


def load_json_list(json_path: Path) -> List[dict]:
    with open(json_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    if not isinstance(data, list):
        raise ValueError("JSON 文件的顶层结构必须是列表。")
    return data


def extract_field_values(json_path: Path, field: str) -> List[str]:
    """
    从指定 JSON 文件中提取指定字段,返回字符串列表。

    :param parmjson_path (Path): JSON 文件路径
    :param field (str): 目标字段名
    :return List[str]: 字段值列表
    """
    data = load_json_list(json_path)
    values = [item[field] for item in data if isinstance(item, dict) and field in item]
    return values


def extract_jp_fields(json_path: Path) -> List[str]:
    """
    从指定 JSON 文件中提取所有 'jp' 字段,返回一个字符串列表。
    """
    return extract_field_values(json_path, "jp")


def extract_en_fields(json_path: Path) -> List[str]:
    """
    从指定 JSON 文件中提取所有 'en' 字段,返回一个字符串列表。
    """
    return extract_field_values(json_path, "en")


def extract_all_values_from_folder(folder_path: Path, field: str) -> List[str]:
    """
    遍历文件夹下所有 JSON 文件,提取指定字段并去重返回。
    """
    folder_path = Path(folder_path)
    if not folder_path.is_dir():
        raise NotADirectoryError(f"{folder_path} 不是有效的文件夹路径。")

    all_values = []

    for json_file in folder_path.rglob("*.json"):
        try:
            values = extract_field_values(json_file, field)
            all_values.extend(values)
        except Exception as e:
            print(f"读取文件 {json_file} 时出错: {e}")

    # 去重并排序（可选）
    return sorted(set(all_values))


def extract_all_jp_from_folder(folder_path: Path) -> List[str]:
    """
    遍历文件夹下所有 JSON 文件,提取 jp 字段并去重返回。
    """
    return extract_all_values_from_folder(folder_path, "jp")


def extract_all_en_from_folder(folder_path: Path) -> List[str]:
    """
    遍历文件夹下所有 JSON 文件,提取 en 字段并去重返回。
    """
    return extract_all_values_from_folder(folder_path, "en")


def list_wav_filenames(folder_path: Path) -> List[str]:
    """
    输入文件夹路径,返回其中所有 .wav 文件的文件名列表（不含路径）。
    """
    folder_path = Path(folder_path)

    if not folder_path.is_dir():
        raise NotADirectoryError(f"{folder_path} 不是有效的文件夹。")

    return [wav_file.stem for wav_file in folder_path.glob("*.wav")]


def collect_merged_entries_by_key(
    folder_path,
    key_field: str = "jp",
    score_field: str = "score",
    tone_field: str | None = "tone",
):
    """
    扫描文件夹所有 JSON 文件,并构建合并后的大词典。
    返回 all_entries（dict）,键为指定字段。
    """
    folder = Path(folder_path)
    all_entries = {}

    for json_file in folder.rglob("*.json"):
        try:
            data = load_json_list(json_file)
        except Exception as e:
            print(f"Error reading {json_file}: {e}")
            continue

        for entry in data:
            key_value = entry.get(key_field)
            if not key_value:
                continue

            if key_value not in all_entries:
                all_entries[key_value] = dict(entry)
                continue

            merged = all_entries[key_value]
            for field, val in entry.items():
                if field == key_field:
                    continue
                if field not in merged:
                    merged[field] = val
                    continue

                old = merged[field]

                if score_field and field == score_field:
                    try:
                        merged[field] = max(old, val)
                    except Exception:
                        merged[field] = old

                elif tone_field and field == tone_field:
                    if old == val:
                        merged[field] = val
                    else:
                        if old != -1 and val == -1:
                            merged[field] = old
                        elif val != -1 and old == -1:
                            merged[field] = val
                        else:
                            merged[field] = -1

                else:
                    old_str = str(old) if old is not None else ""
                    val_str = str(val) if val is not None else ""

                    if old_str != val_str and val_str not in old_str.split("; "):
                        if old_str:
                            merged[field] = f"{old_str}; {val_str}"
                        else:
                            merged[field] = val_str

            all_entries[key_value] = merged

    return all_entries


def collect_merged_entries(folder_path):
    """
    扫描文件夹所有 JSON 文件,并构建合并后的大词典。
    返回 all_entries（dict）,键为 jp。
    """
    return collect_merged_entries_by_key(folder_path, key_field="jp", tone_field="tone")


def collect_merged_entries_en(folder_path):
    """
    扫描文件夹所有 JSON 文件,并构建合并后的大词典。
    返回 all_entries（dict）,键为 en。
    """
    return collect_merged_entries_by_key(folder_path, key_field="en", tone_field=None)


def apply_merge_and_rewrite_by_key(
    folder_path,
    key_field: str = "jp",
    score_field: str = "score",
    tone_field: str | None = "tone",
):
    """
    调用 collect_merged_entries_by_key,然后把结果写回每个 JSON。
    """
    folder = Path(folder_path)
    all_entries = collect_merged_entries_by_key(
        folder_path,
        key_field=key_field,
        score_field=score_field,
        tone_field=tone_field,
    )

    for json_file in folder.rglob("*.json"):
        data = load_json_list(json_file)

        new_list = []
        for entry in data:
            key_value = entry.get(key_field)
            if not key_value:
                new_list.append(entry)
                continue

            merged = all_entries.get(key_value, {})
            new_entry = {}

            for key in entry.keys():
                if key in merged:
                    new_entry[key] = merged[key]

            for key, val in merged.items():
                if key not in new_entry:
                    new_entry[key] = val

            new_list.append(new_entry)

        with open(json_file, "w", encoding="utf-8") as f:
            json.dump(new_list, f, ensure_ascii=False, indent=4)

    return all_entries


def apply_merge_and_rewrite(folder_path):
    """
    调用 collect_merged_entries,然后把结果写回每个 JSON。
    """
    return apply_merge_and_rewrite_by_key(folder_path, key_field="jp", tone_field="tone")


def apply_merge_and_rewrite_en(folder_path):
    """
    调用 collect_merged_entries_en,然后把结果写回每个 JSON。
    """
    return apply_merge_and_rewrite_by_key(folder_path, key_field="en", tone_field=None)


def filter_json_by_key_difference(path_a, path_b, key_field: str = "jp"):
    """
    path_a: 第一个 json 文件路径
    path_b: 第二个 json 文件路径
    """
    path_a = Path(path_a)
    path_b = Path(path_b)

    key_a = set(extract_field_values(path_a, key_field))
    key_b = set(extract_field_values(path_b, key_field))

    unique_keys = key_a - key_b

    data_a = load_json_list(path_a)

    filtered = [entry for entry in data_a if entry.get(key_field) in unique_keys]

    with open(path_a, "w", encoding="utf-8") as f:
        json.dump(filtered, f, ensure_ascii=False, indent=4)

    return unique_keys


def filter_json_by_jp_difference(path_a, path_b):
    """
    path_a: 第一个 json 文件路径
    path_b: 第二个 json 文件路径
    """
    return filter_json_by_key_difference(path_a, path_b, key_field="jp")


def filter_json_by_en_difference(path_a, path_b):
    """
    path_a: 第一个 json 文件路径
    path_b: 第二个 json 文件路径
    """
    return filter_json_by_key_difference(path_a, path_b, key_field="en")


def dedupe_json_by_key(folder_path, key_field: str = "jp"):
    folder = Path(folder_path)

    for json_file in folder.glob("*.json"):
        try:
            data = load_json_list(json_file)
        except Exception as e:
            print(f"无法读取 {json_file}: {e}")
            continue

        seen = set()
        deduped = []

        for item in data:
            key_value = item.get(key_field)
            if not key_value:
                deduped.append(item)
                continue
            if key_value not in seen:
                seen.add(key_value)
                deduped.append(item)

        try:
            json_file.write_text(
                json.dumps(deduped, ensure_ascii=False, indent=4),
                encoding="utf-8"
            )
            print(f"已去重并写回：{json_file}")
        except Exception as e:
            print(f"写入失败 {json_file}: {e}")


def dedupe_json_by_jp(folder_path):
    return dedupe_json_by_key(folder_path, key_field="jp")


def dedupe_json_by_en(folder_path):
    return dedupe_json_by_key(folder_path, key_field="en")


def split_json_file(file_path: Path, max_per_file=60):
    """读取一个 JSON 文件,如果数量 > max_per_file,则进行均匀拆分。"""

    with open(file_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    total = len(data)
    if total <= max_per_file:
        print(f"✔ {file_path.name}: {total} 个词,不需要拆分。")
        return

    # 计算拆分份数,例如 220 -> 4 份
    parts = math.ceil(total / max_per_file)

    # 尽量平均,例如 220 / 4 = 55
    per_file = math.ceil(total / parts)

    print(f"⚡ {file_path.name}: {total} 个词,拆成 {parts} 份,每份 ~{per_file} 个")

    parent = file_path.parent
    base_name = file_path.stem  # 去掉 .json 后缀

    # 执行拆分
    for i in range(parts):
        start = i * per_file
        end = start + per_file
        chunk = data[start:end]

        out_path = parent / f"{base_name}_part{i+1}.json"
        with open(out_path, "w", encoding="utf-8") as out:
            json.dump(chunk, out, ensure_ascii=False, indent=2)

        print(f"  → 写入 {out_path.name}: {len(chunk)} 个词")

    print("完成。")


def process_folder(folder_path: str, max_per_file=60):
    """处理整个文件夹,拆分其中所有 json 文件。"""
    folder = Path(folder_path)

    if not folder.exists():
        print("路径不存在:", folder)
        return

    json_files = list(folder.glob("*.json"))
    if not json_files:
        print("没有找到 JSON 文件:", folder)
        return

    print(f"在 {folder} 中找到 {len(json_files)} 个 JSON 文件\n")

    for file in json_files:
        split_json_file(file, max_per_file=max_per_file)
