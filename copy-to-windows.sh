#!/bin/sh
_linkDLLs
rm -rf copy-to-windows
mkdir copy-to-windows
cp -rL $prefix/bin/*.{exe,dll} copy-to-windows/
cp copy-to-windows/nix.exe copy-to-windows/nix-instantiate.exe

# _linkDLLs creates symlinks but doesn't copy them
# meaning that recursive dependencies don't get copied!
# For example, libhttp_parser.2.9.4.dll which libgit2 needs.
for i in $(echo "$LINK_DLL_FOLDERS" | tr ':' ' '); do
  for dll in $(find $i -name "*.dll"); do
    if [ ! -e "copy-to-windows/$(basename $dll)" ]; then
      cp $dll copy-to-windows/
    fi
  done
done

rm -rf /mnt/c/nix
cp -r copy-to-windows /mnt/c/nix
