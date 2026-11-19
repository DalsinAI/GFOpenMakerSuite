#!/bin/sh
# Pack the files a lab run opens, under OUT, with the paths the programs use.
# PROGDIR:examples/cad is the editor's shelf. The tests open
# core/tests/data, burn/tests/data and machines from the current directory.
set -eu
if [ -z "${OUT:-}" ]; then
    echo "test_shelf: OUT is not set" >&2
    exit 1
fi

take() {
    mkdir -p "$OUT/$(dirname "$1")"
    cp "$1" "$OUT/$1"
}

find examples/cad core/tests/data burn/tests/data -type f | while IFS= read -r rel; do
    take "$rel"
done
find machines -name '*.om' -type f | while IFS= read -r rel; do
    take "$rel"
done

echo "test_shelf: $(find "$OUT" -type f | wc -l) files"
