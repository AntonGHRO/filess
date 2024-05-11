#!/bin/bash because I want to be able to have an array of words, which only works in bash, unlike sh

# Echo off debugging
set +x

# Check if the correct number of arguments is provided
if [ "$#" -ne 1 ]; then
    echo "$file" # Too Many Arguments Provided
    exit 1
fi

# File
file="$1"

# Check if the file exists -> Useless
if [ ! -f "$file" ]; then
    echo "$file" # File Does Not Exist
    exit 1
fi

# Get the number of lines, words, and characters in the file
num_lines=$(wc -l < "$file")
num_words=$(wc -w < "$file")
num_chars=$(wc -c < "$file")

# Check the conditions
if [ "$num_lines" -lt 3 ] && [ "$num_words" -gt 1000 ] && [ "$num_chars" -gt 2000 ]; then
    echo "$file"
    exit 1
fi

# Check for non-ASCII characters
if grep -qP "[^\x00-\x7F]" "$file"; then
    echo "$file" # Non Ascii Char Found
    exit 1
fi

# Words to search for
words=("corrupted" "dangerous" "risk" "attack" "malware" "malicious")

# Loop through each word and check if it exists in the file
for word in "${words[@]}"; do
    if grep -q "$word" "$file"; then
        echo "$file"
        exit 1
    fi
done

# If none of the words are found, exit with code 0
echo "SAFE"
exit 0
