#!/usr/bin/env bash
# Report the Vulkan driver Unreal will use, and whether it implements the
# extension UE 5.8 requires.
#
# UE 5.8 calls vkCmdSetRenderingInputAttachmentIndicesKHR on every render pass.
# That entry point belongs to VK_KHR_dynamic_rendering_local_read (Vulkan 1.4),
# which Mesa implements from 24.2 onwards. On an older driver the pointer is
# null and the RHI thread segfaults on the first frame.

set -euo pipefail

# shellcheck source=/dev/null
source "$(dirname -- "${BASH_SOURCE[0]}")/common.sh"

if ! command -v vulkaninfo >/dev/null 2>&1; then
    echo "vulkaninfo not found. Install vulkan-tools to run this check." >&2
    exit 2
fi

if [[ -n "${MESA_PREFIX:-}" ]]; then
    echo "MESA_PREFIX : $MESA_PREFIX"
    echo "ICD         : ${VK_DRIVER_FILES:-<not exported>}"
else
    echo "MESA_PREFIX : <unset, using the system driver>"
fi

info="$(vulkaninfo 2>/dev/null || true)"

driver="$(printf '%s\n' "$info" | grep -m1 -E "^\s+driverInfo" | sed 's/.*= *//')"
echo "driverInfo  : ${driver:-unknown}"

if printf '%s\n' "$info" | grep -q "VK_KHR_dynamic_rendering_local_read"; then
    echo "local_read  : present"
    echo
    echo "This driver satisfies UE 5.8's render pass requirement."
else
    echo "local_read  : MISSING"
    echo
    echo "VK_KHR_dynamic_rendering_local_read is absent. Unreal will segfault on" >&2
    echo "the first render pass. Build a newer driver with:" >&2
    echo "    Scripts/build_local_mesa.sh" >&2
    exit 1
fi
