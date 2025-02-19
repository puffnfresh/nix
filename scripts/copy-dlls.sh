#!/usr/bin/env bash

if [ "$#" = 0 ]
then
    echo "Usage: source ${BASH_SOURCE[0]} <build_dir> <dest>" >&2
    return 1
fi

while IFS= read -r -d '' dll; do
  ln -fsv "$(realpath "$dll")" "$2/"
done < <(find "$1" -name "*.dll" -type f -print0)
linkDLLsInfolder "$2"
