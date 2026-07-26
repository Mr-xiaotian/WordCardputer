import os
import binascii
import subprocess
import requests
from typing import Tuple
from pathlib import Path
import sqlite3
import time
from dotenv import load_dotenv


load_dotenv()
api_key = os.getenv("API_KEY")


def generate_tts_minimax(
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
    :param emotion (str): 语音情感,可选 [happy, calm, sad, angry...]
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


def generate_tts_youdao(text: str, output_path: str | Path) -> Tuple[bool, str]:
    output_path = Path(output_path)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    temp_mp3 = output_path.with_suffix(".mp3.tmp")

    try:
        headers = {
            "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36"
        }
        response = requests.get(
            "https://dict.youdao.com/dictvoice",
            params={"audio": text, "type": 2},
            headers=headers,
            timeout=10,
        )
        if response.status_code != 200 or len(response.content) <= 1000:
            return False, f"download failed (status: {response.status_code})"

        with temp_mp3.open("wb") as f:
            f.write(response.content)

        result = subprocess.run(
            [
                "ffmpeg",
                "-y",
                "-i",
                str(temp_mp3),
                "-ar",
                "16000",
                "-ac",
                "1",
                "-c:a",
                "pcm_s16le",
                str(output_path),
            ],
            capture_output=True,
            timeout=30,
        )
        temp_mp3.unlink(missing_ok=True)

        if result.returncode == 0 and output_path.exists():
            return True, str(output_path)
        err = result.stderr.decode(errors="ignore").strip()
        return False, err or "ffmpeg failed"
    except Exception as e:
        temp_mp3.unlink(missing_ok=True)
        return False, str(e)


def fill_missing_audio(
    db_path: str = "words_study/en/en_words.db",
    audio_dir: str | Path = "words_study/en/audio",
    delay: float = 0.5,
) -> Tuple[int, int]:
    """
    读取英语词库，为没有音频的单词生成有道TTS音频。

    :param db_path: SQLite 数据库路径
    :param audio_dir: 音频文件存放目录
    :param delay: 每次请求间隔（秒），避免被限流
    :return: (成功数, 失败数)
    """
    audio_dir = Path(audio_dir)
    audio_dir.mkdir(parents=True, exist_ok=True)

    # 读取已有的音频文件（不含扩展名）
    existing = {f.stem.lower() for f in audio_dir.iterdir() if f.suffix == ".wav"}
    print(f"已有音频: {len(existing)} 个")

    # 从数据库读取所有单词
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    cursor.execute("SELECT en FROM en_words ORDER BY id")
    all_words = [r[0].strip() for r in cursor.fetchall()]
    conn.close()

    print(f"词库总计: {len(all_words)} 个单词")

    # 筛选出缺少音频的
    missing = [w for w in all_words if w.lower() not in existing]
    print(f"缺少音频: {len(missing)} 个单词")

    if not missing:
        print("所有单词均有音频，无需处理。")
        return 0, 0

    success = 0
    fail = 0

    for i, word in enumerate(missing, 1):
        output_path = audio_dir / f"{word}.wav"
        ok, msg = generate_tts_youdao(word, output_path)

        if ok:
            success += 1
            print(f"[{i}/{len(missing)}] ✅ {word}")
        else:
            fail += 1
            print(f"[{i}/{len(missing)}] ❌ {word}: {msg}")

        if i < len(missing):
            time.sleep(delay)

    print(f"\n完成: 成功 {success}, 失败 {fail}")
    return success, fail

if __name__ == "__main__":
    # 生成所有缺失的音频
    success, fail = fill_missing_audio()
