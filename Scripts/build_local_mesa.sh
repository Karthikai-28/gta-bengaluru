#!/usr/bin/env bash
# Build a recent Mesa into a private prefix, for hosts whose distro Mesa is too
# old for Unreal Engine 5.8.
#
# Why this exists: UE 5.8 calls vkCmdSetRenderingInputAttachmentIndicesKHR
# (VK_KHR_dynamic_rendering_local_read, Vulkan 1.4) unconditionally on every
# render pass. Mesa gained that extension in 24.2. On an older Mesa the entry
# point resolves to null and the RHI thread segfaults on the first frame, before
# anything is drawn. Ubuntu 22.04 ships Mesa 23.2.1 and has no newer package, so
# the driver is built here instead.
#
# Nothing outside the prefix is modified: system Mesa, the desktop session and
# every other application keep using the distro driver. Point the project at
# this build by setting MESA_PREFIX in .env; remove it by deleting the prefix.

set -euo pipefail

MESA_VERSION="${MESA_VERSION:-26.2.2}"
LIBDRM_VERSION="${LIBDRM_VERSION:-2.4.134}"
GLSLANG_VERSION="${GLSLANG_VERSION:-16.5.0}"
SPIRV_TOOLS_VERSION="${SPIRV_TOOLS_VERSION:-v2026.3}"
PREFIX="${MESA_PREFIX:-$HOME/dev/mesa-26}"
JOBS="${JOBS:-$(nproc)}"
WORK_DIR="${WORK_DIR:-${TMPDIR:-/tmp}/mesa-build-$MESA_VERSION}"

usage() {
    cat <<USAGE
Usage: $0 [--prefix DIR] [--jobs N] [--work-dir DIR]

Builds libdrm $LIBDRM_VERSION, glslang $GLSLANG_VERSION, SPIRV-Tools
$SPIRV_TOOLS_VERSION and Mesa $MESA_VERSION (Intel Vulkan driver only) into a
private prefix.

  --prefix DIR     install destination (default: $PREFIX)
  --jobs N         parallel compile jobs (default: $JOBS)
  --work-dir DIR   source/build scratch space (default: $WORK_DIR)

Override versions with the MESA_VERSION, LIBDRM_VERSION and GLSLANG_VERSION
environment variables (SPIRV_TOOLS_VERSION too).
USAGE
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --prefix) PREFIX="$2"; shift 2 ;;
        --jobs) JOBS="$2"; shift 2 ;;
        --work-dir) WORK_DIR="$2"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown argument: $1" >&2; usage >&2; exit 2 ;;
    esac
done

# Build-time distribution packages. Mesa's Intel Vulkan driver pulls in CLC,
# which requires LLVM >= 15; Ubuntu 22.04 provides exactly 15, so no external
# APT repository is needed. glslang is built from source because Mesa wants
# >= 12.2 for its BVH shader preamble and jammy only packages 11.8.
REQUIRED_PACKAGES=(
    flex bison
    libxcb-dri3-dev libxcb-present-dev libxcb-sync-dev
    libxcb-randr0-dev libxcb-xfixes0-dev libxshmfence-dev
    llvm-15-dev llvm-15-tools libclang-15-dev libclang-cpp15-dev
    libllvmspirvlib-15-dev libclc-15-dev clang-15 spirv-tools
)

missing=()
for package in "${REQUIRED_PACKAGES[@]}"; do
    if ! dpkg-query -W -f='${Status}' "$package" 2>/dev/null | grep -q "install ok installed"; then
        missing+=("$package")
    fi
done
if [[ ${#missing[@]} -gt 0 ]]; then
    echo "Missing build dependencies. Install them with:" >&2
    echo "  sudo apt install ${missing[*]}" >&2
    exit 2
fi

mkdir -p "$WORK_DIR" "$PREFIX"
cd "$WORK_DIR"

# Ubuntu 22.04 packages Meson 0.61; Mesa 26 requires >= 1.4. A virtualenv keeps
# the newer Meson away from the system Python environment.
if [[ ! -x "$WORK_DIR/.venv/bin/meson" ]]; then
    python3 -m venv "$WORK_DIR/.venv"
    "$WORK_DIR/.venv/bin/pip" -q install --upgrade pip
    "$WORK_DIR/.venv/bin/pip" -q install meson mako pyyaml
fi
MESON="$WORK_DIR/.venv/bin/meson"

export PATH="$PREFIX/bin:$PATH"
export PKG_CONFIG_PATH="$PREFIX/lib/x86_64-linux-gnu/pkgconfig:$PREFIX/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"

fetch() {
    local url="$1" file="$2"
    [[ -f "$file" ]] || curl -sSL --fail -o "$file" "$url"
}

echo "==> libdrm $LIBDRM_VERSION"
fetch "https://dri.freedesktop.org/libdrm/libdrm-$LIBDRM_VERSION.tar.xz" "libdrm-$LIBDRM_VERSION.tar.xz"
[[ -d "libdrm-$LIBDRM_VERSION" ]] || tar xf "libdrm-$LIBDRM_VERSION.tar.xz"
(
    cd "libdrm-$LIBDRM_VERSION"
    rm -rf build
    # Only the core library is needed; the per-vendor modules would pull in
    # extra dependencies that Mesa's Intel Vulkan driver never uses.
    "$MESON" setup build --prefix="$PREFIX" --buildtype=release \
        -Dintel=disabled -Dradeon=disabled -Damdgpu=disabled -Dnouveau=disabled \
        -Dvmwgfx=disabled -Dman-pages=disabled -Dvalgrind=disabled \
        -Dtests=false -Dcairo-tests=disabled -Dudev=false
    "$MESON" compile -C build -j "$JOBS"
    "$MESON" install -C build
)

echo "==> glslang $GLSLANG_VERSION"
fetch "https://github.com/KhronosGroup/glslang/archive/refs/tags/$GLSLANG_VERSION.tar.gz" "glslang-$GLSLANG_VERSION.tar.gz"
[[ -d "glslang-$GLSLANG_VERSION" ]] || tar xf "glslang-$GLSLANG_VERSION.tar.gz"
(
    cd "glslang-$GLSLANG_VERSION"
    # ENABLE_OPT=OFF drops the SPIRV-Tools optimiser, which Mesa does not need
    # and which would otherwise have to be vendored as well.
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX" \
        -DENABLE_OPT=OFF -DGLSLANG_TESTS=OFF -DBUILD_SHARED_LIBS=OFF
    cmake --build build -j "$JOBS"
    cmake --install build
)

echo "==> SPIRV-Tools $SPIRV_TOOLS_VERSION"
# Mesa 26 needs SPIRV-Tools >= 2024.1 for its CLC pipeline; jammy packages
# 2022.2. Built statically with PIC so it links into the driver and needs no
# runtime library path of its own.
fetch "https://github.com/KhronosGroup/SPIRV-Tools/archive/refs/tags/$SPIRV_TOOLS_VERSION.tar.gz" "spirv-tools-$SPIRV_TOOLS_VERSION.tar.gz"
SPIRV_TOOLS_DIR="SPIRV-Tools-${SPIRV_TOOLS_VERSION#v}"
[[ -d "$SPIRV_TOOLS_DIR" ]] || tar xf "spirv-tools-$SPIRV_TOOLS_VERSION.tar.gz"
# SPIRV-Tools pins the exact SPIRV-Headers commit it was tested against in DEPS;
# using anything else risks opcode tables that disagree with the tools.
SPIRV_HEADERS_REV="$(sed -n "s/.*'spirv_headers_revision': *'\([0-9a-f]*\)'.*/\1/p" "$SPIRV_TOOLS_DIR/DEPS" | head -1)"
if [[ -z "$SPIRV_HEADERS_REV" ]]; then
    echo "Could not read spirv_headers_revision from $SPIRV_TOOLS_DIR/DEPS" >&2
    exit 1
fi
fetch "https://github.com/KhronosGroup/SPIRV-Headers/archive/$SPIRV_HEADERS_REV.tar.gz" "spirv-headers-$SPIRV_HEADERS_REV.tar.gz"
if [[ ! -d "$SPIRV_TOOLS_DIR/external/spirv-headers" ]]; then
    rm -rf "SPIRV-Headers-$SPIRV_HEADERS_REV"
    tar xf "spirv-headers-$SPIRV_HEADERS_REV.tar.gz"
    mkdir -p "$SPIRV_TOOLS_DIR/external"
    mv "SPIRV-Headers-$SPIRV_HEADERS_REV" "$SPIRV_TOOLS_DIR/external/spirv-headers"
fi
(
    cd "$SPIRV_TOOLS_DIR"
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX" \
        -DSPIRV_SKIP_TESTS=ON -DSPIRV_WERROR=OFF \
        -DBUILD_SHARED_LIBS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    cmake --build build -j "$JOBS"
    cmake --install build
)

# Pin the LLVM toolchain. A host often carries several (this one has 11, 14, 15
# and 20); Meson otherwise picks the newest llvm-config on PATH, and Mesa then
# demands an LLVMSPIRVLib of the same major version. Ubuntu 22.04 only packages
# LLVMSPIRVLib for 15, so an unpinned build fails at configure time on a machine
# that happens to have a newer LLVM installed for something else.
LLVM_VERSION="${LLVM_VERSION:-15}"
LLVM_CONFIG="/usr/bin/llvm-config-$LLVM_VERSION"
if [[ ! -x "$LLVM_CONFIG" ]]; then
    echo "llvm-config for LLVM $LLVM_VERSION not found at $LLVM_CONFIG" >&2
    exit 2
fi
cat > "$WORK_DIR/llvm-pin.ini" <<PIN
[binaries]
llvm-config = '$LLVM_CONFIG'
PIN

# Mesa 26's Intel backend uses C23 constructs (enums with a fixed underlying
# type). GCC only accepts those from 13 onwards and Ubuntu 22.04 tops out at 12,
# so the build needs a Clang. Any Clang from 15 up will do; prefer the newest
# installed. Only Mesa needs this - libdrm, glslang and SPIRV-Tools build fine
# with the system GCC.
if [[ -z "${MESA_CC:-}" ]]; then
    for candidate in clang-21 clang-20 clang-19 clang-18 clang-17 clang-16 clang-15; do
        if command -v "$candidate" >/dev/null 2>&1; then
            MESA_CC="$candidate"
            break
        fi
    done
fi
MESA_CXX="${MESA_CXX:-${MESA_CC/clang/clang++}}"
if [[ -z "${MESA_CC:-}" ]] || ! command -v "$MESA_CXX" >/dev/null 2>&1; then
    echo "No suitable Clang found. Mesa $MESA_VERSION needs Clang >= 15 (C23 enums)." >&2
    echo "Install one with: sudo apt install clang-15" >&2
    exit 2
fi

# Clang locates its C++ runtime through a GCC installation and picks the newest
# it finds. A host can easily have a newer GCC whose libstdc++ development files
# are absent (gcc-12 present without libstdc++-12-dev is a common Ubuntu state),
# and then linking fails with "cannot find -lstdc++". Pin Clang to the newest GCC
# directory that actually ships libstdc++.so.
MESA_CLANG_FLAGS=""
mapfile -t _gcc_dirs < <(printf '%s\n' /usr/lib/gcc/x86_64-linux-gnu/*/ | sort -Vr)
for _dir in "${_gcc_dirs[@]}"; do
    [[ -e "${_dir}libstdc++.so" ]] || continue
    if echo 'int main(){return 0;}' | "$MESA_CXX" --gcc-install-dir="${_dir%/}" -x c++ - -o /dev/null 2>/dev/null; then
        MESA_CLANG_FLAGS="--gcc-install-dir=${_dir%/}"
    fi
    break
done

echo "==> Mesa $MESA_VERSION (LLVM $LLVM_VERSION, $MESA_CC)"
fetch "https://archive.mesa3d.org/mesa-$MESA_VERSION.tar.xz" "mesa-$MESA_VERSION.tar.xz"
[[ -d "mesa-$MESA_VERSION" ]] || tar xf "mesa-$MESA_VERSION.tar.xz"
(
    cd "mesa-$MESA_VERSION"
    rm -rf build
    export CC="$MESA_CC" CXX="$MESA_CXX"
    export CFLAGS="${CFLAGS:-} $MESA_CLANG_FLAGS"
    export CXXFLAGS="${CXXFLAGS:-} $MESA_CLANG_FLAGS"
    export LDFLAGS="${LDFLAGS:-} $MESA_CLANG_FLAGS"
    # Vulkan only: Unreal renders through Vulkan on Linux and the desktop keeps
    # using the system OpenGL stack, so GL/EGL/GBM are left out. Ray tracing is
    # disabled because this targets integrated Intel parts without RT hardware.
    "$MESON" setup build --prefix="$PREFIX" --buildtype=release \
        --native-file "$WORK_DIR/llvm-pin.ini" \
        -Dvulkan-drivers=intel -Dgallium-drivers= -Dplatforms=x11 \
        -Dglx=disabled -Degl=disabled -Dgbm=disabled -Dopengl=false \
        -Dgles1=disabled -Dgles2=disabled \
        -Dvideo-codecs= -Dintel-rt=disabled \
        -Dvalgrind=disabled -Dlibunwind=disabled -Dbuild-tests=false
    "$MESON" compile -C build -j "$JOBS"
    "$MESON" install -C build
)

ICD="$PREFIX/share/vulkan/icd.d/intel_icd.x86_64.json"
if [[ ! -f "$ICD" ]]; then
    echo "Build finished but the Vulkan ICD manifest is missing: $ICD" >&2
    exit 1
fi

cat <<DONE

Mesa $MESA_VERSION installed to $PREFIX
Vulkan ICD: $ICD

Add this line to .env so the project's scripts use it:
    MESA_PREFIX=$PREFIX

Verify with:
    MESA_PREFIX=$PREFIX Scripts/check_vulkan_driver.sh
DONE
