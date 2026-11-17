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
# instruction. traps: only those the 68040 and 68060 trap to software
# (FSIN, FCOS, FETOX and the rest, 68040.library).
# MIT, Copyright (c) 2026 AmiCradle Team.
set -eu
OBJDUMP=${OBJDUMP:?"OBJDUMP: m68k-amigaos-objdump"}
f=$1
if [ "${2:-}" = traps ]; then
    want='^f(sin|cos|tan|atan|asin|acos|etox|twotox|tentox|log|sinh|cosh|tanh|getexp|getman|rem|mod|scale)'
else
    want='^f'
fi
{
    "$OBJDUMP" -r "$f" | awk '$2 == "RELOC32" { print "R", $1 }'
    "$OBJDUMP" -m m68k:68040 -d "$f" | awk -F'\t' 'NF >= 3 { a = $1; gsub(/[ :]/, "", a); print "I", a, $3 }'
} | awk -v want="$want" '
function hex(s,    i, n) {
    n = 0
    s = tolower(s)
    for (i = 1; i <= length(s); i++) n = n * 16 + index("0123456789abcdef", substr(s, i, 1)) - 1
    return n
}
$1 == "R" { r = hex($2); for (k = 0; k < 4; k++) moved[r + k] = 1; next }
$1 == "I" {
    op = $0
    sub(/^I [^ ]+ /, "", op)
    if (op ~ want && op !~ /d0,%?fpcr$/ && !(hex($2) in moved)) n++
}
END { print n + 0 }'
