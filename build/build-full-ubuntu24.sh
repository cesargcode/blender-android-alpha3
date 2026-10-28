#!/usr/bin/env bash
# Build recipe for the supplied Alpha 3 patch. Run on Ubuntu 24.04 x86_64.
# This recipe has not been executed end-to-end in the Windows authoring environment.
set -Eeuo pipefail
umask 077
trap 'printf "Build interrompido na linha %s. A entrega do APK não foi concluída; consulte o log.\n" "$LINENO" >&2' ERR

die() { printf 'ERRO: %s\n' "$*" >&2; exit 1; }
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PATCH_FILE="$(realpath -- "$SCRIPT_DIR/../blender-alpha3-modificadores.patch")"
BASE_SHA=89479ec817bfe9411ae6ef6f35392d5450e1ad0b
HOST_LIB_SHA=ecbd06cf6d2a4aa6b00a61ffb479fc81b17aba08
ANDROID_LIB_SHA=59001bef2a4983d0f42e3281cf622a5734c1cb02
VULKAN_SHA=b5c8f996196ba4aa6d8f97e52b5d3b6e70f7e4e2
NDK_REVISION=30.0.14904198
JOBS="${JOBS:-2}"
MIN_FREE_GIB="${MIN_FREE_GIB:-100}"
SIGN_APK="${SIGN_APK:-0}"
[[ "$SIGN_APK" == 0 || "$SIGN_APK" == 1 ]] || die 'SIGN_APK deve ser 0 (unsigned) ou 1 (assinatura local).'

[[ "$(uname -s)" == Linux && "$(uname -m)" == x86_64 ]] || die 'Requer Linux x86_64.'
# shellcheck source=/dev/null
source /etc/os-release
[[ "$ID" == ubuntu && "$VERSION_ID" == 24.04 ]] || die 'Receita preparada para Ubuntu 24.04.'
[[ "$JOBS" =~ ^[1-9][0-9]*$ && "$MIN_FREE_GIB" =~ ^[1-9][0-9]*$ ]] || die 'JOBS e MIN_FREE_GIB devem ser inteiros positivos.'
[[ -s "$PATCH_FILE" ]] || die "Patch ausente: $PATCH_FILE"
for tool in git cmake ninja clang-18 clang++-18 patchelf zip unzip python3 sha256sum; do
  command -v "$tool" >/dev/null || die "Ferramenta ausente: $tool (consulte README.md)."
done
git lfs version >/dev/null || die 'Instale git-lfs.'
python3 -m pip --version >/dev/null || die 'Instale python3-pip para empacotar o pip do Blender.'
python3 - "$(cmake --version | sed -n '1s/.* //p')" <<'PY'
import sys
version = tuple(int(part) for part in sys.argv[1].split('.')[:2])
if version < (3, 26):
    raise SystemExit('CMake 3.26 ou superior é necessário.')
PY

export JAVA_HOME="${JAVA_HOME:-/usr/lib/jvm/java-17-openjdk-amd64}"
export ANDROID_HOME="${ANDROID_HOME:-$HOME/android-sdk}"
export ANDROID_NDK_VERSION="$NDK_REVISION"
export ANDROID_NDK_ROOT="$ANDROID_HOME/ndk/$NDK_REVISION"
export ANDROID_HOST_CC=clang-18 ANDROID_HOST_CXX=clang++-18
export ANDROID_ABI=arm64-v8a ANDROID_API=31 ANDROID_TARGET_API=34
export BLENDER_ANDROID_CONFIG=full
export PATH="$JAVA_HOME/bin:$ANDROID_HOME/platform-tools:$PATH"
[[ -x "$JAVA_HOME/bin/javac" && -x "$JAVA_HOME/bin/keytool" ]] || die 'JAVA_HOME precisa apontar para um JDK 17 completo.'
[[ "$("$JAVA_HOME/bin/javac" -version 2>&1)" == 'javac 17.'* ]] || die 'Esta receita fixa o JDK 17.'
[[ -f "$ANDROID_NDK_ROOT/source.properties" ]] || die 'Instale o pacote SDK ndk;30.0.14904198, sem o sufixo -beta1 no identificador.'
grep -Eq '^Pkg.Revision[[:space:]]*=[[:space:]]*30\.0\.14904198(-beta1)?[[:space:]]*$' \
  "$ANDROID_NDK_ROOT/source.properties" || die 'A revisão do NDK diverge da r30 beta1 exigida.'
[[ -f "$ANDROID_HOME/platforms/android-35/android.jar" ]] || die 'Falta platforms;android-35.'
BT="$ANDROID_HOME/build-tools/35.0.1"
for tool in aapt2 d8 zipalign apksigner; do
  [[ -x "$BT/$tool" ]] || die "Falta build-tools;35.0.1 ($tool)."
done
SIGNING_PASSWORD_VAR=""
if [[ "$SIGN_APK" == 1 ]]; then
: "${BLENDER_ANDROID_KEYSTORE:?Defina BLENDER_ANDROID_KEYSTORE com sua chave; veja README.md.}"
export BLENDER_ANDROID_KEYSTORE="$(realpath -- "$BLENDER_ANDROID_KEYSTORE")"
[[ -f "$BLENDER_ANDROID_KEYSTORE" ]] || die 'Keystore não encontrado.'
export BLENDER_ANDROID_KEY_ALIAS="${BLENDER_ANDROID_KEY_ALIAS:-blender-alpha3}"
# The upstream script interprets this variable as the NAME of a password variable.
# Read silently before tee redirects output; export only for the packaging step.
SIGNING_PASSWORD_VAR="${BLENDER_ANDROID_KEYSTORE_PASS:-}"
if [[ -z "$SIGNING_PASSWORD_VAR" ]]; then
  [[ -t 0 ]] || die 'Em execução sem terminal, configure BLENDER_ANDROID_KEYSTORE_PASS conforme README.md.'
  read -rsp 'Senha do keystore: ' BLENDER_ALPHA3_SIGNING_PASSWORD
  printf '\n'
  SIGNING_PASSWORD_VAR=BLENDER_ALPHA3_SIGNING_PASSWORD
fi
[[ "$SIGNING_PASSWORD_VAR" =~ ^[A-Za-z_][A-Za-z_0-9]*$ ]] || die 'BLENDER_ANDROID_KEYSTORE_PASS precisa ser um nome de variável.'
[[ -v "$SIGNING_PASSWORD_VAR" ]] || die 'A variável de senha indicada não está disponível no ambiente.'
fi
unset BLENDER_ANDROID_DEBUGGABLE BLENDER_ANDROID_FLAVOUR

TASK_DIR="${1:-$HOME/blender-alpha3-build}"
mkdir -p -- "$TASK_DIR"
TASK_DIR="$(realpath -- "$TASK_DIR")"
[[ "$TASK_DIR" != / ]] || die 'Escolha uma pasta exclusiva para o build.'
[[ "$TASK_DIR" != *[[:space:]]* && "$ANDROID_HOME" != *[[:space:]]* ]] || die 'A receita upstream requer caminhos de trabalho/SDK sem espaços.'
AVAILABLE_KIB="$(df -Pk -- "$TASK_DIR" | awk 'END {print $4}')"
(( AVAILABLE_KIB >= MIN_FREE_GIB * 1024 * 1024 )) || die "Reserve pelo menos $MIN_FREE_GIB GiB livres (margem de planejamento configurável, não tamanho medido)."
mkdir -p "$TASK_DIR/logs" "$TASK_DIR/output"
exec > >(tee -a "$TASK_DIR/logs/build-full.log") 2>&1
SRC="$TASK_DIR/source"
export BUILD_BASE="$TASK_DIR/blender_build_android"
HOST_BUILD="$BUILD_BASE/build_host_tools_full"
TARGET_BUILD="$BUILD_BASE/build_android_full"
export CMAKE_BUILD_PARALLEL_LEVEL="$JOBS"
export GIT_TERMINAL_PROMPT=0
PATCH_SHA="$(sha256sum "$PATCH_FILE" | cut -d ' ' -f1)"

printf 'Base: %s\nPatch SHA256: %s\nTrabalho: %s\n' "$BASE_SHA" "$PATCH_SHA" "$TASK_DIR"
record_resources() {
  printf '\n[resources] %s\n' "$1"
  date -u '+%Y-%m-%dT%H:%M:%SZ'
  free -b
  df -B1 --output=size,used,avail,target "$TASK_DIR"
  du -sx -B1 -- "$TASK_DIR"
}
timed_build() {
  if [[ -x /usr/bin/time ]]; then
    /usr/bin/time -v "$@"
  else
    "$@"
  fi
}
record_resources before-downloads
if [[ ! -d "$SRC/.git" ]]; then
  [[ ! -e "$SRC" ]] || die "A pasta $SRC já existe sem um repositório gerenciado por esta receita."
  git init "$SRC"
  git -C "$SRC" remote add origin https://github.com/simfeo/blender.git
  git -C "$SRC" config core.autocrlf false
  GIT_LFS_SKIP_SMUDGE=1 git -C "$SRC" fetch --depth 1 origin "$BASE_SHA"
  GIT_LFS_SKIP_SMUDGE=1 git -C "$SRC" checkout --detach "$BASE_SHA"
fi
[[ "$(git -C "$SRC" rev-parse HEAD)" == "$BASE_SHA" ]] || die 'O checkout existente está em outra base; use uma pasta de trabalho nova.'
git -C "$SRC" diff --cached --quiet || die 'Há alterações staged no checkout; preserve-as e use outra pasta.'
[[ -z "$(git -C "$SRC" ls-files --others --exclude-standard)" ]] || die 'Há arquivos não rastreados no checkout; use uma pasta de trabalho exclusiva.'

# Keep a copy of the exact first applied diff to detect edits during resumed builds.
if [[ -f "$TASK_DIR/applied-patch.sha256" ]]; then
  [[ "$(cat "$TASK_DIR/applied-patch.sha256")" == "$PATCH_SHA" ]] || die 'O patch mudou; use uma pasta nova para não misturar resultados.'
  git -C "$SRC" diff --binary --no-ext-diff > "$TASK_DIR/current-source.diff"
  cmp -s "$TASK_DIR/applied-source.diff" "$TASK_DIR/current-source.diff" || die 'O checkout foi editado após aplicar o patch; use uma pasta nova.'
else
  git -C "$SRC" diff --quiet || die 'Há alterações locais antes de aplicar o patch.'
  git -C "$SRC" apply --check "$PATCH_FILE"
  git -C "$SRC" apply "$PATCH_FILE"
  git -C "$SRC" diff --binary --no-ext-diff > "$TASK_DIR/applied-source.diff"
  printf '%s\n' "$PATCH_SHA" > "$TASK_DIR/applied-patch.sha256"
fi

configure_lfs() {
  git -C "$1" lfs install --local
  git -C "$1" config --local credential.https://projects.blender.org.helper \
    '!f() { printf "username=\npassword=\n"; }; f'
}
configure_lfs "$SRC"
git -C "$SRC" config --local lfs.url https://projects.blender.org/blender/blender.git/info/lfs
# Exclude only LFS payloads of upstream test data, not ordinary source files.
# fsck uses the same local exclusion; runtime/assets outside tests remain complete.
git -C "$SRC" config --local lfs.fetchinclude ''
git -C "$SRC" config --local lfs.fetchexclude 'tests/**'
git -C "$SRC" lfs pull --include='' --exclude='tests/**'
GIT_LFS_SKIP_SMUDGE=1 git -C "$SRC" -c submodule.lib/linux_x64.update=checkout \
  submodule update --init lib/linux_x64 lib/android_arm64
for lib in linux_x64 android_arm64; do
  configure_lfs "$SRC/lib/$lib"
  git -C "$SRC/lib/$lib" config --local lfs.fetchinclude ''
  git -C "$SRC/lib/$lib" config --local lfs.fetchexclude ''
  git -C "$SRC/lib/$lib" lfs pull --include='' --exclude=''
done
[[ "$(git -C "$SRC/lib/linux_x64" rev-parse HEAD)" == "$HOST_LIB_SHA" ]] || die 'Commit inesperado das bibliotecas Linux.'
[[ "$(git -C "$SRC/lib/android_arm64" rev-parse HEAD)" == "$ANDROID_LIB_SHA" ]] || die 'Commit inesperado das bibliotecas Android.'
for lib in linux_x64 android_arm64; do
  [[ -z "$(git -C "$SRC/lib/$lib" status --porcelain --untracked-files=normal)" ]] || die "As bibliotecas $lib têm alterações locais."
done
for repo in "$SRC" "$SRC/lib/linux_x64" "$SRC/lib/android_arm64"; do
  excluded=''
  if [[ "$repo" == "$SRC" ]]; then
    excluded='tests/**'
  fi
  git -C "$repo" -c "lfs.fetchexclude=$excluded" lfs fsck
  git -C "$repo" lfs ls-files --long --include='' --exclude="$excluded" > "$TASK_DIR/lfs-inventory.txt"
  if grep -E '^[0-9a-f]{64} - ' "$TASK_DIR/lfs-inventory.txt" > "$TASK_DIR/missing-lfs.txt"; then
    cat "$TASK_DIR/missing-lfs.txt"
    die "Há ponteiros LFS sem conteúdo em $repo."
  fi
done
record_resources after-lfs

VH="$BUILD_BASE/lib/vulkan_headers"
if [[ ! -d "$VH/.git" ]]; then
  [[ ! -e "$VH" ]] || die 'A pasta Vulkan-Headers existe sem um repositório verificável.'
  mkdir -p "$(dirname -- "$VH")"
  git init "$VH"
  git -C "$VH" remote add origin https://github.com/KhronosGroup/Vulkan-Headers.git
  git -C "$VH" fetch --depth 1 origin "$VULKAN_SHA"
  git -C "$VH" checkout --detach "$VULKAN_SHA"
fi
[[ "$(git -C "$VH" rev-parse HEAD)" == "$VULKAN_SHA" ]] || die 'Vulkan-Headers diverge da versão v1.4.341 fixada.'
git -C "$VH" diff --quiet || die 'Os headers Vulkan têm alterações locais.'
git -C "$VH" diff --cached --quiet || die 'Os headers Vulkan têm alterações staged.'

# Do not call build_apk.sh: at this tag it unsets ANDROID_HOME/ANDROID_NDK_ROOT.
# These are the documented equivalent steps, retaining the verified SDK paths.
cd "$SRC"
# shellcheck source=/dev/null
source build_files/android/env.sh
printf '\n[preflight] Compiling the patched Android activity with the packaging SDK/JDK.\n'
mkdir -p "$TASK_DIR/java-compile-check"
"$JAVA_HOME/bin/javac" -classpath "$ANDROID_HOME/platforms/android-35/android.jar" \
  -source 17 -target 17 -d "$TASK_DIR/java-compile-check" \
  "$SRC/build_files/android/apk/app/src/main/java/org/blender/blender/BlenderActivity.java"
cmake -S . -B "$HOST_BUILD" -G Ninja \
  -C build_files/android/android_features_full.cmake \
  -DWITH_CROSSCOMPILED_TOOLS=OFF \
  -DCMAKE_C_COMPILER="$ANDROID_HOST_CC" -DCMAKE_CXX_COMPILER="$ANDROID_HOST_CXX" \
  -DWITH_HEADLESS=ON -DWITH_X11_XINPUT=OFF -DWITH_AUDASPACE=OFF \
  -DCMAKE_BUILD_RPATH="$SRC/lib/linux_x64/tbb/lib"
timed_build ninja -j "$JOBS" -C "$HOST_BUILD" makesdna makesrna datatoc msgfmt shader_tool
record_resources after-host-tools

# Android also creates an ExternalProject at this distinct host_tools path.
# It forwards the target's WITH_* flags, including WITH_HEADLESS=OFF, on every
# configure. Keep a project hook in this native cache so those command-line
# flags cannot re-enable desktop input. Do not forward this hook to Android;
# audio and all other Full features retain the target's forwarded values.
HOST_OVERRIDE_FILE="$TASK_DIR/host-tools-overrides.cmake"
cat > "$HOST_OVERRIDE_FILE" <<'CMAKE'
if(ANDROID OR CMAKE_CROSSCOMPILING)
  message(FATAL_ERROR "Host code-generator overrides reached a cross-compiled target")
endif()
set(WITH_HEADLESS ON CACHE BOOL "Native code generators need no display" FORCE)
set(WITH_X11_XINPUT OFF CACHE BOOL "Native code generators need no XInput" FORCE)
CMAKE
cmake -S . -B "$TARGET_BUILD/host_tools" -G Ninja \
  -C build_files/android/android_features_full.cmake \
  -DWITH_CROSSCOMPILED_TOOLS=OFF \
  -DCMAKE_C_COMPILER="$ANDROID_HOST_CC" -DCMAKE_CXX_COMPILER="$ANDROID_HOST_CXX" \
  -DCMAKE_PROJECT_Blender_INCLUDE:FILEPATH="$HOST_OVERRIDE_FILE" \
  -DCMAKE_BUILD_RPATH="$SRC/lib/linux_x64/tbb/lib"

cmake -S . -B "$TARGET_BUILD" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_TOOLCHAIN_FILE" \
  -DANDROID_ABI="$ANDROID_ABI" -DANDROID_PLATFORM="android-$ANDROID_API" \
  -DHOST_C_COMPILER="$ANDROID_HOST_CC" -DHOST_CXX_COMPILER="$ANDROID_HOST_CXX" \
  -DBLENDER_ANDROID_CONFIG=full

# Compile all patched translation units before the rest of Blender. Resolve
# their object targets from this generated Ninja graph and reject missing or
# ambiguous matches instead of assuming a CMake object-directory layout.
ninja -C "$TARGET_BUILD" -t targets all > "$TASK_DIR/logs/ninja-targets.txt"
python3 - "$TASK_DIR/logs/ninja-targets.txt" "$TASK_DIR/changed-object-targets.txt" <<'PY'
from pathlib import Path
import sys

targets = [line.split(': ', 1)[0] for line in Path(sys.argv[1]).read_text().splitlines()]
sources = {
    'intern/ghost/': ['GHOST_SystemAndroid.cc', 'GHOST_AndroidMain.cc'],
    'source/blender/windowmanager/': [
        'wm_event_system.cc', 'wm_window.cc', 'wm_virtual_keyboard.cc'],
}
objects = []
for directory, names in sources.items():
    for name in names:
        matches = [target for target in targets
                   if target.startswith(directory) and target.endswith('/' + name + '.o')]
        if len(matches) != 1:
            raise SystemExit(f'Expected one Android object target for {name}; found {matches}')
        objects.append(matches[0])
Path(sys.argv[2]).write_text('\n'.join(objects) + '\n')
print('[preflight] Patched Android objects:\n' + '\n'.join(objects))
PY
mapfile -t PATCHED_OBJECT_TARGETS < "$TASK_DIR/changed-object-targets.txt"
timed_build ninja -j "$JOBS" -C "$TARGET_BUILD" "${PATCHED_OBJECT_TARGETS[@]}"
timed_build ninja -j "$JOBS" -C "$TARGET_BUILD" blender bf_intern_meshopt_bridge bf_intern_draco_bridge
record_resources after-android-build
if [[ "$SIGN_APK" == 1 ]]; then
  export BLENDER_ANDROID_KEYSTORE_PASS="$SIGNING_PASSWORD_VAR"
  export "$SIGNING_PASSWORD_VAR"
  BUILD="$TARGET_BUILD" bash build_files/android/apk/package.sh
else
  python3 "$SCRIPT_DIR/prepare-unsigned-package.py" \
    --source "$SRC/build_files/android/apk/package.sh" --output "$TASK_DIR/package-unsigned.sh"
  BLENDER_PACKAGE_SCRIPT_DIR="$SRC/build_files/android/apk" BUILD="$TARGET_BUILD" \
    bash "$TASK_DIR/package-unsigned.sh"
fi
unset BLENDER_ALPHA3_SIGNING_PASSWORD
record_resources after-packaging

APK="$BUILD_BASE/android_apk_stage_full/blender-full.apk"
[[ -s "$APK" ]] || die 'O empacotador não produziu o APK esperado.'
if [[ "$SIGN_APK" == 1 ]]; then
  "$BT/apksigner" verify --verbose --print-certs "$APK" | tee "$TASK_DIR/output/apk-signature.txt"
  APK_BASENAME=blender-alpha3-modificadores-full-arm64.apk
else
  if "$BT/apksigner" verify "$APK" >/dev/null 2>&1; then
    die 'O modo unsigned produziu um APK assinado inesperadamente.'
  fi
  printf 'UNSIGNED: requires local signing before Android installation.\n' > "$TASK_DIR/output/apk-signature.txt"
  APK_BASENAME=blender-alpha3-modificadores-full-arm64-unsigned.apk
fi
"$BT/zipalign" -c -p 4 "$APK"
"$BT/aapt2" dump badging "$APK" > "$TASK_DIR/output/apk-badging.txt"
grep -Fq "native-code: 'arm64-v8a'" "$TASK_DIR/output/apk-badging.txt" || die 'ABI inesperada no APK.'
grep -Fq "sdkVersion:'31'" "$TASK_DIR/output/apk-badging.txt" || die 'minSdk inesperado no APK.'
DEST="$TASK_DIR/output/$APK_BASENAME"
cp -- "$APK" "$DEST"
(
  cd "$TASK_DIR/output"
  sha256sum "$APK_BASENAME" > SHA256SUMS.txt
)
{
  printf 'source_commit=%s\npatch_sha256=%s\n' "$BASE_SHA" "$PATCH_SHA"
  printf 'linux_lib_commit=%s\nandroid_lib_commit=%s\nvulkan_headers_commit=%s\n' "$HOST_LIB_SHA" "$ANDROID_LIB_SHA" "$VULKAN_SHA"
  printf 'configuration=full\nabi=arm64-v8a\nmin_sdk=31\ntarget_sdk=34\ncompile_sdk=35\n'
  printf 'sign_apk=%s\nsource_lfs_exclude=tests/**\nparallel_jobs=%s\n' "$SIGN_APK" "$JOBS"
  cat "$ANDROID_NDK_ROOT/source.properties"
  cmake --version
  ninja --version
  clang-18 --version
  "$JAVA_HOME/bin/javac" -version
  printf 'device_runtime_test=NOT_PERFORMED\n'
} > "$TASK_DIR/output/build-manifest.txt" 2>&1
if [[ "$SIGN_APK" == 1 ]]; then
  printf '\nAPK compilado e assinatura verificada: %s\nAinda exige teste funcional no aparelho Android.\n' "$DEST"
else
  printf '\nAPK unsigned compilado: %s\nAssinatura local e teste no Android ainda pendentes.\n' "$DEST"
fi
