# Ubuntu 22.04 workstation setup

## Active machine and target

Use the existing Ubuntu 22.04.5 i7-1360P laptop with 15 GiB usable RAM and Intel integrated graphics. Target a 120 × 120 m block at 720p/30 first. This is below Epic's recommended Linux development hardware; renderer viability must be measured. See [baseline](../../../performance/BASELINE.md).

**Do not start a full Unreal source build on the current disk.** Use an Epic precompiled Linux installed build, stored outside the game repository. [Epic's Linux quickstart](https://dev.epicgames.com/documentation/unreal-engine/linux-development-quickstart-for-unreal-engine) documents the account-gated download and toolchain setup.

## Storage and installed engine

1. Review [exact storage candidates](../../../phase-1/STORAGE_REVIEW.md). Delete only user-selected paths after verifying recoverability; no cleanup has been performed by the project tooling.
2. Sign in to Epic's Linux engine download page and select a precompiled UE5 release. Check compressed + unpacked sizes and keep an additional 40 GiB initial working reserve. This reserve is a project estimate, not a vendor requirement.
3. Download only when space fits; use `Scripts/check_workstation.py --archive /path/to/archive.zip --install-parent /existing/parent` to inspect extraction space before unpacking.
4. Unpack outside the repository. Run the distribution's `Engine/Build/BatchFiles/Linux/SetupToolchain.sh` if its documented setup requires it. Use the matching bundled toolchain, not arbitrary system Clang.
5. Run `python3 Scripts/configure_engine.py /absolute/engine/path`. This validates UE5, writes `.env`, and pins actual version/changelist metadata in `Config/UnrealVersion.json`. Commit the pin once selected. Build scripts reject a mismatched engine.

No engine version is claimed as installed before step 5. Future contributors use that pin, rather than silently adopting the newest release.

## Desktop graphics prerequisites

In a terminal on the actual desktop, install missing tools as needed:

```bash
sudo apt update
sudo apt install -y build-essential git git-lfs python3 libvulkan1 mesa-vulkan-drivers vulkan-tools mesa-utils unzip zip
```

Run:

```bash
python3 Scripts/check_workstation.py --engine-root /absolute/engine/path --desktop
```

Hardware Intel Vulkan must appear in `vulkaninfo --summary`. A software adapter does not pass. Do not install NVIDIA drivers for the observed Intel-only hardware. The agent's current environment lacks `/dev/dri` and display access; its failure does not prove the desktop GPU is unsupported.

## Build in this order

```bash
Scripts/test_host.sh
Scripts/build_editor.sh
Scripts/create_smoke_test_map.sh
Scripts/create_player_sandbox.sh
Scripts/build_game.sh
Scripts/package_game.sh
Scripts/launch_game.sh
```

The generators require the compiled runtime module. Both preserve existing map assets; to regenerate, explicitly delete the desired generated map in the editor first. Materials are reused if already present. Keep custom maps separate from generator-owned maps.

Build scripts limit UnrealBuildTool to two parallel compile actions. Close memory-heavy desktop applications before launching the editor; do not automatically terminate user processes. Run the editor and packaged performance captures separately.

Use `Scripts/launch_editor.sh` to inspect the generated sandbox. Follow [the acceptance run](../../../phase-1/PLAYTEST.md) before claiming playability. Configure backups using the Phase 0 scripts and a separate destination; backup storage is not included in engine disk headroom.
