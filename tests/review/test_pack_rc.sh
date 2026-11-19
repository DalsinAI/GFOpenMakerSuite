#!/bin/sh
# Pack the FPU review drawer from the newest finished stove build of this
# commit and attach it to the v0.3.0-rc2 prereleases. Runs on Daletop.
# Does not commit the binaries.
set -eu

REPO=DalsinAI/GFOpenMakerSuite
SUITE_TAG=v0.3.0-rc2
DRAWER_NAME=OpenMake-0.3.0-rc2-fpu
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

FPU=$(SHA="$sha" python3 - "$JOBS_A" "$JOBS_B" << 'PY'
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
        fpu = os.path.join(root, name, "out", "fpu")
        if not os.path.isfile(os.path.join(fpu, "GFOpenMake")):
            continue
        key = rec.get("endedAt") or ""
        if best is None or key > best_key:
            best = fpu
            best_key = key
if not best:
    sys.exit(1)
print(best)
PY
) || {
    echo "test_pack_rc: no finished stove build of $sha" >&2
    exit 1
}
echo "stove $FPU"

need_ver() {
    file=$1
    text=$2
    if [ ! -f "$FPU/$file" ]; then
        echo "test_pack_rc: missing $file" >&2
        exit 1
    fi
    if ! grep -a -F -q "$text" "$FPU/$file"; then
        echo "test_pack_rc: $file has no $text" >&2
        exit 1
    fi
    echo "ok $file $(wc -c < "$FPU/$file" | tr -d ' ')"
}

need_ver GFOpenMake '$VER: GFOpenMake 0.3.0-rc2 (19.11.2026) AmiCradle Team'
need_ver GFOpenSlicer '$VER: GFOpenSlicer 0.3.0-rc2 (19.11.2026) AmiCradle Team'
need_ver GFOpenBurn '$VER: OpenMakeBurn 0.1.0-rc2 (19.11.2026) AmiCradle Team'
need_ver GFOpenCNC '$VER: GFOpenCNC 0.1.0-rc2 (19.11.2026) AmiCradle Team'
need_ver GFOpenWorkshop '$VER: GFOpenWorkshop 0.1.0-rc2 (19.11.2026) AmiCradle Team'

stage=$(mktemp -d)
trap 'rm -rf "$stage"' EXIT
drawer=$stage/$DRAWER_NAME
mkdir -p "$drawer"
cp -a "$FPU/." "$drawer/"

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

job=$(basename "$(dirname "$(dirname "$FPU")")")
cat > "$drawer/README.txt" << EOF
GFOpenMake suite 0.3.0-rc2
FPU executables for review. Not a hardware sign-off.

Commit $sha
Stove os32-gcc16, job $job
Built for a 68020 or later with a 68881 or 68882. These programs will not run on a machine with no FPU. There is no soft-float build in this archive.

Copy this drawer somewhere on the Amiga and run the programs from it.
PROGDIR is this drawer.

  GFOpenSlicer      reports GFOpenSlicer 0.3.0-rc2
  GFOpenMake        reports GFOpenMake 0.3.0-rc2
  GFOpenBurn        reports OpenMakeBurn 0.1.0-rc2
  GFOpenCNC         reports GFOpenCNC 0.1.0-rc2
  GFOpenWorkshop    reports GFOpenMakerWorkshop 0.1.0-rc2

Each of those five has a \$VER: line (Amiga version command).
machines, materials, settings and examples/cad come from the stove.
examples/print, examples/laser, examples/cnc and examples/electronics come from the same commit.
The Test* programs are the stove's checks, not the review programs.

Not in this drawer, and required on the machine: GFOpenLayout, and
gfopengpu.library (Make, Slicer, Burn), asl.library, rexxsyslib.library,
diskfont.library, dos.library, graphics.library, intuition.library,
layers.library, cybergraphics.library, gfopenfont.library, keymap.library,
mathieeedoubtrans.library, mathieeesingbas.library, mathieeesingtrans.library.
Workshop starts PROGDIR:gfamigakicad for an electronics project. That program is not in this suite.
EOF

(
    cd "$drawer"
    sha256sum GFOpenMake GFOpenSlicer GFOpenBurn GFOpenCNC GFOpenWorkshop > SHA256SUMS
)

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

lha=$OUT/$DRAWER_NAME.lha
rm -f "$lha"
(
    cd "$stage"
    TZ=UTC "$gflha_bin" r "$lha" "$DRAWER_NAME"
)
TZ=UTC "$gflha_bin" t "$lha"
sha256sum "$lha" | tee "$OUT/lha.sha256"
cp "$drawer/SHA256SUMS" "$OUT/SHA256SUMS"
echo "archive $(wc -c < "$lha" | tr -d ' ') bytes"

if ! command -v gh >/dev/null 2>&1; then
    echo "test_pack_rc: gh is not installed; the archive is in OUT only" >&2
    exit 2
fi

suite_notes="GFOpenMake suite 0.3.0-rc2, for review. Not a hardware sign-off.

Commit $sha. Stove os32-gcc16, job $job. 68020 or later with a 68881 or 68882. No soft-float build.

\`$DRAWER_NAME.lha\` is the drawer: the five programs, machines, materials, settings, examples/cad, examples/print, examples/laser, examples/cnc and examples/electronics. \`SHA256SUMS\` is the five programs. Copy the drawer and run the programs from it.

Each program keeps a \`\$VER:\` line. The window still needs GFOpenLayout, and Make, Slicer and Burn need gfopengpu.library. asl.library, rexxsyslib.library and the FPU maths libraries are not in the archive. Workshop can start PROGDIR:gfamigakicad; that program is not in this suite."

upload() {
    tag=$1
    title=$2
    notes=$3
    if gh release view "$tag" --repo "$REPO" >/dev/null 2>&1; then
        gh release upload "$tag" "$lha" "$OUT/SHA256SUMS" --repo "$REPO" --clobber
        body=$(gh release view "$tag" --repo "$REPO" --json body -q .body)
        case "$body" in
            *"$DRAWER_NAME.lha"*) echo "notes already name the archive ($tag)" ;;
            *) gh release edit "$tag" --repo "$REPO" --notes "${body}

${notes}" ;;
        esac
    else
        gh release create "$tag" "$lha" "$OUT/SHA256SUMS" --repo "$REPO" \
            --prerelease --target "$sha" --title "$title" --notes "$notes"
    fi
    echo "uploaded $tag"
}

upload "$SUITE_TAG" "GFOpenMake suite 0.3.0-rc2" "$suite_notes"
upload gfopenslicer-v0.3.0-rc2 "GFOpenSlicer 0.3.0-rc2" "GFOpenSlicer 0.3.0-rc2. The FPU binary is in \`$DRAWER_NAME.lha\` on this release (the suite stove build, 68881/68882). Not a hardware sign-off. \`SHA256SUMS\` lists the five programs."
upload gfopenmake-v0.3.0-rc2 "GFOpenMake 0.3.0-rc2" "GFOpenMake 0.3.0-rc2. The FPU binary is in \`$DRAWER_NAME.lha\` on this release (the suite stove build, 68881/68882). Not a hardware sign-off. \`SHA256SUMS\` lists the five programs."
upload openmakeburn-v0.1.0-rc2 "OpenMakeBurn 0.1.0-rc2" "OpenMakeBurn 0.1.0-rc2. The FPU binary is in \`$DRAWER_NAME.lha\` on this release (the suite stove build, 68881/68882). Not a hardware sign-off. \`SHA256SUMS\` lists the five programs."
upload gfopencnc-v0.1.0-rc2 "GFOpenCNC 0.1.0-rc2" "GFOpenCNC 0.1.0-rc2. The FPU binary is in \`$DRAWER_NAME.lha\` on this release (the suite stove build, 68881/68882). Not a hardware sign-off. \`SHA256SUMS\` lists the five programs."
upload gfopenworkshop-v0.1.0-rc2 "GFOpenWorkshop 0.1.0-rc2" "GFOpenWorkshop 0.1.0-rc2 (window title GFOpenMakerWorkshop 0.1.0-rc2). The FPU binary is in \`$DRAWER_NAME.lha\` on this release (the suite stove build, 68881/68882). Not a hardware sign-off. \`SHA256SUMS\` lists the five programs."
