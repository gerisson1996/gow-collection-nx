#!/bin/bash
# build/gow-nx.nro: launcher com arte de fundo e música extraídas de game/PCSA00126/ (ou personalizadas no SD),
# menu de opções/idiomas com detecção automática do Switch e de data/traducoes/*/, troféus, atualizador e encadeamento via envSetNextLoad.
# Opcionalmente, defina LIBATRAC9=/caminho/do/LibAtrac9 (ou VITART=/caminho/do/vitaRT) para habilitar
# a música oficial ATRAC9 direto de game/PCSA00126/exec/gow1_psp2_SCEA.psarc.
# Precisa dos portlibs switch-freetype, switch-libpng, switch-zlib, switch-libwebp e switch-mpg123 (e switch-curl para o atualizador).
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
DKP=${DEVKITPRO:-/opt/devkitpro}; B=$DKP/devkitA64/bin; PL=$DKP/portlibs/switch; O=$ROOT/build/launcher
if [ -z "${VERSION:-}" ]; then
    VERSION=$(git -C "$ROOT" tag --list | grep -E '^v?[0-9]+\.[0-9]+' | sed 's/^v//' | sort -t. -k1,1nr -k2,2nr -k3,3nr | head -n1 || true)
    VERSION=${VERSION:-1.4}
fi
UPDATE_REPO=${UPDATE_REPO:-gerisson1996/gow-collection-nx}
mkdir -p "$O" "$ROOT/launcher/romfs"
ARCH=(-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE)
CFLAGS=("${ARCH[@]}" -O2 -Wall -Wno-format-truncation -D__SWITCH__ "-DVERSAO=\"$VERSION\"" "-DUPDATE_REPO=\"$UPDATE_REPO\"" "-I$DKP/libnx/include" "-I$PL/include" "-I$PL/include/freetype2" "-I$ROOT/launcher" "-I$ROOT/platform/nx")

OBJS=("$O/main.o" "$O/game_assets.o" "$O/trophies.o" "$O/i18n.o" "$O/i18n_data.o")

# LibAtrac9 opcional (MIT): decodifica tit1e1_2.at9 direto de game/PCSA00126/exec/gow1_psp2_SCEA.psarc
AT9_SRC=""
for cand in "${LIBATRAC9:-}/C/src" "${LIBATRAC9:-}/src" "${VITART:-}/third_party/libatrac9/src" "$ROOT/build/libatrac9/C/src"; do
    if [ -n "$cand" ] && [ -f "$cand/libatrac9.h" ]; then
        AT9_SRC="$cand"
        break
    fi
done
if [ -n "$AT9_SRC" ]; then
    CFLAGS+=(-DENABLE_ATRAC9 "-I$AT9_SRC")
    for f in "$AT9_SRC"/*.c; do
        obj="$O/at9_$(basename "${f%.c}").o"
        "$B/aarch64-none-elf-gcc" "${ARCH[@]}" -O2 -w -I"$AT9_SRC" -c "$f" -o "$obj"
        OBJS+=("$obj")
    done
else
    echo "aviso: LibAtrac9 não encontrada (defina LIBATRAC9); compilando sem áudio ATRAC9 de PCSA00126"
fi

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

"$B/aarch64-none-elf-gcc" "${CFLAGS[@]}" -c "$ROOT/launcher/game_assets.c" -o "$O/game_assets.o"
"$B/aarch64-none-elf-gcc" "${CFLAGS[@]}" -c "$ROOT/launcher/trophies.c" -o "$O/trophies.o"
"$B/aarch64-none-elf-gcc" "${CFLAGS[@]}" -c "$ROOT/platform/nx/i18n.c" -o "$O/i18n.o"
"$B/aarch64-none-elf-gcc" "${CFLAGS[@]}" -c "$ROOT/platform/nx/i18n_data.c" -o "$O/i18n_data.o"
"$B/aarch64-none-elf-gcc" "${CFLAGS[@]}" -c "$ROOT/launcher/main.c" -o "$O/main.o"
"$B/aarch64-none-elf-gcc" -specs="$DKP/libnx/switch.specs" "${ARCH[@]}" -o "$O/gow-nx.elf" "${OBJS[@]}" -L"$PL/lib" -L"$DKP/libnx/lib" \
    "${CURL_LIBS[@]}" -lwebp -lfreetype -lharfbuzz -lfreetype -lbz2 -lpng16 -lz -lmpg123 -lnx -lm
"$DKP/tools/bin/nacptool" --create "God of War Collection" "Guto Recomp" "$VERSION" "$O/gow-nx.nacp"
ICON=""; [ -f "$ROOT/res/icon_launcher.jpg" ] && ICON="--icon=$ROOT/res/icon_launcher.jpg"
"$DKP/tools/bin/elf2nro" "$O/gow-nx.elf" "$ROOT/build/gow-nx.nro" --nacp="$O/gow-nx.nacp" $ICON --romfsdir="$ROOT/launcher/romfs"
ls -la "$ROOT/build/gow-nx.nro"
