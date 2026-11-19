#!/bin/sh
# Pack the FPU review drawer from stove job j20261119T070038-9bae41 and
# attach it to the v0.3.0-rc1 prerelease. Meant to run on Daletop, where
# that job's out/fpu is on disk. Does not commit the binaries.
set -eu

FPU=/run/media/da1ek/WorkingSpace/AmigaChrome/archive/daletop-offload-20261023/AmigaChrome-dev/cradle-builds/jobs/j20261119T070038-9bae41/out/fpu
REPO=DalsinAI/GFOpenMakerSuite
TAG=v0.3.0-rc1

if [ -z "${OUT:-}" ]; then
    echo "test_pack_rc: OUT is not set" >&2
    exit 1
fi
if [ ! -d "$FPU" ]; then
    echo "test_pack_rc: stove output is not there: $FPU" >&2
    exit 1
fi

check() {
    file=$1
    want=$2
    size=$3
    got=$(sha256sum "$FPU/$file" | awk '{print $1}')
    bytes=$(wc -c < "$FPU/$file" | tr -d ' ')
    if [ "$got" != "$want" ] || [ "$bytes" != "$size" ]; then
        echo "test_pack_rc: $file does not match ($got $bytes)" >&2
        exit 1
    fi
    echo "ok $file $bytes"
}

check GFOpenMake 68b6f97f050b0d3ccff4ac00be6a110938553462d382c26d0e35ea4ae8a9088a 578160
check GFOpenSlicer 1040532c1d395e62b70ec70db4cce5ca7ff8695f36fc41bc044010d124ab10ad 472068
check GFOpenBurn 50d0611f855cfbe166e1f2eb422246c5f25b78878de2821cda2b302353b90b0e 544468
check GFOpenCNC 09acb19d1f7169a11bfabe35a6ea4af15442fae1dc6b23d051d7069cebecb014 481724
check GFOpenWorkshop 410cd9e1a76d24f259fb768dd7b9bd41407363e38a69d18778d39afe0cfd0db1 483108

stage=$(mktemp -d)
trap 'rm -rf "$stage"' EXIT
drawer=$stage/OpenMake-0.3.0-rc1-fpu
mkdir -p "$drawer"
cp -a "$FPU/." "$drawer/"
cat > "$drawer/README.txt" << 'EOF'
GFOpenMake suite 0.3.0-rc1
FPU executables for review. Not a hardware sign-off.

Commit 6f628ddeed83741df45f17bab0808c79389e7fcb
Stove os32-gcc16, job j20261119T070038-9bae41
Built for a 68020 or later with a 68881 or 68882. These programs will not run on a machine with no FPU.

Copy this drawer somewhere on the Amiga and run the programs from it.
PROGDIR is this drawer: machines, materials, settings and examples/cad sit beside the programs.

  GFOpenSlicer      reports 0.3.0-rc1
  GFOpenMake        reports 0.3.0-rc1
  GFOpenBurn        reports OpenMakeBurn 0.1.0-rc1
  GFOpenCNC         reports 0.1.0-rc1
  GFOpenWorkshop    reports GFOpenMakerWorkshop 0.1.0-rc1

Host make test passed before this build. The binaries are the stove FPU output, checked by SHA-256.
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

lha=$OUT/OpenMake-0.3.0-rc1-fpu.lha
rm -f "$lha"
(
    cd "$stage"
    TZ=UTC "$gflha_bin" r "$lha" OpenMake-0.3.0-rc1-fpu
)
TZ=UTC "$gflha_bin" t "$lha"
sha256sum "$lha" | tee "$OUT/lha.sha256"
cp "$drawer/SHA256SUMS" "$OUT/SHA256SUMS"
echo "archive $(wc -c < "$lha" | tr -d ' ') bytes"

if ! command -v gh >/dev/null 2>&1; then
    echo "test_pack_rc: gh is not installed; the archive is in OUT only" >&2
    exit 2
fi

gh release upload "$TAG" "$lha" "$OUT/SHA256SUMS" --repo "$REPO"
echo "uploaded $TAG"

body=$(gh release view "$TAG" --repo "$REPO" --json body -q .body)
case "$body" in
    *OpenMake-0.3.0-rc1-fpu.lha*)
        echo "notes already name the archive"
        ;;
    *)
        gh release edit "$TAG" --repo "$REPO" --notes "${body}

FPU archive for review: \`OpenMake-0.3.0-rc1-fpu.lha\` (68881/68882, stove job j20261119T070038-9bae41). Not a hardware sign-off. \`SHA256SUMS\` lists the five programs.
"
        ;;
esac
