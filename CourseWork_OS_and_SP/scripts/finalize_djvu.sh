#!/usr/bin/env bash
# Скрипт собирает итоговый DJVU-документ из всех промежуточных DJVU-файлов.
# Путь к итоговому файлу передаётся через опцию -o/--output.

set -euo pipefail

print_usage() {
    echo "Usage: $0 -o <output_file>" >&2
}

output_file=""
while [[ "$#" -gt 0 ]]; do
    case "$1" in
        -o|--output)
            if [[ "$#" -lt 2 ]]; then
                echo "Error: missing value for '$1'." >&2
                print_usage
                exit 1
            fi
            output_file="$2"
            shift 2
            ;;
        -h|--help)
            print_usage
            exit 0
            ;;
        --)
            shift
            break
            ;;
        -*)
            echo "Error: unknown option '$1'." >&2
            print_usage
            exit 1
            ;;
        *)
            echo "Error: positional arguments are not supported: '$1'." >&2
            print_usage
            exit 1
            ;;
    esac
done

if [[ "$#" -ne 0 ]]; then
    echo "Error: unexpected positional arguments after '--'." >&2
    print_usage
    exit 1
fi

if [[ -z "$output_file" ]]; then
    echo "Error: option -o/--output is required." >&2
    print_usage
    exit 1
fi

output_dir="${PROCESS_OUTPUT_DIR:-./work}"

if ! command -v djvm >/dev/null 2>&1; then
    echo "Error: required tool 'djvm' (djvulibre) not found in PATH." >&2
    exit 2
fi

if [[ ! -d "$output_dir" ]]; then
    echo "Error: output directory '$output_dir' does not exist." >&2
    exit 1
fi

mkdir -p -- "$(dirname -- "$output_file")"
rm -f -- "$output_file"

mapfile -d '' djvu_files < <(find "$output_dir" -maxdepth 1 -type f -name '*.djvu' -print0 | sort -z)
if [[ "${#djvu_files[@]}" -eq 0 ]]; then
    echo "Error: no intermediate DJVU files found in '$output_dir'." >&2
    exit 1
fi

djvm -c "$output_file" "${djvu_files[@]}"
