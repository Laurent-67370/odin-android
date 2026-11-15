#!/bin/sh
# Odin's sources at the commit of odin.version, with their submodules, in
# odin-src/, the Android patch applied.
set -eu
here=$(cd "$(dirname "$0")" && pwd)
commit=$(cat "$here/odin.version")
src="$here/odin-src"

if [ ! -d "$src/.git" ]; then
	git init -q "$src"
	git -C "$src" remote add origin https://github.com/Tovarichtch/Odin.git
fi
git -C "$src" fetch -q --depth 1 origin "$commit"
git -C "$src" checkout -q -f FETCH_HEAD
git -C "$src" clean -q -fdx
git -C "$src" submodule update -q --init --depth 1
for patch in "$here"/patches/*.patch; do
	git -C "$src" apply --whitespace=nowarn "$patch"
	echo "applied $(basename "$patch")"
done
echo "Odin $commit ready in $src"
