#!/usr/bin/env bash
# Measure the size of tinytcp, built the way it would be for a small target:
# -Os, no exceptions, no RTTI, section garbage collection, statically linked.
#
#   tools/size/measure.sh            measure every toolchain that is installed
#   tools/size/measure.sh host       host compiler only
#   tools/size/measure.sh rv64       riscv64-linux-gnu cross compiler only
#
# Reports, in bytes:
#   * object sizes of src/os and src/tinytcp (text/data/bss)
#   * a minimal program that needs the whole stack (min_app.cpp) and the full test_app,
#     linked statically, with the linked bytes attributed to libstdc++, libc and our code
#   * a floor: an empty static main() and a one-line iostream program, to put the above in context
#
# The libc here is the toolchain's static glibc, much larger than an embedded libc such as
# newlib or picolibc. Compare the libstdc++ and project-code numbers rather than the totals.
#
# Output goes to build/size/<label>/.
set -euo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
OUTROOT=${OUT:-$ROOT/build/size}

FLAGS="-std=c++14 -Os -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti"
INCLUDES="-I$ROOT/src -I$ROOT/src/os -I$ROOT/src/tinytcp -I$ROOT/src/packet_io -I$ROOT/src/web_server"

measure() {
    local label=$1 cxx=$2 size=$3 strip=$4
    if ! command -v "$cxx" >/dev/null 2>&1; then
        echo "== $label: $cxx not installed, skipping"
        return
    fi
    local out=$OUTROOT/$label
    rm -rf "$out"
    mkdir -p "$out"

    for f in "$ROOT"/src/os/*.cpp "$ROOT"/src/tinytcp/*.cpp "$ROOT"/src/packet_io/*.cpp \
             "$ROOT"/src/web_server/*.cpp; do
        $cxx $FLAGS $INCLUDES -c "$f" -o "$out/$(basename "$(dirname "$f")")_$(basename "${f%.cpp}").o"
    done
    $cxx $FLAGS $INCLUDES -DBINARY_DIR=\"/tmp/\" -c "$ROOT/test_app/TestApp.cpp" -o "$out/test_app.o"
    $cxx $FLAGS $INCLUDES -c "$HERE/min_app.cpp" -o "$out/min_app.o"

    echo "== $label ($($cxx --version | head -1))"
    for d in os tinytcp; do
        printf "  objects src/%-8s text/data/bss: " "$d"
        $size -t "$out"/${d}_*.o | tail -1 | awk '{print $1 " / " $2 " / " $3}'
    done

    local link="$cxx -static -Wl,--gc-sections"
    $link -Wl,-Map,"$out/min_app.map" -o "$out/min_app" "$out/min_app.o" \
        "$out"/os_*.o "$out"/tinytcp_*.o -lpthread
    $link -o "$out/test_app" "$out/test_app.o" "$out"/os_*.o "$out"/tinytcp_*.o \
        "$out"/packet_io_*.o "$out"/web_server_*.o -lpthread
    for b in min_app test_app; do
        printf "  %-8s linked: " "$b"
        $size "$out/$b" | tail -1 | awk '{printf "text=%d data=%d bss=%d (total %d)", $1, $2, $3, $1+$2+$3}'
        $strip "$out/$b" -o "$out/$b.stripped"
        printf "; stripped file %d\n" "$(stat -c %s "$out/$b.stripped")"
    done
    echo "  min_app contents (bytes of kept sections):"
    "$HERE/attribute.sh" "$out/min_app.map" | sed 's/^/  /'

    echo 'int main() { return 0; }' >"$out/empty.cpp"
    printf '#include <iostream>\nint main() { std::cout << "x"; return 0; }\n' >"$out/hello.cpp"
    for p in empty hello; do
        $link $FLAGS -o "$out/$p" "$out/$p.cpp"
    done
    printf "  floor: empty main    "
    $size "$out/empty" | tail -1 | awk '{printf "text=%d data=%d bss=%d (total %d)\n", $1, $2, $3, $1+$2+$3}'
    printf "  floor: iostream hello "
    $size "$out/hello" | tail -1 | awk '{printf "text=%d data=%d bss=%d (total %d)\n", $1, $2, $3, $1+$2+$3}'
}

want=${1:-all}
if [ "$want" = all ] || [ "$want" = host ]; then
    measure host g++ size strip
fi
if [ "$want" = all ] || [ "$want" = rv64 ]; then
    measure rv64 riscv64-linux-gnu-g++ riscv64-linux-gnu-size riscv64-linux-gnu-strip
fi
