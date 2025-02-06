#!/usr/bin/env bash

copied=1
while [ "$copied" -ne 0 ]; do
  copied=0
  for i in src/nix/{*.exe,lib*.dll}; do
    for j in $("/mnt/d/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.42.34433/bin/Hostx64/x64/dumpbin.exe" /dependents $i | grep '^[ ].*\.dll' | grep -v 'msvcrt\.dll' | grep -v 'ADVAPI32\.dll' | grep -v 'KERNEL32\.dll' | grep -v 'CRYPT32\.dll' | grep -v 'USER32\.dll' | grep -v 'WS2_32\.dll' | tr -d '\r'); do
      if [ -f src/nix/$j ]; then
        continue
      fi
      found="$(find {/nix/store,src} -maxdepth 4 -name $j -type f | head -n1)"
      if [ -z "$found" ]; then
        continue
      fi
      copied=1
      cp -v "$found" src/nix/ 2>/dev/null
    done
  done
done
