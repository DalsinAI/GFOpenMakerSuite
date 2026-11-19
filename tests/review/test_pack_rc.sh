#!/bin/sh
# Pack the FPU review drawer from the newest finished stove build of
# this commit and attach it to the v0.3.0-rc3 prereleases. Runs on
# Daletop. Does not commit the binaries. Soft-float is not packed:
# the stove's -msoft-float output still has FPU instructions.
set -eu

REPO=DalsinAI/GFOpenMakerSuite
SUITE_TAG=v0.3.0-rc3
FPU_NAME=OpenMake-0.3.0-rc3-fpu
STACK='$STACK:65536'
JOBS_A=/run/media/da1ek/WorkingSpace/AmigaChrome/archive/daletop-offload-20261023/AmigaChrome-dev/cradle-builds/jobs
JOBS_B=${HOME:-/home/da1ek}/AmiCradle-dev/cradle-builds/jobs

if [ -z "${OUT:-}" ]; then
    echo "test_pack_rc: OUT is not set" >&2
    exit 1
fi

root=$(git rev-parse --show-toplevel)
sha=$(git -C "$root" rev-parse HEAD)
case "$sha" in
    ????????????????????????????????????????) ;;
    *) echo "test_pack_rc: HEAD is not a full SHA ($sha)" >&2; exit 1 ;;
esac

ver_src() {
    grep -o '\$VER:[^"]*' "$1" | head -1
}

VER_MAKE=$(ver_src "$root/cad/src/make.c")
VER_SLICER=$(ver_src "$root/core/include/gs.h")
VER_BURN=$(ver_src "$root/burn/src/amiga.c")
VER_CNC=$(ver_src "$root/cnc/src/amiga.c")
VER_WORKSHOP=$(ver_src "$root/workshop/src/amiga.c")
case "$VER_SLICER" in
    *0.3.0-rc3*) ;;
    *) echo "test_pack_rc: slicer version is $VER_SLICER, not 0.3.0-rc3" >&2; exit 1 ;;
esac

TREE=$(SHA="$sha" python3 - "$JOBS_A" "$JOBS_B" << 'PY'
import json, os, sys
sha = os.environ["SHA"]
repo = "DalsinAI/GFOpenMakerSuite"
best = None
best_key = ""
for root in sys.argv[1:]:
    if not os.path.isdir(root):
        continue
    for name in os.listdir(root):
        p = os.path.join(root, name, "job.json")
        if not os.path.isfile(p):
            continue
        try:
            rec = json.load(open(p))
        except Exception:
            continue
        if rec.get("sha") != sha or rec.get("repo") != repo:
            continue
        if rec.get("kind") != "build" or rec.get("state") != "done" or rec.get("exitCode") != 0:
            continue
        out = os.path.join(root, name, "out")
        if not os.path.isfile(os.path.join(out, "fpu", "GFOpenMake")):
            continue
        key = rec.get("endedAt") or ""
        if best is None or key > best_key:
            best = out
            best_key = key
if not best:
    sys.exit(1)
print(best)
PY
) || {
    echo "test_pack_rc: no finished stove build of $sha" >&2
    exit 1
}
echo "stove $TREE"
job=$(basename "$(dirname "$TREE")")

need_mark() {
    file=$1
    text=$2
    if [ ! -f "$file" ]; then
        echo "test_pack_rc: missing $file" >&2
        exit 1
    fi
    if ! grep -a -F -q "$text" "$file"; then
        echo "test_pack_rc: $file has no $text" >&2
        exit 1
    fi
}

check_tree() {
    dir=$1
    need_mark "$dir/GFOpenMake" "$VER_MAKE"
    need_mark "$dir/GFOpenSlicer" "$VER_SLICER"
    need_mark "$dir/GFOpenBurn" "$VER_BURN"
    need_mark "$dir/GFOpenCNC" "$VER_CNC"
    need_mark "$dir/GFOpenWorkshop" "$VER_WORKSHOP"
    for f in GFOpenMake GFOpenSlicer GFOpenBurn GFOpenCNC GFOpenWorkshop; do
        need_mark "$dir/$f" "$STACK"
        echo "ok $dir/$f $(wc -c < "$dir/$f" | tr -d ' ')"
    done
}

check_tree "$TREE/fpu"

if command -v gflha >/dev/null 2>&1; then
    gflha_bin=$(command -v gflha)
else
    src=$(mktemp -d)
    git clone --depth 1 --quiet https://github.com/DalsinAI/gfopenlha.git "$src"
    if command -v cc >/dev/null 2>&1; then
        CC=cc
    elif command -v gcc >/dev/null 2>&1; then
        CC=gcc
    else
        echo "test_pack_rc: no C compiler for gflha" >&2
        exit 1
    fi
    "$CC" -std=c99 -O2 -o "$src/gflha" "$src/src/gflha.c"
    gflha_bin=$src/gflha
fi

stage=$(mktemp -d)
trap 'rm -rf "$stage"' EXIT

pack_one() {
    kind=$1
    name=$2
    blurb=$3
    src=$TREE/$kind
    drawer=$stage/$name
    rm -rf "$drawer"
    mkdir -p "$drawer"
    cp -a "$src/." "$drawer/"
    for extra in examples/print examples/laser examples/cnc examples/electronics; do
        if [ ! -d "$root/$extra" ]; then
            echo "test_pack_rc: $extra is not in the checkout" >&2
            exit 1
        fi
        mkdir -p "$drawer/$extra"
        cp -a "$root/$extra/." "$drawer/$extra/"
        if ! find "$drawer/$extra" -type f | grep -q .; then
            echo "test_pack_rc: $extra copied no files" >&2
            exit 1
        fi
    done
    cat > "$drawer/README.txt" << EOF
GFOpenMake suite 0.3.0-rc3
$name. Not a hardware sign-off.

Commit $sha
Stove os32-gcc16, job $job
$blurb

Copy this drawer somewhere on the Amiga and run the programs from it.
PROGDIR is this drawer. Each program has a \$VER: line and a \$STACK:65536 line.

  GFOpenSlicer      $VER_SLICER
  GFOpenMake        $VER_MAKE
  GFOpenBurn        $VER_BURN
  GFOpenCNC         $VER_CNC
  GFOpenWorkshop    $VER_WORKSHOP

machines, materials, settings and examples/cad come from the stove.
examples/print, examples/laser, examples/cnc and examples/electronics come from the same commit.
The Test* programs are the stove's checks, not the review programs.

The window code is linked in. These programs do not open gfopenlayout.library.
"GFOpenLayout is not open" means olui_new failed, not that a library is missing.
Make, Slicer and Burn use gfopengpu.library when it is open, and the pen drawer when it is not.
The theme asks for gfopenfont.library (1.2 wants a 68040 with an FPU) and otherwise uses the screen font.
cybergraphics.library is optional. asl.library is the file requester. rexxsyslib.library is the ARexx port.
Workshop starts PROGDIR:gfamigakicad for an electronics project, or that name on the path. That program is not in this suite.
EOF
    (
        cd "$drawer"
        sha256sum GFOpenMake GFOpenSlicer GFOpenBurn GFOpenCNC GFOpenWorkshop > SHA256SUMS
    )
    lha=$OUT/$name.lha
    rm -f "$lha"
    (
        cd "$stage"
        TZ=UTC "$gflha_bin" r "$lha" "$name"
    )
    TZ=UTC "$gflha_bin" t "$lha"
    sha256sum "$lha" | tee "$OUT/$name.sha256"
    cp "$drawer/SHA256SUMS" "$OUT/SHA256SUMS-$kind"
    echo "archive $name $(wc -c < "$lha" | tr -d ' ') bytes"
}

pack_one fpu "$FPU_NAME" "Compiled -m68040 -mhard-float, for a 68040 or 68060 with an FPU. The build rejects an FPU instruction a 68040 traps. Not run on a 68020. Will not run with no FPU. No soft-float archive: the stove's -msoft-float output still has FPU instructions, and the build rejects it."

if ! command -v gh >/dev/null 2>&1; then
    echo "test_pack_rc: gh is not installed; the archives are in OUT only" >&2
    exit 2
fi

fpu_hash=$(awk '{print $1}' "$OUT/$FPU_NAME.sha256")

suite_notes="GFOpenMake suite 0.3.0-rc3, for review. Not a hardware sign-off.

Commit $sha. Stove os32-gcc16, job $job.

\`$FPU_NAME.lha\` is compiled -m68040 -mhard-float, for a 68040 or 68060 with an FPU. The build rejects an FPU instruction a 68040 traps. It was not run on a 68020, and it will not run with no FPU. SHA-256 \`$fpu_hash\`. There is no soft-float archive. A stove build with -msoft-float still contained FPU instructions, and the build rejects that.

The drawer is the five programs, machines, materials, settings, and examples/cad, print, laser, cnc and electronics. Copy the drawer and run the programs from it. Each program has a \`\$VER:\` line and a \`\$STACK:65536\` line. \`SHA256SUMS\` lists the five programs, not the examples and not the archive.

The window code is linked in. The programs do not open gfopenlayout.library. Make, Slicer and Burn use gfopengpu.library when it is open. The theme asks for gfopenfont.library and otherwise uses the screen font. Workshop can start PROGDIR:gfamigakicad; that program is not in this suite."

upload() {
    tag=$1
    title=$2
    notes=$3
    sums=$OUT/SHA256SUMS
    cp "$OUT/SHA256SUMS-fpu" "$sums"
    if gh release view "$tag" --repo "$REPO" >/dev/null 2>&1; then
        gh release upload "$tag" \
            "$OUT/$FPU_NAME.lha" "$sums" \
            --repo "$REPO" --clobber
        body=$(gh release view "$tag" --repo "$REPO" --json body -q .body)
        case "$body" in
            *"$FPU_NAME.lha"*) echo "notes already name the archive ($tag)" ;;
            *) gh release edit "$tag" --repo "$REPO" --notes "${body}

${notes}" ;;
        esac
    else
        gh release create "$tag" \
            "$OUT/$FPU_NAME.lha" "$sums" \
            --repo "$REPO" --prerelease --target "$sha" --title "$title" --notes "$notes"
    fi
    echo "uploaded $tag"
}

upload "$SUITE_TAG" "GFOpenMake suite 0.3.0-rc3" "$suite_notes"
upload gfopenslicer-v0.3.0-rc3 "GFOpenSlicer 0.3.0-rc3" "GFOpenSlicer 0.3.0-rc3. The FPU binary is in \`$FPU_NAME.lha\` (stove build of $sha, -m68040 -mhard-float). Not a hardware sign-off. No soft-float archive. \`SHA256SUMS\` lists the five programs."
upload gfopenmake-v0.3.0-rc3 "GFOpenMake 0.3.0-rc3" "GFOpenMake 0.3.0-rc3. The FPU binary is in \`$FPU_NAME.lha\` (stove build of $sha, -m68040 -mhard-float). Not a hardware sign-off. No soft-float archive. \`SHA256SUMS\` lists the five programs."
upload openmakeburn-v0.1.0-rc3 "OpenMakeBurn 0.1.0-rc3" "OpenMakeBurn 0.1.0-rc3. The FPU binary is in \`$FPU_NAME.lha\` (stove build of $sha, -m68040 -mhard-float). Not a hardware sign-off. No soft-float archive. \`SHA256SUMS\` lists the five programs."
upload gfopencnc-v0.1.0-rc3 "GFOpenCNC 0.1.0-rc3" "GFOpenCNC 0.1.0-rc3. The FPU binary is in \`$FPU_NAME.lha\` (stove build of $sha, -m68040 -mhard-float). Not a hardware sign-off. No soft-float archive. \`SHA256SUMS\` lists the five programs."
upload gfopenworkshop-v0.1.0-rc3 "GFOpenWorkshop 0.1.0-rc3" "GFOpenWorkshop 0.1.0-rc3. The window title and the \`\$VER:\` line are GFOpenMakerWorkshop 0.1.0-rc3. The FPU binary is in \`$FPU_NAME.lha\` (stove build of $sha, -m68040 -mhard-float). Not a hardware sign-off. No soft-float archive."
