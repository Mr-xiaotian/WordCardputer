import requests
import binascii
import os
import json
from typing import List

def generate_tts(
    api_key: str,
    text: str,
    output_path: str = "output.wav",
    model: str = "speech-2.6-turbo",
    voice_id: str = "Japanese_DecisivePrincess",
    language: str = "Japanese",
    sample_rate: int = 32000,
    bitrate: int = 128000,
    pitch: int = 0,
    speed: float = 1.0,
    volume: float = 1.0,
    emotion: str = "calm",
):
    """
    使用 MiniMax T2A 接口生成语音文件 (WAV 格式)

    :param api_key (str): 你的 MiniMax API 密钥
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
    :return bool: 是否生成成功
    :retutn str: 输出文件路径或错误信息
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

    try:
        response = requests.post(url, headers=headers, json=payload, timeout=60)
        result = response.json()

        # 检查返回码
        if result.get("base_resp", {}).get("status_code") == 0:
            audio_hex = result["data"]["audio"]
            audio_bytes = binascii.unhexlify(audio_hex)

            # 自动创建文件夹
            os.makedirs(os.path.dirname(output_path) or ".", exist_ok=True)

            with open(output_path, "wb") as f:
                f.write(audio_bytes)

            return True, output_path
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