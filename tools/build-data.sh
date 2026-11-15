#!/bin/sh
# Odin's data files for the APK, laid out as Odin's CMake lays out
# build/share/odin: odin-games.json, shaders/*.txt, filters/ with each
# filter's GLSL compiled to SPIR-V (the same steps as CMakeLists.txt).
#   build-data.sh <odin-src> <out>
set -eu
src=$1
out=$2
glslang=${GLSLANG:-glslangValidator}
awk_script="$src/src/odin/host/video/filter_spirv.awk"

rm -rf "$out"
mkdir -p "$out"
cp "$src/odin-games.json" "$out/"
if ls "$src"/shaders/*.txt > /dev/null 2>&1; then
	mkdir -p "$out/shaders"
	cp "$src"/shaders/*.txt "$out/shaders/"
fi
(cd "$src" && find filters -type f ! -name '*.spv') | while read -r file; do
	mkdir -p "$out/$(dirname "$file")"
	cp "$src/$file" "$out/$file"
done
for glsl in "$src"/filters/*/shaders/*.glsl; do
	[ -f "$glsl" ] || continue
	rel=${glsl#"$src"/}
	folder=$(dirname "$glsl")
	awk -v part=VERTEX -f "$awk_script" "$folder"/*.glsl "$glsl" |
		"$glslang" -V -R --amb --aml --sub vert 8 --stdin -S vert -o "$out/$rel.vert.spv" > /dev/null
	awk -v part=FRAGMENT -f "$awk_script" "$folder"/*.glsl "$glsl" |
		"$glslang" -V -R --amb --aml --stdin -S frag -o "$out/$rel.frag.spv" > /dev/null
done
# The list of files, for the activity that unpacks them
(cd "$out" && find . -type f ! -name files.txt | sed 's|^\./||' | sort > files.txt)
echo "odin data: $(wc -l < "$out/files.txt") files in $out"
