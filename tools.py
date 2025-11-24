import os
import json
import requests
import binascii
from typing import List, Tuple
from pathlib import Path
from dotenv import load_dotenv


load_dotenv()  # 默认会在当前目录和父目录寻找 .env
api_key = os.getenv("API_KEY")

def generate_tts(
    text: str,
    output_path: Path = Path("output.wav"),
    model: str = "speech-2.6-turbo",
    voice_id: str = "Japanese_DecisivePrincess",
    language: str = "Japanese",
    sample_rate: int = 32000,
    bitrate: int = 128000,
    pitch: int = 0,
    speed: float = 1.0,
    volume: float = 1.0,
    emotion: str = "calm",
) -> Tuple[bool, str]:
    """
    使用 MiniMax T2A 接口生成语音文件 (WAV 格式)

    :param text (str): 要转换的文本
    :param output_path (str): 输出文件路径 (默认 'output.wav')
    :param model (str): 使用的模型 (默认 speech-2.6-turbo)
    :param voice_id (str): 声音ID (默认 Japanese_DecisivePrincess)
    :param language (str): 语言增强设置 (默认 Japanese)
    :param sample_rate (int): 采样率 (默认 32000)
    :param bitrate (int): 比特率 (默认 128000)
    :param pitch (int): 音高调整 (范围 -12 ~ 12)
    :param speed (float): 语速 (0.5 ~ 2.0)
    :param volume (float): 音量 (0.1 ~ 10)
    :param emotion (str): 语音情感，可选 [happy, calm, sad, angry...]
    :retutn [bool, str]: 输出文件路径或错误信息
    """

    url = "https://api.minimax.io/v1/t2a_v2"

    payload = {
        "model": model,
        "text": text,
        "stream": False,
        "output_format": "hex",
        "voice_setting": {
            "voice_id": voice_id,
            "speed": speed,
            "vol": volume,
            "pitch": pitch,
            "emotion": emotion
        },
        "audio_setting": {
            "sample_rate": sample_rate,
            "bitrate": bitrate,
            "format": "wav",
            "channel": 1
        },
        "language_boost": language
    }

    headers = {
        "Authorization": f"Bearer {api_key}",
        "Content-Type": "application/json"
    }
    output_path = Path(output_path)

    try:
        response = requests.post(url, headers=headers, json=payload, timeout=60)
        result = response.json()

        # 检查返回码
        if result.get("base_resp", {}).get("status_code") == 0:
            audio_hex = result["data"]["audio"]
            audio_bytes = binascii.unhexlify(audio_hex)

            # 自动创建文件夹
            output_path.parent.mkdir(parents=True, exist_ok=True)

            # 保存音频
            with output_path.open("wb") as f:
                f.write(audio_bytes)

            return True, str(output_path)

        else:
            msg = result.get("base_resp", {}).get("status_msg", "未知错误")
            return False, msg

    except Exception as e:
        return False, str(e)
    

def extract_jp_fields(json_path: str) -> List[str]:
    """
    从指定 JSON 文件中提取所有 'jp' 字段，返回一个字符串列表。

    :param parmjson_path (str): JSON 文件路径
    :return List[str]: 所有 jp 字段组成的列表
    """
    with open(json_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    
    # 确保 data 是列表
    if not isinstance(data, list):
        raise ValueError("JSON 文件的顶层结构必须是列表。")

    jp_list = [item["jp"] for item in data if isinstance(item, dict) and "jp" in item]
    return jp_list


def extract_all_jp_from_folder(folder_path: Path) -> List[str]:
    """
    遍历文件夹下所有 JSON 文件，提取 jp 字段并去重返回。
    
    :param folder_path (Path): 文件夹路径
    :return List[str]: 所有 jp 字段组成的列表
    """
    folder_path = Path(folder_path)
    if not folder_path.is_dir():
        raise NotADirectoryError(f"{folder_path} 不是有效的文件夹路径。")

    all_jp = []

    for json_file in folder_path.glob("*.json"):
        try:
            jp_list = extract_jp_fields(json_file)
            all_jp.extend(jp_list)
        except Exception as e:
            print(f"读取文件 {json_file} 时出错: {e}")

    # 去重并排序（可选）
    return sorted(set(all_jp))


def list_wav_filenames(folder_path: Path) -> List[str]:
    """
    输入文件夹路径，返回其中所有 .wav 文件的文件名列表（不含路径）。
    """
    folder_path = Path(folder_path)

    if not folder_path.is_dir():
        raise NotADirectoryError(f"{folder_path} 不是有效的文件夹。")

    return [wav_file.stem for wav_file in folder_path.glob("*.wav")]


def merge_json_folder(folder_path):
    folder = Path(folder_path)
    all_entries = {}  # keyed by jp

    # 读取所有 json 文件，累积大词典
    for json_file in folder.glob("*.json"):
        with open(json_file, "r", encoding="utf-8") as f:
            try:
                data = json.load(f)
            except Exception as e:
                print(f"Error reading {json_file}: {e}")
                continue

        for entry in data:
            jp = entry.get("jp")
            if not jp:
                continue

            if jp not in all_entries:
                all_entries[jp] = dict(entry)
            else:
                merged = all_entries[jp]
                for key, val in entry.items():
                    if key not in merged:
                        merged[key] = val
                        continue

                    old = merged[key]
                    if key == "score":
                        try:
                            merged[key] = min(old, val)
                        except:
                            merged[key] = old
                    elif key == "tone":
                        try:
                            merged[key] = val if old == val else -1
                        except:
                            merged[key] = -1
                    else:
                        if old != val and val not in old.split("; "):
                            merged[key] = f"{old}; {val}"
                all_entries[jp] = merged

    # 用合并后的大词典更新每个文件的内容
    for json_file in folder.glob("*.json"):
        with open(json_file, "r", encoding="utf-8") as f:
            data = json.load(f)

        new_list = []
        for entry in data:
            jp = entry.get("jp")
            if not jp:
                new_list.append(entry)
                continue

            merged = all_entries.get(jp, {})
            # 按原顺序更新字段
            new_entry = {}
            for key in entry.keys():
                if key in merged:
                    new_entry[key] = merged[key]
            # 加入额外字段（保持出现顺序：原字段优先，新增字段后）
            for key, val in merged.items():
                if key not in new_entry:
                    new_entry[key] = val

            new_list.append(new_entry)

        with open(json_file, "w", encoding="utf-8") as f:
            json.dump(new_list, f, ensure_ascii=False, indent=4)

    return all_entries


def filter_json_by_jp_difference(path_a, path_b):
    """
    path_a: 第一个 json 文件路径
    path_b: 第二个 json 文件路径
    """
    path_a = Path(path_a)
    path_b = Path(path_b)

    # 取两个文件的 jp 列表
    jp_a = set(extract_jp_fields(path_a))
    jp_b = set(extract_jp_fields(path_b))

    # 只保留 A 中独有的 JP
    unique_jp = jp_a - jp_b

    # 读取 A 的原始内容
    with open(path_a, "r", encoding="utf-8") as f:
        data_a = json.load(f)

    # 过滤
    filtered = [entry for entry in data_a if entry.get("jp") in unique_jp]

    # 写回 A
    with open(path_a, "w", encoding="utf-8") as f:
        json.dump(filtered, f, ensure_ascii=False, indent=4)

    return unique_jp