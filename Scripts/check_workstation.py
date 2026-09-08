#!/usr/bin/env python3
"""Read-only host/engine preflight; never deletes files or downloads an engine."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]
GIB = 1024 ** 3


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine-root", type=Path)
    parser.add_argument("--archive", type=Path, help="Epic installed-build ZIP; inspect unpacked size without extracting")
    parser.add_argument("--install-parent", type=Path, default=ROOT.parent)
    parser.add_argument("--desktop", action="store_true", help="Require successful hardware Vulkan enumeration")
    args = parser.parse_args()
    failures = []
    free = shutil.disk_usage(args.install_parent).free
    print(f"Free on {args.install_parent}: {free / GIB:.1f} GiB")
    print("Planning reserve: 40 GiB for project/toolchain/caches beyond the unpacked engine (estimate, not vendor requirement).")
    if args.archive:
        with zipfile.ZipFile(args.archive) as archive:
            unpacked = sum(item.file_size for item in archive.infolist())
        required = unpacked + 40 * GIB
        print(f"ZIP unpacked size: {unpacked / GIB:.1f} GiB; required free before extraction: {required / GIB:.1f} GiB")
        if free < required:
            failures.append("Insufficient free space for this archive plus working reserve")
    engine = args.engine_root or (Path(os.environ["UE_ROOT"]) if os.environ.get("UE_ROOT") else None)
    if not engine:
        failures.append("No engine configured; pass --engine-root or export UE_ROOT (this Python tool does not source .env)")
    else:
        for relative in ("Engine/Build/Build.version", "Engine/Binaries/Linux/UnrealEditor", "Engine/Build/BatchFiles/RunUAT.sh"):
            if not (engine / relative).is_file():
                failures.append(f"Missing {engine / relative}")
        version_file = engine / "Engine/Build/Build.version"
        if version_file.is_file():
            version = json.loads(version_file.read_text())
            print("Engine:", ".".join(str(version[k]) for k in ("MajorVersion", "MinorVersion", "PatchVersion")))
    if not Path("/dev/dri").exists():
        print("GPU device nodes unavailable in this environment; desktop verification remains open.")
        if args.desktop:
            failures.append("No /dev/dri device access")
    if args.desktop:
        if not shutil.which("vulkaninfo"):
            failures.append("vulkaninfo missing: install vulkan-tools in the desktop session")
        else:
            result = subprocess.run(["vulkaninfo", "--summary"], capture_output=True, text=True, timeout=45)
            print(result.stdout)
            if result.returncode != 0:
                failures.append("Vulkan enumeration failed: " + result.stderr[-1000:])
            elif not any(kind in result.stdout for kind in ("PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU", "PHYSICAL_DEVICE_TYPE_DISCRETE_GPU")):
                failures.append("No hardware GPU reported; software Vulkan does not pass")
    for failure in failures:
        print("BLOCKED:", failure)
    return 2 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
