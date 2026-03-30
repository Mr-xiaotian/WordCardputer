import os
import wave
from typing import Tuple
from pathlib import Path


def trim_leading_silence_wav(
    wav_path: str | Path,
    output_path: str | Path | None = None,
    threshold_ratio: float = 0.015,
    keep_ms: int = 40,
    max_trim_ms: int = 800,
) -> Tuple[bool, str]:
    """
    裁剪 WAV 文件开头静音,保留少量前导缓冲避免爆破音丢失。
    """
    wav_path = Path(wav_path)
    if output_path is None:
        output_path = wav_path
    output_path = Path(output_path)

    try:
        with wave.open(str(wav_path), "rb") as wf:
            nchannels = wf.getnchannels()
            sampwidth = wf.getsampwidth()
            framerate = wf.getframerate()
            nframes = wf.getnframes()
            comptype = wf.getcomptype()
            compname = wf.getcompname()
            raw = wf.readframes(nframes)
    except Exception as e:
        return False, f"读取 WAV 失败: {e}"

    if nframes == 0 or len(raw) == 0:
        return False, "WAV 文件为空"
    if sampwidth not in (1, 2):
        return False, f"仅支持 8/16-bit PCM, 当前为 {sampwidth * 8}-bit"

    frame_size = sampwidth * nchannels
    max_abs = 127 if sampwidth == 1 else 32767
    threshold = max(1, int(max_abs * threshold_ratio))
    start_frame = 0
    found = False

    for i in range(nframes):
        base = i * frame_size
        peak = 0
        for c in range(nchannels):
            off = base + c * sampwidth
            if sampwidth == 1:
                v = raw[off] - 128
            else:
                v = int.from_bytes(raw[off:off + 2], "little", signed=True)
            av = abs(v)
            if av > peak:
                peak = av
        if peak >= threshold:
            start_frame = i
            found = True
            break

    if not found:
        return False, "未检测到有效发音段"

    keep_frames = max(0, int(framerate * keep_ms / 1000))
    max_trim_frames = max(0, int(framerate * max_trim_ms / 1000))
    trim_start = max(0, start_frame - keep_frames)
    trim_start = min(trim_start, max_trim_frames)

    if trim_start <= 0:
        return True, str(output_path)

    new_raw = raw[trim_start * frame_size:]
    output_path.parent.mkdir(parents=True, exist_ok=True)

    if output_path.resolve() == wav_path.resolve():
        tmp_path = output_path.with_suffix(output_path.suffix + ".tmp")
        target_path = tmp_path
    else:
        tmp_path = None
        target_path = output_path

    try:
        with wave.open(str(target_path), "wb") as wf:
            wf.setnchannels(nchannels)
            wf.setsampwidth(sampwidth)
            wf.setframerate(framerate)
            wf.setcomptype(comptype, compname)
            wf.writeframes(new_raw)
        if tmp_path is not None:
            os.replace(str(tmp_path), str(output_path))
    except Exception as e:
        return False, f"写入 WAV 失败: {e}"

    return True, str(output_path)


def trim_leading_silence_in_folder(
    folder_path: str | Path,
    recursive: bool = True,
    threshold_ratio: float = 0.015,
    keep_ms: int = 40,
    max_trim_ms: int = 800,
) -> dict:
    """
    批量裁剪文件夹内 WAV 文件的前导静音。
    """
    folder = Path(folder_path)
    if not folder.is_dir():
        raise NotADirectoryError(f"{folder} 不是有效文件夹")

    wav_files = list(folder.rglob("*.wav")) if recursive else list(folder.glob("*.wav"))
    summary = {
        "folder": str(folder),
        "total": len(wav_files),
        "success": 0,
        "failed": 0,
        "errors": [],
    }

    for wav_file in wav_files:
        ok, msg = trim_leading_silence_wav(
            wav_path=wav_file,
            output_path=wav_file,
            threshold_ratio=threshold_ratio,
            keep_ms=keep_ms,
            max_trim_ms=max_trim_ms,
        )
        if ok:
            summary["success"] += 1
        else:
            summary["failed"] += 1
            summary["errors"].append({"file": str(wav_file), "error": msg})

    return summary
