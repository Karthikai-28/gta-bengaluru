# Unreal 5.8.2 installation capacity check

Checked 2026-09-08 using HTTP range requests against the user-provided Epic download and reading the ZIP central directory. No full archive was downloaded and no engine files were extracted.

| Item | Size |
|---|---:|
| Linux Unreal Engine 5.8.2 ZIP | 37.08 GiB |
| Unpacked files (299,713 entries) | 71.68 GiB |
| Last measured free space after authorized cache cleanup | 45.76 GiB |
| Download plus unpacking, without working reserve | 108.76 GiB |
| Download plus unpacking plus 40 GiB planned reserve | 148.76 GiB |
| Additional free space for planned workflow | 103.00 GiB |

The unpacked engine alone does not fit the current filesystem, even if the archive is stored elsewhere. Do not start the full download here until sufficient capacity is available. No partial or stripped engine installation has been substituted.

Next step: free additional user-approved storage or select a sufficiently large SSD. A host mounted-drive inspection was rejected by automatic approval review because the workspace is out of credits. Restore workspace credits before retrying that operation.

The signed URL is temporary and is not stored in the repository. Obtain a fresh link if it expires before installation.

## Rescan — 2026-09-08 10:20 UTC

Host inspection now reports **111.8 GiB free** on the main ext4 filesystem. No additional mounted SSD was found. The full unpacked engine fits, leaving approximately 40.1 GiB after any downloaded archive is removed. Holding both the 37.08 GiB ZIP and the 71.68 GiB unpacked engine simultaneously would leave only about 3 GiB before filesystem overhead; prefer a verified streaming extraction workflow to preserve headroom.

The existing signed download URL was tested and returned HTTP 403 `Request has expired`. It expired at 08:28:28 UTC (13:58:28 IST). Obtain a fresh Epic Linux Unreal Engine 5.8.2 link before download/installation. No full download has started.

## Graphics driver deny list — 2026-09-08

The editor opens a blocking modal on startup: *"WARNING: Known issues with graphics driver — Installed: 23.2.1, Recommended: 26.0.0 or latest driver available"*.

This is UE 5.8's driver deny list, not a Vulkan failure. `Engine/Config/BaseHardware.ini` declares:

```
[GPU_Intel Linux]
SuggestedDriverVersion="26.0.0"
+DriverDenyList=(DriverVersion="<25.0.0", Reason="These driver versions have known stability issues and missing features")
```

The check runs in `RHIDetectAndWarnOfBadDrivers` (`Engine/Source/Runtime/RHI/Private/DynamicRHI.cpp`) and reports the Mesa version as the Intel driver version. The "Intel download center" URL in the dialog is hardcoded per vendor and does not apply to Linux/Mesa.

Host: Ubuntu 22.04.5, Intel Graphics (RPL-P, 0xa7a0), Mesa `23.2.1-1ubuntu3.1~22.04.4` — the newest Mesa in jammy-updates. Vulkan enumeration and direct rendering both pass; only the version test fails.

Mitigation applied: `r.WarnOfBadDrivers=0` in `[SystemSettings]` of `Config/DefaultEngine.ini`. This suppresses the dialog for editor and packaged runs; the deny-list warning is still written to the log.

### The deny list is describing a real, fatal fault — 2026-09-08 17:2x

Suppressing the dialog does not make the game run. Launching standalone
(`UnrealEditor NammaCity.uproject /Game/NammaCity/Maps/L_SmokeTest -game`) reaches
`Game Engine Initialized`, then dies on the RHI thread with SIGSEGV at a null
function pointer. Both maps fail identically, so it is not map or content related.
Backtrace under gdb:

```
#0  0x0000000000000000 in ()
#1  FVulkanCommandBuffer::BeginDynamicRendering(...) at VulkanCommandBuffer.cpp:181
#2  FVulkanCommandListContext::RHIBeginRenderPass(...) at VulkanRenderTarget.cpp:639
```

The null pointer is `VulkanRHI::vkCmdSetRenderingInputAttachmentIndicesKHR`, an entry
point of `VK_KHR_dynamic_rendering_local_read` (Vulkan 1.4). UE 5.8 calls it
unconditionally in the dynamic-rendering path. Mesa 23.2.1 does not implement that
extension — `vulkaninfo` reports zero occurrences and the system ANV library does not
export the symbol — so the pointer resolves to null and the first render pass crashes.
Mesa gained it in 24.2; `anv_physical_device.c:215` in 26.2.2 enables it.

The engine's SM5 target and the disabled Nanite/Lumen/VSM settings do not avoid this:
the crash is in the base render-pass path, hit by any frame.

Consequence: the Mesa upgrade is a prerequisite for running the project at all, not a
cosmetic fix. `r.WarnOfBadDrivers=0` stays because the dialog is an unactionable modal
on Linux (it links to Intel's Windows download center), but it does not make the
project runnable on Mesa 23.2.1.

Real driver upgrade is not available from packages on this release. `kisak-mesa` publishes Mesa 26.2.2 for noble (24.04) and nothing for jammy; jammy has no maintained source of Mesa 25+. Reaching a supported driver requires an Ubuntu 24.04 upgrade followed by the `kisak-mesa` PPA. The SM5 target does not sidestep the fault (see above). Remove the `r.WarnOfBadDrivers` line once the host reports Mesa 25.0.0 or newer.
