#!/bin/sh
# How many FPU instructions an Amiga program or library has, for
# build-amiga.sh and build-libraries.sh:
#
#   OBJDUMP=.../m68k-amigaos-objdump sh fpu-count.sh FILE [traps]
#
# Decoded as a 68040's, or objdump shows FPU words as .short and misses
# them. Every FPU mnemonic starts with f; no other does. libnix's start-up
# clears the FPCR only when AttnFlags says there is an FPU, so that one is
# not counted. Nor is a word inside a relocated long: that is an address
# in a table among the code (0x0003f23a reads as fintrz), not an
# instruction.
#
# This target puts C string literals in .text, so objdump -d also walks
# those bytes as code and can lose sync. gcc's usual prologue after a
# pool is addaw #N,sp then moveml (the words defc, N, 48e7). When N is
# 0xfxxx, a lost sync in the strings decodes that immediate as frestore
# (as after the "Fill" / "Offset" labels in OpenMakeBurn). The word is
# the immediate of the prologue, not an FPU instruction, and is not
# counted.
#
# traps: only those the 68040 and 68060 trap to software
# (FSIN, FCOS, FETOX and the rest, 68040.library).
# MIT, Copyright (c) 2026 AmiCradle Team.
set -eu
OBJDUMP=${OBJDUMP:?"OBJDUMP: m68k-amigaos-objdump"}
export OBJDUMP
if ! command -v python3 >/dev/null 2>&1; then
    echo "fpu-count.sh: python3 is required" >&2
    exit 1
fi
python3 - "$1" "${2:-}" <<'PY'
import os, re, subprocess, sys

objdump = os.environ["OBJDUMP"]
path = sys.argv[1]
traps = sys.argv[2] == "traps"
if traps:
    want = re.compile(
        r"^f(sin|cos|tan|atan|asin|acos|etox|twotox|tentox|log|"
        r"sinh|cosh|tanh|getexp|getman|rem|mod|scale)"
    )
else:
    want = re.compile(r"^f")
fpcr = re.compile(r"d0,%?fpcr$")
hex_tok = re.compile(r"^[0-9a-fA-F]{2,8}$")


def hexint(s):
    return int(s, 16)


def text_bytes(path):
    out = subprocess.check_output(
        [objdump, "-s", "-j", ".text", path], text=True
    )
    chunks = []
    for line in out.splitlines():
        m = re.match(
            r"^\s*([0-9a-fA-F]+)\s+((?:[0-9a-fA-F]{2,8}\s*){1,4})", line
        )
        if not m:
            continue
        addr = hexint(m.group(1))
        words = [
            t
            for t in m.group(2).split()
            if hex_tok.match(t) and len(t) % 2 == 0
        ]
        if words:
            chunks.append((addr, bytes.fromhex("".join(words))))
    if not chunks:
        return 0, bytearray()
    base = chunks[0][0]
    size = max(a + len(b) for a, b in chunks) - base
    data = bytearray(size)
    for a, b in chunks:
        data[a - base : a - base + len(b)] = b
    return base, data


def word_at(data, base, addr):
    off = addr - base
    if off < 0 or off + 2 > len(data):
        return None
    return (data[off] << 8) | data[off + 1]


def is_prologue_immediate(data, base, addr):
    # addaw #N,sp ; moveml ...,sp@-: N sits at addr.
    return (
        word_at(data, base, addr - 2) == 0xDEFC
        and word_at(data, base, addr + 2) == 0x48E7
    )


reloc = set()
for line in subprocess.check_output([objdump, "-r", path], text=True).splitlines():
    parts = line.split()
    if len(parts) >= 2 and parts[1] == "RELOC32":
        r = hexint(parts[0])
        reloc.update(range(r, r + 4))

base, data = text_bytes(path)
n = 0
for line in subprocess.check_output(
    [objdump, "-m", "m68k:68040", "-d", path], text=True
).splitlines():
    if "\t" not in line:
        continue
    parts = line.split("\t")
    if len(parts) < 3:
        continue
    a = parts[0].replace(" ", "").replace(":", "")
    try:
        addr = hexint(a)
    except ValueError:
        continue
    op = parts[2].strip()
    if not want.search(op):
        continue
    if fpcr.search(op):
        continue
    if addr in reloc:
        continue
    if is_prologue_immediate(data, base, addr):
        continue
    n += 1
print(n)
PY
