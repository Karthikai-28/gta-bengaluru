#!/usr/bin/env python3
"""Track a person's 3D skeleton through a reference video with MediaPipe Pose.

Writes one .npz per video next to it (or under --out) holding, per frame, the
33 MediaPipe world landmarks (metres, hip-centred), the 33 normalised image
landmarks, their visibility and the frame timestamp. The strike extractor
(Scripts/extract_strike_motion.py) consumes these; the videos themselves are
never committed.

Usage: Scripts/track_reference_pose.py VIDEO... [--out DIR] [--model FILE]
"""
import argparse
import multiprocessing
import sys
import urllib.request
from pathlib import Path

MODEL_URL = ('https://storage.googleapis.com/mediapipe-models/pose_landmarker/'
             'pose_landmarker_heavy/float16/latest/pose_landmarker_heavy.task')
PROJECT = Path(__file__).resolve().parents[1]
DEFAULT_MODEL = PROJECT / 'Saved' / 'MotionReference' / 'pose_landmarker_heavy.task'


def ensure_model(path: Path) -> Path:
    if not path.is_file():
        path.parent.mkdir(parents=True, exist_ok=True)
        print(f'downloading pose model to {path}', flush=True)
        urllib.request.urlretrieve(MODEL_URL, path)
    return path


def track(video: Path, out_dir: Path, model: Path, width: int = 640) -> Path:
    import cv2
    import numpy as np
    import mediapipe as mp
    from mediapipe.tasks import python as mp_python
    from mediapipe.tasks.python import vision

    options = vision.PoseLandmarkerOptions(
        base_options=mp_python.BaseOptions(model_asset_path=str(model)),
        running_mode=vision.RunningMode.VIDEO,
        num_poses=1,
        min_pose_detection_confidence=0.5,
        min_pose_presence_confidence=0.5,
        min_tracking_confidence=0.5,
        output_segmentation_masks=False)
    capture = cv2.VideoCapture(str(video))
    if not capture.isOpened():
        raise SystemExit(f'cannot open {video}')
    fps = capture.get(cv2.CAP_PROP_FPS) or 30.0
    world, image, visibility, times = [], [], [], []
    index = 0
    with vision.PoseLandmarker.create_from_options(options) as landmarker:
        while True:
            ok, frame = capture.read()
            if not ok:
                break
            timestamp_ms = int(round(index * 1000.0 / fps))
            index += 1
            height = int(frame.shape[0] * width / frame.shape[1])
            small = cv2.resize(frame, (width, height), interpolation=cv2.INTER_AREA)
            rgb = cv2.cvtColor(small, cv2.COLOR_BGR2RGB)
            result = landmarker.detect_for_video(
                mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb), timestamp_ms)
            if result.pose_world_landmarks:
                w = result.pose_world_landmarks[0]
                i = result.pose_landmarks[0]
                world.append([[p.x, p.y, p.z] for p in w])
                image.append([[p.x, p.y, p.z] for p in i])
                visibility.append([p.visibility for p in w])
            else:
                world.append(np.full((33, 3), np.nan))
                image.append(np.full((33, 3), np.nan))
                visibility.append(np.zeros(33))
            times.append(index / fps)
            if index % 300 == 0:
                print(f'{video.name}: {index} frames', flush=True)
    capture.release()
    out_dir.mkdir(parents=True, exist_ok=True)
    out = out_dir / (video.stem + '.pose.npz')
    np.savez_compressed(out, world=np.array(world, dtype=np.float32),
                        image=np.array(image, dtype=np.float32),
                        visibility=np.array(visibility, dtype=np.float32),
                        time=np.array(times, dtype=np.float64), fps=fps, source=video.name)
    print(f'{video.name}: {index} frames -> {out}', flush=True)
    return out


def _job(args):
    video, out_dir, model = args
    return str(track(Path(video), Path(out_dir), Path(model)))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('videos', nargs='+', type=Path)
    parser.add_argument('--out', type=Path, default=None, help='output directory (default: beside each video)')
    parser.add_argument('--model', type=Path, default=DEFAULT_MODEL)
    parser.add_argument('--jobs', type=int, default=min(4, multiprocessing.cpu_count()))
    args = parser.parse_args()
    model = ensure_model(args.model)
    jobs = [(str(v), str(args.out or v.parent), str(model)) for v in args.videos]
    if len(jobs) == 1 or args.jobs <= 1:
        for job in jobs:
            _job(job)
    else:
        with multiprocessing.Pool(min(args.jobs, len(jobs))) as pool:
            for _ in pool.imap_unordered(_job, jobs):
                pass
    return 0


if __name__ == '__main__':
    sys.exit(main())
