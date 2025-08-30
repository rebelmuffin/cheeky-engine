#!/bin/bash

SRC_DIR="src"
FORMAT_CMD="clang-format -i"
CHANGED=0

find "$SRC_DIR" -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" -o -name "*.c" \) | while read -r file; do
    # Save original file
    cp "$file" "$file.orig"
    # Run clang-format
    $FORMAT_CMD "$file"
    # Check for changes
    if ! cmp -s "$file" "$file.orig"; then
        CHANGED=1
        # Count changed lines
        DIFF_LINES=$(diff -u "$file.orig" "$file" | grep -E '^\+|^-' | grep -vE '^(\+\+\+|---)' | wc -l)
        echo "$file: $DIFF_LINES lines changed"
    fi
    rm "$file.orig"
done

if [ $CHANGED -eq 0 ]; then
    echo "No files changed."
fi