# Linux GPU driver setup

Follow this once per workstation, in order. It takes about 20 minutes, most of
it compiling. Everything it installs lives in a private prefix; the system
driver and the desktop session are never modified.

## Who needs this

Anyone on Linux whose Mesa is older than **24.2**. Check first:

```bash
glxinfo -B | grep -i "opengl core profile version"
```

Mesa 24.2 or newer: skip this document entirely. Older: continue.

Ubuntu 22.04 users are always in the second group. Jammy's newest Mesa is
23.2.1 and there is no maintained backport — `kisak-mesa` publishes for noble
(24.04) and questing, nothing for jammy.

## Why

UE 5.8 calls `vkCmdSetRenderingInputAttachmentIndicesKHR` unconditionally in
`FVulkanCommandBuffer::BeginDynamicRendering`, on every render pass. That entry
point belongs to `VK_KHR_dynamic_rendering_local_read` (Vulkan 1.4), which Mesa
implements from 24.2 onward.

On an older Mesa the loader returns a null pointer and the engine calls it
anyway. The RHI thread dies on the first frame, before anything is drawn:

```
#0  0x0000000000000000 in ()
#1  FVulkanCommandBuffer::BeginDynamicRendering(...) VulkanCommandBuffer.cpp:181
#2  FVulkanCommandListContext::RHIBeginRenderPass(...) VulkanRenderTarget.cpp:639
```

The crash is in the base render-pass path, so no project setting avoids it. The
SM5 target and the disabled Nanite/Lumen/VSM options do not help. The engine's
own driver deny list (`Engine/Config/BaseHardware.ini`, `[GPU_Intel Linux]`)
already refuses Mesa below 25.0.0 for this class of problem, but its warning
dialog links to Intel's Windows download center and is unactionable on Linux, so
the project suppresses it with `r.WarnOfBadDrivers=0` in `Config/DefaultEngine.ini`.
Suppressing the dialog does not make the engine run — only a newer driver does.

## Steps

**1. Install the build prerequisites.**

```bash
sudo apt install flex bison \
    libxcb-dri3-dev libxcb-present-dev libxcb-sync-dev \
    libxcb-randr0-dev libxcb-xfixes0-dev libxshmfence-dev \
    llvm-15-dev llvm-15-tools libclang-15-dev libclang-cpp15-dev \
    libllvmspirvlib-15-dev libclc-15-dev clang-15 spirv-tools
```

All are stock jammy packages; no external APT repository is required. Mesa's
Intel Vulkan driver pulls in CLC, which needs LLVM >= 15, and jammy provides
exactly 15. Running the build script without these prints the same line, so
skipping this step is not fatal.

**2. Build the driver.**

```bash
Scripts/build_local_mesa.sh
```

Installs to `~/dev/mesa-26` by default; pass `--prefix DIR` to change it. The
script fetches and builds three things, in this order and for these reasons:

| Component | Version | Why not the distro package |
|---|---|---|
| Meson | latest, in a venv | jammy ships 0.61, Mesa needs >= 1.4 |
| libdrm | 2.4.134 | keeps the driver on a known-good DRM library |
| glslang | 16.5.0 | Mesa wants >= 12.2 for BVH shaders, jammy has 11.8 |
| Mesa | 26.2.2 | the driver itself |

Mesa is configured for Vulkan only (`-Dvulkan-drivers=intel`, no GL/EGL/GBM).
Unreal renders through Vulkan on Linux, so the desktop keeps using the system
OpenGL stack and the two never mix.

**3. Point the project at it.**

Add to `.env`:

```
MESA_PREFIX=/home/your-user/dev/mesa-26
```

`Scripts/common.sh` reads this and exports `VK_DRIVER_FILES`,
`VK_ICD_FILENAMES` and `LD_LIBRARY_PATH` for every Unreal process the project
starts — editor, standalone game and map-generation commandlets alike. Leave
`MESA_PREFIX` unset to fall back to the system driver.

**4. Verify.**

```bash
Scripts/check_vulkan_driver.sh
```

Expect `local_read : present` and a `driverInfo` naming the built Mesa version.
The check exits non-zero when the extension is missing, so it is safe to run in
a setup script.

## Reverting

```bash
rm -rf ~/dev/mesa-26        # delete the driver
```

and remove `MESA_PREFIX` from `.env`. Nothing else in the system changed. The
APT packages from step 1 are build-time only and can be removed with
`sudo apt remove` if wanted.

## The alternative

Upgrading the host to Ubuntu 24.04 and adding the `kisak-mesa` PPA reaches the
same Mesa system-wide, and is the better long-term answer. It is a release
upgrade with a reboot, so it is a scheduled task rather than a setup step. Once
a workstation is on Mesa 25.0.0 or newer, drop `MESA_PREFIX` from `.env` and
remove `r.WarnOfBadDrivers=0` from `Config/DefaultEngine.ini`.
