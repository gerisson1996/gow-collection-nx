#!/bin/bash
# Compila e roda os testes do leitor de idiomas no PC, com e sem sanitizers.
#   bash tests/run_i18n_tests.sh            # usa $CC ou cc
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
CC=${CC:-cc}
OUT=$(mktemp -d)
cd "$OUT"
FLAGS=(-std=c99 -Wall -Wextra -Werror -I"$ROOT/platform/nx")

"$CC" "${FLAGS[@]}" -O2 "$ROOT/platform/nx/i18n.c" "$ROOT/tests/i18n_test.c" -o i18n_test
./i18n_test

if "$CC" -fsanitize=address,undefined -x c /dev/null -c -o /dev/null 2>/dev/null; then
    "$CC" "${FLAGS[@]}" -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer \
        "$ROOT/platform/nx/i18n.c" "$ROOT/tests/i18n_test.c" -o i18n_test_san
    ./i18n_test_san
fi

# O i18n_data.c gerado tem de compilar junto com o leitor.
"$CC" "${FLAGS[@]}" -c "$ROOT/platform/nx/i18n_data.c" -o i18n_data.o
echo "testes do i18n ok"
