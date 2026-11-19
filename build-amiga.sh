#!/bin/sh
# GFOpenMake and the core tests for AmigaOS 3.x, twice: -m68040
# -mhard-float, and -m68040 -msoft-float for a 68LC040 or 68EC040.
# The hard-float build must contain no FPU instruction a 68040 traps.
# The soft-float build must contain no FPU instruction at all.
#
#   CC=.../m68k-amigaos-gcc SDK=.../gfopensdk LAYOUT=.../gfopenlayout \
#   OGT_ROOT=.../amicradle-guest/libraries/opengadtools ./build-amiga.sh
#
# A stove build sets STOVE, OUT and GUEST and does not set SDK or LAYOUT.
# This script then takes gfopensdk at the commit CI uses, GFOpenLayout at
# gfopenlayout.ref, and opengadtools from GUEST. Those checkouts sit beside
# this tree, not in OUT.
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
# The commit .github/workflows/ci.yml checks out for Include/gfopengpu.
SDK_PIN=79e8d48151082ef2ee3cfe97998ebb80eec2aa86

take() {
    dest=$1
    url=$2
    sha=$3
    got=$(git -C "$dest" rev-parse HEAD 2>/dev/null || true)
    if [ "$got" = "$sha" ]; then
        return 0
    fi
    mkdir -p "$(dirname "$dest")"
    rm -rf "$dest"
    GIT_TERMINAL_PROMPT=0 git clone --quiet "$url" "$dest"
    GIT_TERMINAL_PROMPT=0 git -C "$dest" checkout --quiet "$sha"
}

if [ -n "${STOVE:-}" ] && [ -z "${CC:-}" ]; then
    CC="$STOVE/prefix/bin/m68k-amigaos-gcc"
fi
CC=${CC:-m68k-amigaos-gcc}
OBJDUMP=${OBJDUMP:-$(dirname "$CC")/m68k-amigaos-objdump}
if [ -n "${STOVE:-}" ]; then
    DEPS=$(CDPATH= cd -- "$HERE/.." && pwd)/.openmake-deps
    if [ -z "${SDK:-}" ]; then
        take "$DEPS/gfopensdk" https://github.com/DalsinAI/gfopensdk.git "$SDK_PIN"
        SDK="$DEPS/gfopensdk"
    fi
    if [ -z "${LAYOUT:-}" ]; then
        take "$DEPS/gfopenlayout" https://github.com/DalsinAI/gfopenlayout.git "$(cat "$HERE/gfopenlayout.ref")"
        LAYOUT="$DEPS/gfopenlayout"
    fi
    if [ -z "${OGT_ROOT:-}" ]; then
        if [ -z "${GUEST:-}" ] || [ ! -d "$GUEST/libraries/opengadtools" ]; then
            echo "build-amiga: OGT_ROOT is not set, and GUEST has no libraries/opengadtools" >&2
            exit 1
        fi
        OGT_ROOT="$GUEST/libraries/opengadtools"
    fi
fi
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
for f in $(grep -ho 'olui_[a-z_0-9]*(' cad/src/*.c slicer/src/*.c burn/src/*.c workshop/src/*.c core/ui/*.c | tr -d '(' | sort -u); do
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
    "$CC" $cf -s -o "$dir/GFOpenMake" cad/src/make.c cad/src/make_rexx.c cad/src/edit.c core/src/rexx.c core/src/rexx_amiga.c core/src/parts.c core/src/gpu.c core/src/layer.c core/src/layer_paint.c core/src/loops.c \
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
    # The editor's shelf, as PROGDIR:examples/cad (ge_shelf_load).
    for f in examples/cad/*/*; do
        [ -f "$f" ] || continue
        mkdir -p "$dir/$(dirname "$f")"
        cp "$f" "$dir/$f"
    done
    # GFOpenSlicer's window (slicer/src/amiga.c): the toolpath engine on
    # exec tasks (sl_job.c), on core/ui's helpers, picker and OpenGPU view
    "$CC" $cf -Islicer/include -Islicer/src -s -o "$dir/GFOpenSlicer" slicer/src/amiga.c slicer/src/slicer_rexx.c slicer/src/sl_job.c core/src/rexx.c core/src/rexx_amiga.c \
        slicer/src/slicer_ui.c core/ui/settings_ui.c core/ui/view_ui.c core/ui/window_amiga.c core/ui/machines_ui.c \
        core/ui/view_gpu.c core/ui/view_gpu_amiga.c core/src/gpu.c core/src/plan_par.c \
        core/src/par_exec.c core/src/gcode.c core/src/settings.c machines/src/machine.c $PLAN core/src/layer.c \
        core/src/loops.c $CORE "$OLUI" -lm
    # OpenMakeBurn's window (burn/src/amiga.c): SVG or DXF to Grbl laser G-code,
    # core/ui's helpers, the laser picker and the OpenGPU view
    "$CC" $cf -Iburn/include -Iburn/src -s -o "$dir/GFOpenBurn" burn/src/amiga.c burn/src/burn_rexx.c burn/src/burn_job.c burn/src/burn_ui.c burn/src/material.c burn/src/image_load.c burn/src/image.c core/src/rexx.c core/src/rexx_amiga.c \
        core/src/vec.c core/src/dxf.c core/src/gcode.c core/src/toolpath.c core/src/fill.c core/src/loops.c core/src/inset.c core/src/union.c core/src/toolpath_paint.c core/src/gpu.c \
        core/ui/view_ui.c core/ui/window_amiga.c core/ui/machines_ui.c core/ui/view_gpu.c core/ui/view_gpu_amiga.c \
        machines/src/machine.c $CORE "$OLUI" -lm
    # OpenMakeCNC's window (cnc/src/amiga.c): SVG or DXF to Grbl mill G-code,
    # core/ui's helpers, the mill picker and the pen-drawn toolpath
    "$CC" $cf -Icnc/include -Icnc/src -s -o "$dir/GFOpenCNC" cnc/src/amiga.c cnc/src/cnc_job.c cnc/src/cnc_ui.c cnc/src/cnc_rexx.c \
        core/src/rexx.c core/src/rexx_amiga.c \
        core/src/vec.c core/src/dxf.c core/src/toolpath.c core/src/loops.c core/src/inset.c core/src/union.c \
        core/src/toolpath_paint.c core/ui/view_ui.c core/ui/window_amiga.c core/ui/machines_ui.c \
        machines/src/machine.c $CORE "$OLUI" -lm
    # GFOpenWorkshop's window (workshop/src/amiga.c): one product's drawer,
    # core/ui's helpers and the pen-drawn preview of the picked item
    "$CC" $cf -Iworkshop/src -Iburn/include -s -o "$dir/GFOpenWorkshop" workshop/src/amiga.c workshop/src/workshop_ui.c \
        workshop/src/workshop_rexx.c core/src/rexx.c core/src/rexx_amiga.c burn/src/image_load.c \
        core/src/project.c core/src/vec.c core/src/dxf.c \
        core/ui/view_ui.c core/ui/window_amiga.c \
        $CORE "$OLUI" -lm
    "$CC" $cf -s -o "$dir/TestModel" core/tests/test_model.c $CORE -lm
    "$CC" $cf -s -o "$dir/TestLayer" core/tests/test_layer.c core/src/layer.c core/src/layer_paint.c core/src/loops.c cad/src/edit.c core/src/parts.c $CORE -lm
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
# Soft-float as well, including on a stove, unless SOFT=0.
SOFT=${SOFT:-1}
if [ "$SOFT" != 0 ]; then
    build soft "-m68040 -msoft-float -specs=$SDK/Lib/m68k-soft-m68040.specs"
    if awk '$2 != 0 { bad = 1 } END { exit !bad }' "$OUT/soft.fpu"; then
        echo "build-amiga: the soft build has FPU instructions" >&2
        exit 1
    fi
fi

# The FPU build should use the FPU.
if awk '$1 == "TestLayer" && $2 == 0 { bad = 1 } END { exit !bad }' "$OUT/fpu.fpu"; then
    echo "build-amiga: the FPU build has no FPU instructions" >&2
    exit 1
fi
rm -f "$OUT/soft.fpu" "$OUT/fpu.fpu"
if [ "$SOFT" != 0 ]; then
    echo "build-amiga: $OUT/fpu and $OUT/soft"
else
    echo "build-amiga: $OUT/fpu"
fi
