#!/bin/sh
# GFOpenMake and the core tests for AmigaOS 3.x, twice: with the FPU
# (68040/68060, or a 68881/68882) and without it (soft float, for the
# 68LC040, 68EC040 and the rest). The soft build must hold no FPU
# instruction at all: that is checked here, and the build fails if it does.
#
#   CC=.../m68k-amigaos-gcc SDK=.../gfopensdk LAYOUT=.../gfopenlayout \
#   OLUI=.../libolui.a ./build-amiga.sh
#
# SDK is the GFOpen SDK's root (Include, and Lib/m68k-soft-m68040.specs).
# OLUI is GFOpenLayout's UI link library for the 68040, from its
# build-ui-amiga.sh; it holds no floating point, so one serves both builds.
# MIT, Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
CC=${CC:-m68k-amigaos-gcc}
OBJDUMP=${OBJDUMP:-$(dirname "$CC")/m68k-amigaos-objdump}
SDK=${SDK:?"SDK: the GFOpen SDK's root"}
LAYOUT=${LAYOUT:?"LAYOUT: a GFOpenLayout checkout, for its headers"}
OLUI=${OLUI:?"OLUI: GFOpenLayout's 68040 libolui.a (its build-ui-amiga.sh)"}
OUT=${OUT:-"$HERE/build-amiga"}
cd "$HERE"

CORE="core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c"
WARN="-Wall -Wextra -Werror"
INC="-Icore/include -Icad/include -Icad/src -Imachines/include -I$LAYOUT/include -I$SDK/Include"

build() {
    name=$1
    flags=$2
    dir="$OUT/$name"
    rm -rf "$dir"
    mkdir -p "$dir"
    : > "$OUT/$name.fpu"
    cf="$flags -noixemul -O2 -std=gnu99 $WARN $INC"
    "$CC" $cf -s -o "$dir/GFOpenMake" cad/src/make.c cad/src/edit.c core/src/gpu.c core/src/layer.c core/src/layer_paint.c core/src/loops.c \
        $CORE "$OLUI" -lm
    "$CC" $cf -s -o "$dir/TestModel" core/tests/test_model.c $CORE -lm
    "$CC" $cf -s -o "$dir/TestLayer" core/tests/test_layer.c core/src/layer.c core/src/layer_paint.c core/src/loops.c $CORE -lm
    "$CC" $cf -s -o "$dir/TestGpu" core/tests/test_gpu.c core/src/gpu.c core/src/layer.c $CORE -lm
    "$CC" $cf -s -o "$dir/TestEdit" cad/tests/test_edit.c cad/src/edit.c $CORE -lm
    "$CC" $cf -s -o "$dir/TestMachine" machines/tests/test_machine.c machines/src/machine.c -lm
    for f in "$dir"/*; do
        # Decoded as a 68040's, or objdump shows FPU words as .short and
        # misses them. Every FPU mnemonic starts with f; no other does.
        # libnix's start-up clears the FPCR only when AttnFlags says there
        # is an FPU, so that one is not counted.
        n=$("$OBJDUMP" -m m68k:68040 -d "$f" | awk -F'\t' 'NF >= 3 && $3 ~ /^f/ && $3 !~ /d0,%?fpcr$/' | wc -l | tr -d ' ')
        echo "$f: $(wc -c < "$f") bytes, $n FPU instructions"
        echo "$(basename "$f") $n" >> "$OUT/$name.fpu"
        # The 68040 and 68060 trap FSIN, FCOS, FETOX and the rest to
        # software (68040.library); none may be in either build.
        t=$("$OBJDUMP" -m m68k:68040 -d "$f" | awk -F'\t' 'NF >= 3 && $3 ~ /^f(sin|cos|tan|atan|asin|acos|etox|twotox|tentox|log|sinh|cosh|tanh|getexp|getman|rem|mod|scale)/' | wc -l | tr -d ' ')
        if [ "$t" != 0 ]; then
            echo "build-amiga: $f has $t FPU instructions a 68040 traps" >&2
            exit 1
        fi
    done
}

build fpu "-m68040 -mhard-float"
build soft "-m68040 -msoft-float -specs=$SDK/Lib/m68k-soft-m68040.specs"

# The soft build may not touch the FPU; the FPU build should use it.
if awk '$2 != 0 { bad = 1 } END { exit !bad }' "$OUT/soft.fpu"; then
    echo "build-amiga: the soft build has FPU instructions" >&2
    exit 1
fi
if awk '$1 == "TestLayer" && $2 == 0 { bad = 1 } END { exit !bad }' "$OUT/fpu.fpu"; then
    echo "build-amiga: the FPU build has no FPU instructions" >&2
    exit 1
fi
rm -f "$OUT/soft.fpu" "$OUT/fpu.fpu"
echo "build-amiga: $OUT/fpu and $OUT/soft"
