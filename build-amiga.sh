#!/bin/sh
# GFOpenMake and the core tests for AmigaOS 3.x, twice: with the FPU
# (68040/68060, or a 68881/68882) and without it (soft float, for the
# 68LC040, 68EC040 and the rest). The soft build must hold no FPU
# instruction at all: that is checked here, and the build fails if it does.
#
#   CC=.../m68k-amigaos-gcc SDK=.../gfopensdk LAYOUT=.../gfopenlayout \
#   OGT_ROOT=.../amicradle-guest/libraries/opengadtools ./build-amiga.sh
#
# SDK is the GFOpen SDK's root (Include, and Lib/m68k-soft-m68040.specs).
# LAYOUT is a GFOpenLayout checkout at the commit in gfopenlayout.ref, the
# one CI tests with (LAYOUT_ANY=1 takes any). OLUI is its UI link library
# for the 68040: built here from LAYOUT with its build-ui-amiga.sh (which
# needs OGT_ROOT), or given ready made; it is checked for every olui_ call
# the apps make. It holds no floating point, so one serves both builds.
# MIT, Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
CC=${CC:-m68k-amigaos-gcc}
OBJDUMP=${OBJDUMP:-$(dirname "$CC")/m68k-amigaos-objdump}
SDK=${SDK:?"SDK: the GFOpen SDK's root"}
LAYOUT=${LAYOUT:?"LAYOUT: a GFOpenLayout checkout, for its headers"}
OUT=${OUT:-"$HERE/build-amiga"}
NM=${NM:-$(dirname "$CC")/m68k-amigaos-nm}
cd "$HERE"

# GFOpenLayout as pinned, so a clean clone builds what CI tested.
PIN=$(cat gfopenlayout.ref)
GOT=$(git -C "$LAYOUT" rev-parse HEAD 2>/dev/null || echo "not a git checkout")
if [ "$GOT" != "$PIN" ] && [ -z "${LAYOUT_ANY:-}" ]; then
    echo "build-amiga: LAYOUT is at $GOT, not the pinned $PIN (gfopenlayout.ref);" >&2
    echo "  git -C \"$LAYOUT\" checkout $PIN, or LAYOUT_ANY=1" >&2
    exit 1
fi
if [ -z "${OLUI:-}" ]; then
    OGT_ROOT=${OGT_ROOT:?"OGT_ROOT: GFOpenGadTools (amicradle-guest libraries/opengadtools), to build libolui.a; or OLUI=.../libolui.a"}
    CC="$CC" AR="$(dirname "$CC")/m68k-amigaos-ar" SPECS="$SDK/Lib/m68k-soft-m68040.specs" OGT_ROOT="$OGT_ROOT" \
        OUT="$OUT/olui" sh "$LAYOUT/build-ui-amiga.sh" > /dev/null
    OLUI="$OUT/olui/libolui.a"
fi
# Every olui_ function the apps call is in OLUI: an older one fails here,
# by name, not at the link.
MISSING=""
DEFINED=$("$NM" "$OLUI" 2>/dev/null | awk '$2 == "T" { print $3 }')
for f in $(grep -ho 'olui_[a-z_0-9]*(' cad/src/*.c slicer/src/*.c burn/src/*.c core/ui/*.c | tr -d '(' | sort -u); do
    echo "$DEFINED" | grep -qx "_$f" || MISSING="$MISSING $f"
done
if [ -n "$MISSING" ]; then
    echo "build-amiga: $OLUI lacks$MISSING: build it from GFOpenLayout $PIN (leave OLUI unset)" >&2
    exit 1
fi

CORE="core/src/model.c core/src/stl.c core/src/view.c core/src/solid.c"
# The slice as a toolpath, and its picture (gpu.c draws it for a chip).
PLAN="core/src/plan.c core/src/fill.c core/src/inset.c core/src/union.c core/src/toolpath.c core/src/toolpath_paint.c"
WARN="-Wall -Wextra -Werror"
INC="-Icore/include -Icore/ui -Icad/include -Icad/src -Imachines/include -I$LAYOUT/include -I$SDK/Include"

build() {
    name=$1
    flags=$2
    dir="$OUT/$name"
    rm -rf "$dir"
    mkdir -p "$dir"
    : > "$OUT/$name.fpu"
    cf="$flags -noixemul -O2 -std=gnu99 $WARN $INC"
    "$CC" $cf -s -o "$dir/GFOpenMake" cad/src/make.c cad/src/edit.c core/src/parts.c core/src/gpu.c core/src/layer.c core/src/layer_paint.c core/src/loops.c \
        core/ui/view_ui.c core/ui/window_amiga.c core/ui/view_gpu_amiga.c core/ui/machines_ui.c core/ui/settings_ui.c core/src/settings.c core/src/plan_par.c core/src/par_exec.c core/src/gcode.c \
        machines/src/machine.c $PLAN $CORE "$OLUI" -lm
    # The machines, as PROGDIR:machines, for the printer picker: every
    # driver.om and profile, nothing else
    for f in machines/*/*/driver.om machines/*/*/profiles/*.om; do
        [ -f "$f" ] || continue
        mkdir -p "$dir/$(dirname "$f")"
        cp "$f" "$dir/$f"
    done
    # Materials-library starting points, as PROGDIR:materials/laser
    mkdir -p "$dir/materials/laser"
    cp materials/laser/*.om "$dir/materials/laser/"
    # The print settings profiles, as PROGDIR:settings
    mkdir -p "$dir/settings"
    cp settings/* "$dir/settings/"
    # GFOpenSlicer's window (slicer/src/amiga.c): the toolpath engine on
    # exec tasks (sl_job.c), on core/ui's helpers, picker and OpenGPU view
    "$CC" $cf -Islicer/include -Islicer/src -s -o "$dir/GFOpenSlicer" slicer/src/amiga.c slicer/src/sl_job.c \
        slicer/src/slicer_ui.c core/ui/settings_ui.c core/ui/view_ui.c core/ui/window_amiga.c core/ui/machines_ui.c \
        core/ui/view_gpu.c core/ui/view_gpu_amiga.c core/src/gpu.c core/src/plan_par.c \
        core/src/par_exec.c core/src/gcode.c core/src/settings.c machines/src/machine.c $PLAN core/src/layer.c \
        core/src/loops.c $CORE "$OLUI" -lm
    # OpenMakeBurn's window (burn/src/amiga.c): SVG or DXF to Grbl laser G-code,
    # core/ui's helpers, the laser picker and the OpenGPU view
    "$CC" $cf -Iburn/include -Iburn/src -s -o "$dir/GFOpenBurn" burn/src/amiga.c burn/src/burn_job.c burn/src/burn_ui.c burn/src/material.c \
        core/src/vec.c core/src/dxf.c core/src/gcode.c core/src/toolpath.c core/src/fill.c core/src/loops.c core/src/inset.c core/src/union.c core/src/toolpath_paint.c core/src/gpu.c \
        core/ui/view_ui.c core/ui/window_amiga.c core/ui/machines_ui.c core/ui/view_gpu.c core/ui/view_gpu_amiga.c \
        machines/src/machine.c $CORE "$OLUI" -lm
    "$CC" $cf -s -o "$dir/TestModel" core/tests/test_model.c $CORE -lm
    "$CC" $cf -s -o "$dir/TestLayer" core/tests/test_layer.c core/src/layer.c core/src/layer_paint.c core/src/loops.c $CORE -lm
    "$CC" $cf -s -o "$dir/TestGpu" core/tests/test_gpu.c core/src/gpu.c core/src/layer.c core/src/loops.c $PLAN $CORE -lm
    # The layers on exec tasks (par_exec.c): the same toolpath as one task's
    "$CC" $cf -s -o "$dir/TestPlanPar" core/tests/test_plan_par.c core/src/plan_par.c core/src/par_exec.c \
        core/src/layer.c core/src/loops.c $PLAN $CORE -lm
    "$CC" $cf -s -o "$dir/TestEdit" cad/tests/test_edit.c cad/src/edit.c core/src/parts.c $CORE -lm
    "$CC" $cf -s -o "$dir/TestMachine" machines/tests/test_machine.c machines/src/machine.c -lm
    "$CC" $cf -s -o "$dir/TestVec" core/tests/test_vec.c core/src/vec.c core/src/dxf.c -lm
    "$CC" $cf -s -o "$dir/TestDxf" core/tests/test_dxf.c core/src/vec.c core/src/dxf.c -lm
    for f in "$dir"/[A-Z]*; do
        # fpu-count.sh: the FPU instructions, not counting addresses.
        n=$(OBJDUMP=$OBJDUMP sh "$HERE/fpu-count.sh" "$f")
        echo "$f: $(wc -c < "$f") bytes, $n FPU instructions"
        echo "$(basename "$f") $n" >> "$OUT/$name.fpu"
        # The 68040 and 68060 trap FSIN, FCOS, FETOX and the rest to
        # software (68040.library); none may be in either build.
        t=$(OBJDUMP=$OBJDUMP sh "$HERE/fpu-count.sh" "$f" traps)
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
