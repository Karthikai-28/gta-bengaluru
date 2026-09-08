#!/usr/bin/env python3
"""Explicitly select an installed UE5 build and pin its Build.version identity."""
import argparse
import json
from pathlib import Path
import shlex
import sys

ROOT = Path(__file__).resolve().parents[1]
KEYS = ("MajorVersion", "MinorVersion", "PatchVersion", "Changelist", "CompatibleChangelist", "BranchName")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("engine_root", type=Path)
    parser.add_argument("--check", action="store_true", help="Verify the pin without changing files")
    args = parser.parse_args()
    engine = args.engine_root.resolve()
    source = engine / "Engine/Build/Build.version"
    if not source.is_file() or not (engine / "Engine/Binaries/Linux/UnrealEditor").is_file():
        parser.error("Expected an installed Unreal Engine with Build.version and UnrealEditor")
    version = json.loads(source.read_text())
    if version.get("MajorVersion") != 5:
        parser.error("This project requires Unreal Engine 5")
    identity = {key: version.get(key) for key in KEYS}
    pin = ROOT / "Config/UnrealVersion.json"
    if args.check:
        if not pin.is_file() or json.loads(pin.read_text()) != identity:
            print("Engine pin missing or different. Select the engine with Scripts/configure_engine.py.", file=sys.stderr)
            return 2
        return 0
    pin.write_text(json.dumps(identity, indent=2) + "\n")
    env = ROOT / ".env"
    lines = env.read_text().splitlines() if env.exists() else []
    lines = [line for line in lines if not line.startswith(("UE_ROOT=", "export UE_ROOT="))]
    lines.append("UE_ROOT=" + shlex.quote(str(engine)))
    env.write_text("\n".join(lines) + "\n")
    print("Configured .env and pinned Config/UnrealVersion.json. Review and commit the version pin.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
