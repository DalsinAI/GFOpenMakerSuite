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
python3 core/library/gen-library.py machines/library/openmakemachine_lib.sfd core/library/openmakepath_lib.sfd \
    core/library/openmakemesh_lib.sfd core/library/openmakegcode_lib.sfd \
    core/library/openmakeview_lib.sfd core/library/openmaketask_lib.sfd --check

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
    t=$("$OBJDUMP" -m m68k:68040 -d "$dir/$name" | awk -F'\t' 'NF >= 3 && $3 ~ /^f(sin|cos|tan|atan|asin|acos|etox|twotox|tentox|log|sinh|cosh|tanh|getexp|getman|rem|mod|scale)/' | wc -l | tr -d ' ')
    if [ "$t" != 0 ]; then
        echo "build-libraries: $v/$name has $t FPU instructions a 68040 traps" >&2
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
    lib "$v" "$flags" openmakepath.library core/include \
        core/library/openmakepath_lib.c core/src/layer.c core/src/loops.c core/src/inset.c \
        core/src/toolpath.c core/src/fill.c core/src/plan.c
    # core/tests/test_layer.c, the cut and the loops through the library
    "$CC" $prog -Icore/include -I"$SDK/Include" -s -o "$OUT/$v/TestLayerLib" core/tests/test_layer.c \
        core/src/layer_paint.c core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c \
        core/library/openmakepath_link.c core/library/link_base.c \
        -DLINK_BASE=OpenMakePathBase '-DLINK_NAME="openmakepath.library"' -lm
    echo "$OUT/$v/TestLayerLib: $(wc -c < "$OUT/$v/TestLayerLib") bytes"
    # core/tests/test_fill.c, the toolpath and its fill through the library
    "$CC" $prog -Icore/include -I"$SDK/Include" -s -o "$OUT/$v/TestFillLib" core/tests/test_fill.c \
        core/library/openmakepath_link.c core/library/link_base.c \
        -DLINK_BASE=OpenMakePathBase '-DLINK_NAME="openmakepath.library"' -lm
    echo "$OUT/$v/TestFillLib: $(wc -c < "$OUT/$v/TestFillLib") bytes"
    # core/tests/test_plan.c, the wrench's layers planned through the library
    "$CC" $prog -Icore/include -I"$SDK/Include" -s -o "$OUT/$v/TestPlanLib" core/tests/test_plan.c \
        core/src/model.c core/src/stl.c \
        core/library/openmakepath_link.c core/library/link_base.c \
        -DLINK_BASE=OpenMakePathBase '-DLINK_NAME="openmakepath.library"' -lm
    echo "$OUT/$v/TestPlanLib: $(wc -c < "$OUT/$v/TestPlanLib") bytes"
    lib "$v" "$flags" openmakemesh.library core/include \
        core/library/openmakemesh_lib.c core/src/model.c core/src/stl.c
    # core/tests/test_model.c, the solids, booleans and STL through the library
    "$CC" $prog -Icore/include -I"$SDK/Include" -s -o "$OUT/$v/TestModelLib" core/tests/test_model.c \
        core/src/view.c core/src/solid.c \
        core/library/openmakemesh_link.c core/library/link_base.c \
        -DLINK_BASE=OpenMakeMeshBase '-DLINK_NAME="openmakemesh.library"' -lm
    echo "$OUT/$v/TestModelLib: $(wc -c < "$OUT/$v/TestModelLib") bytes"
    # The G-code, its start and end code through openmakemachine.library
    lib "$v" "$flags" openmakegcode.library core/include -Imachines/include \
        core/library/openmakegcode_lib.c core/src/gcode.c machines/library/openmakemachine_link.c
    lib "$v" "$flags" openmakeview.library core/include \
        core/library/openmakeview_lib.c core/src/solid.c core/src/view.c core/src/layer_paint.c \
        core/src/toolpath_paint.c core/src/toolpath.c core/src/gpu.c
    # core/tests/test_toolpath_paint.c: the toolpath planned through
    # openmakepath.library and drawn through openmakeview.library
    "$CC" $prog -Icore/include -I"$SDK/Include" -s -o "$OUT/$v/TestToolpathPaintLib" core/tests/test_toolpath_paint.c \
        core/src/model.c core/src/stl.c core/library/openmakeview_link.c core/library/openmakepath_link.c \
        core/library/link_base.c -DLINK_BASE=OpenMakeViewBase '-DLINK_NAME="openmakeview.library"' \
        -DLINK_BASE2=OpenMakePathBase '-DLINK_NAME2="openmakepath.library"' -lm
    echo "$OUT/$v/TestToolpathPaintLib: $(wc -c < "$OUT/$v/TestToolpathPaintLib") bytes"
    # Work on several exec tasks; the pieces are the program's own code
    lib "$v" "$flags" openmaketask.library core/include \
        core/library/openmaketask_lib.c core/src/par_exec.c
    # core/tests/test_plan_par.c, the wrench's layers on tasks the library makes
    "$CC" $prog -Icore/include -I"$SDK/Include" -s -o "$OUT/$v/TestPlanParLib" core/tests/test_plan_par.c \
        core/src/plan_par.c core/src/plan.c core/src/fill.c core/src/inset.c core/src/toolpath.c \
        core/src/layer.c core/src/loops.c core/src/model.c core/src/stl.c \
        core/library/openmaketask_link.c core/library/link_base.c \
        -DLINK_BASE=OpenMakeTaskBase '-DLINK_NAME="openmaketask.library"' -lm
    echo "$OUT/$v/TestPlanParLib: $(wc -c < "$OUT/$v/TestPlanParLib") bytes"
done
echo "build-libraries: $OUT/fpu and $OUT/soft"
