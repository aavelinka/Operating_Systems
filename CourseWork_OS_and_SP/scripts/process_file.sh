#!/bin/sh
# Скрипт обрабатывает один входной TIFF-файл и создаёт соответствующий DJVU-файл.
# Входной файл передаётся первым позиционным аргументом.

set -eu

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <input_file>" >&2
    exit 1
fi

input_file="$1"
output_dir="${PROCESS_OUTPUT_DIR:-./work}"
output_extension="${PROCESS_OUTPUT_EXTENSION:-.djvu}"
tmp_dir="${TMPDIR:-/tmp}"
tmp_file=""

cleanup() {
    if [ -n "$tmp_file" ] && [ -f "$tmp_file" ]; then
        rm -f -- "$tmp_file"
    fi
}

trap cleanup EXIT INT TERM

if [ ! -f "$input_file" ]; then
    echo "Error: input file '$input_file' does not exist." >&2
    exit 1
fi

if ! mkdir -p -- "$output_dir"; then
    echo "Error: cannot create output directory '$output_dir'." >&2
    exit 1
fi

if ! command -v c44 >/dev/null 2>&1; then
    echo "Error: required tool 'c44' (djvulibre) not found in PATH." >&2
    exit 2
fi

tmp_file="$(mktemp "${tmp_dir%/}/dispatcher_scan_XXXXXX.pnm")"
input_name="$(basename -- "$input_file")"
file_stem="${input_name%.*}"
output_file="${output_dir%/}/${file_stem}${output_extension}"

if command -v tifftopnm >/dev/null 2>&1; then
    tifftopnm "$input_file" > "$tmp_file"
elif command -v gm >/dev/null 2>&1; then
    gm convert "$input_file" "$tmp_file"
elif command -v convert >/dev/null 2>&1; then
    convert "$input_file" "$tmp_file"
else
    echo "Error: need netpbm 'tifftopnm' or GraphicsMagick/ImageMagick ('gm'/'convert')." >&2
    exit 2
fi

c44 "$tmp_file" "$output_file"
