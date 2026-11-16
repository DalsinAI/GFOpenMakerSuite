#!/bin/sh
# The OpenMake libraries for AmigaOS 3.x, each twice under one name:
# build-amiga/libs/fpu (68040/68060 or a 68881/68882; it will not open
# without an FPU) and build-amiga/libs/soft (no FPU instruction at all, so
# it opens anywhere). With each, its test linked through the library.
# A library may need nothing from outside it: the link fails if it does.
#
#   CC=.../m68k-amigaos-gcc SDK=.../gfopensdk ./build-libraries.sh
# MIT, Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
CC=${CC:-m68k-amigaos-gcc}
BIN=$(dirname "$CC")
OBJDUMP=${OBJDUMP:-$BIN/m68k-amigaos-objdump}
NM=${NM:-$BIN/m68k-amigaos-nm}
SDK=${SDK:?"SDK: the GFOpen SDK's root"}
OUT=${OUT:-"$HERE/build-amiga/libs"}
cd "$HERE"
python3 core/library/gen-library.py machines/library/openmakemachine_lib.sfd --check

RT="core/library/runtime.c core/library/runtime_dos.c"
WARN="-Wall -Wextra -Werror"
LIBF="-O2 -std=gnu99 -fno-builtin -fno-toplevel-reorder -fomit-frame-pointer -nostartfiles $WARN -Icore/library -I$SDK/Include"

fpu_count() {
    "$OBJDUMP" -m m68k:68040 -d "$1" | awk -F'\t' 'NF >= 3 && $3 ~ /^f/ && $3 !~ /d0,%?fpcr$/' | wc -l | tr -d ' '
}

# name: the library; its include and sources
lib() {
    v=$1 flags=$2 name=$3 inc=$4
    shift 4
    dir="$OUT/$v"
    mkdir -p "$dir"
    # Without the FPU: libnix's soft float (libm) over the mathieee libraries,
    # whose bases libshell.c opens.
    m=""
    [ "$v" = soft ] && m="-lm"
    "$CC" $flags $LIBF -I"$inc" -o "$dir/$name" core/library/libstart.c "$@" $RT $m -lgcc
    left=$("$NM" -u "$dir/$name" | wc -l | tr -d ' ')
    if [ "$left" != 0 ]; then
        "$NM" -u "$dir/$name" >&2
        echo "build-libraries: $v/$name needs $left symbols from outside it" >&2
        exit 1
    fi
    n=$(fpu_count "$dir/$name")
    if [ "$v" = soft ] && [ "$n" != 0 ]; then
        echo "build-libraries: soft/$name has $n FPU instructions" >&2
        exit 1
    fi
    echo "$dir/$name: $(wc -c < "$dir/$name") bytes, $n FPU instructions"
}

for v in fpu soft; do
    if [ "$v" = fpu ]; then flags="-m68040 -mhard-float"; else flags="-m68040 -msoft-float"; fi
    prog="$flags -noixemul -O2 -std=gnu99 $WARN -Icore/library"
    [ "$v" = soft ] && prog="$prog -specs=$SDK/Lib/m68k-soft-m68040.specs"
    lib "$v" "$flags" openmakemachine.library machines/include \
        machines/library/openmakemachine_lib.c machines/src/machine.c
    # machines/tests/test_machine.c, its calls through the library
    "$CC" $prog -Imachines/include -I"$SDK/Include" -s -o "$OUT/$v/TestMachineLib" machines/tests/test_machine.c \
        machines/library/openmakemachine_link.c core/library/link_base.c \
        -DLINK_BASE=OpenMakeMachineBase '-DLINK_NAME="openmakemachine.library"' -lm
    echo "$OUT/$v/TestMachineLib: $(wc -c < "$OUT/$v/TestMachineLib") bytes"
done
echo "build-libraries: $OUT/fpu and $OUT/soft"
