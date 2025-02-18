#!/usr/bin/env bash

if [ "$#" = 0 ]
then
    echo "Usage: source $BASH_SOURCE <build_dir> <dest>" >&2
    return 1
fi

for i in $(find "$1" -name "*.dll" -type f); do
  ln -fsv "$(realpath $i)" "$2/"
done
linkDLLsInfolder "$2"
