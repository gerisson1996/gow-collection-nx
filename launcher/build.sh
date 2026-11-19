#!/bin/bash
# build/gow-nx.nro: launcher com capa, música, menu de opções/idiomas, atualizador (GitHub Releases) e encadeamento via envSetNextLoad.
# A capa (bg.bgra, 1280x720 BGRA) e a música (music.mp3) padrão ficam em launcher/romfs/ (versionadas, embutidas pelo CI);
# também podem ser substituídas pela pasta do launcher no SD
# (tools/make_launcher_assets.ps1 gera o bg.bgra a partir de uma imagem). Sem elas o launcher abre com fundo preto e mudo.
# Precisa dos portlibs switch-freetype e switch-mpg123 (e switch-curl para habilitar o atualizador).
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
DKP=${DEVKITPRO:-/opt/devkitpro}; B=$DKP/devkitA64/bin; PL=$DKP/portlibs/switch; O=$ROOT/build/launcher
VERSION=${VERSION:-1.3}
UPDATE_REPO=${UPDATE_REPO:-gerisson1996/gow-collection-nx}
mkdir -p "$O" "$ROOT/launcher/romfs"
ARCH=(-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE)
CFLAGS=("${ARCH[@]}" -O2 -Wall -Wno-format-truncation -D__SWITCH__ "-DVERSAO=\"$VERSION\"" "-DUPDATE_REPO=\"$UPDATE_REPO\"" "-I$DKP/libnx/include" "-I$PL/include" "-I$PL/include/freetype2")

OBJS=("$O/main.o" "$O/trophies.o")
CURL_LIBS=()
if [ -f "$PL/lib/libcurl.a" ]; then
    CFLAGS+=(-DENABLE_UPDATER)
    PKGCFG=""
    if [ -x "$PL/bin/aarch64-none-elf-pkg-config" ]; then
        PKGCFG="$PL/bin/aarch64-none-elf-pkg-config"
    elif command -v aarch64-none-elf-pkg-config >/dev/null 2>&1; then
        PKGCFG=$(command -v aarch64-none-elf-pkg-config)
    fi
    if [ -n "$PKGCFG" ]; then
        read -ra CURL_LIBS <<< "$("$PKGCFG" --libs --static libcurl)"
    elif [ -f "$PL/lib/libmbedtls.a" ]; then
        CURL_LIBS=(-lcurl -lmbedtls -lmbedx509 -lmbedcrypto -lz)
    else
        CURL_LIBS=(-lcurl -lz)
    fi
    "$B/aarch64-none-elf-gcc" "${CFLAGS[@]}" -c "$ROOT/launcher/updater.c" -o "$O/updater.o"
    OBJS+=("$O/updater.o")
else
    echo "aviso: switch-curl não encontrado em $PL; compilando sem o atualizador"
fi

"$B/aarch64-none-elf-gcc" "${CFLAGS[@]}" -c "$ROOT/launcher/trophies.c" -o "$O/trophies.o"
"$B/aarch64-none-elf-gcc" "${CFLAGS[@]}" -c "$ROOT/launcher/main.c" -o "$O/main.o"
"$B/aarch64-none-elf-gcc" -specs="$DKP/libnx/switch.specs" "${ARCH[@]}" -o "$O/gow-nx.elf" "${OBJS[@]}" -L"$PL/lib" -L"$DKP/libnx/lib" \
    "${CURL_LIBS[@]}" -lfreetype -lharfbuzz -lfreetype -lbz2 -lpng16 -lz -lmpg123 -lnx -lm
"$DKP/tools/bin/nacptool" --create "God of War Collection" "Guto Recomp" "$VERSION" "$O/gow-nx.nacp"
ICON=""; [ -f "$ROOT/res/icon_launcher.jpg" ] && ICON="--icon=$ROOT/res/icon_launcher.jpg"
"$DKP/tools/bin/elf2nro" "$O/gow-nx.elf" "$ROOT/build/gow-nx.nro" --nacp="$O/gow-nx.nacp" $ICON --romfsdir="$ROOT/launcher/romfs"
ls -la "$ROOT/build/gow-nx.nro"
