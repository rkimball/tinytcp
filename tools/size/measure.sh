#!/usr/bin/env bash
# Measure the size of tinytcp for x86-64, RISC-V and ARM, built the way it would be for a small
# target: -Os, no exceptions, no RTTI, no thread-safe statics, section garbage collection and
# static linking. Prints a markdown table to stdout; progress and warnings go to stderr.
#
#   tools/size/measure.sh                       print the table
#   tools/size/measure.sh --details             also print where the linked bytes come from
#   tools/size/measure.sh --update-readme [F]   rewrite the table in README.md (or F), between
#                                               the <!-- size-table:begin/end --> markers
#
# What each row means:
#   Stack code / RAM   src/tinytcp objects (text + data), and sizeof(DefaultStack): every buffer
#                      the stack uses is inside that one object
#   OS layer           src/os objects: code (text + data) and RAM (data + bss)
#   C++ runtime        bytes of libstdc++ left in a minimal program that uses the whole stack
#   Minimal program    that program (tools/size/min_app.cpp), statically linked
#   Empty program      an empty main(), statically linked the same way
#   Net cost           minimal program minus empty program
#
# The C library is the toolchain's static glibc, much larger than an embedded libc such as newlib
# or picolibc, so the program totals are mostly libc. Compare the net cost and the stack rows.
# RISC-V is rv64gc and ARM is 32-bit ARMv7 with Thumb-2 and hard float. 32-bit RISC-V is not
# measured because the toolchain has no 32-bit libraries.
#
# Needs: g++, g++-riscv64-linux-gnu and g++-arm-linux-gnueabihf (a column is n/a without its
# compiler). Objects and binaries are left in build/size/<target>/.
set -euo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
OUTROOT=${OUT:-$ROOT/build/size}

FLAGS="-std=c++14 -Os -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-threadsafe-statics"
INCLUDES="-I$ROOT/src -I$ROOT/src/os -I$ROOT/src/tinytcp -I$ROOT/src/packet_io -I$ROOT/src/web_server"

# label|column title|compiler|size tool|strip tool|extra flags|package to install
TOOLCHAINS=(
    "x86-64|x86-64|g++|size|strip||g++"
    "riscv|RISC-V (rv64)|riscv64-linux-gnu-g++|riscv64-linux-gnu-size|riscv64-linux-gnu-strip||g++-riscv64-linux-gnu"
    "arm|ARM (Thumb-2)|arm-linux-gnueabihf-g++|arm-linux-gnueabihf-size|arm-linux-gnueabihf-strip|-mthumb|g++-arm-linux-gnueabihf"
)

DETAILS=0
README=""
while [ $# -gt 0 ]; do
    case $1 in
    --details) DETAILS=1 ;;
    --update-readme)
        README=$ROOT/README.md
        if [ $# -gt 1 ] && [[ $2 != --* ]]; then
            README=$2
            shift
        fi
        ;;
    -h | --help)
        sed -n '2,/^set -euo/p' "$0" | sed '$d; s/^# \{0,1\}//'
        exit 0
        ;;
    *)
        echo "unknown option: $1" >&2
        exit 2
        ;;
    esac
    shift
done

declare -A V     # V[<label>.<metric>] = bytes
declare -A TITLE # TITLE[<label>] = column title
LABELS=()

total3() { # text+data+bss of the last (totals) line of `size -t`
    "$@" | tail -1 | awk '{print $1 + $2 + $3}'
}

compile_dir() { # compile_dir <out> <flags> <source dir>...
    local out=$1 flags=$2
    shift 2
    local d f
    for d in "$@"; do
        for f in "$ROOT/src/$d"/*.cpp; do
            $CXX $flags $INCLUDES -c "$f" -o "$out/${d}_$(basename "${f%.cpp}").o"
        done
    done
}

measure() {
    local label=$1 title=$2 cxx=$3 size=$4 strip=$5 extra=$6 package=$7
    TITLE[$label]=$title
    LABELS+=("$label")
    if ! command -v "$cxx" >/dev/null 2>&1; then
        echo "measure.sh: $cxx not found; the '$title' column will be n/a (apt install $package)" >&2
        return
    fi
    echo "measure.sh: $title ($cxx)" >&2

    local out=$OUTROOT/$label flags="$FLAGS $extra" nm="${size%size}nm" link
    CXX=$cxx
    rm -rf "$out"
    mkdir -p "$out"
    link="$cxx $flags -static -Wl,--gc-sections"

    compile_dir "$out" "$flags" os tinytcp
    $cxx $flags $INCLUDES -c "$HERE/min_app.cpp" -o "$out/min_app.o"
    $cxx $flags $INCLUDES -c "$HERE/probe.cpp" -o "$out/probe.o"
    $link -Wl,-Map,"$out/min_app.map" -o "$out/min_app" "$out/min_app.o" \
        "$out"/os_*.o "$out"/tinytcp_*.o -lpthread
    echo 'int main() { return 0; }' >"$out/empty.cpp"
    $link -o "$out/empty" "$out/empty.cpp"

    local t d b
    read -r t d b _ < <($size -t "$out"/tinytcp_*.o | tail -1)
    V[$label.stack_code]=$((t + d))
    read -r t d b _ < <($size -t "$out"/os_*.o | tail -1)
    V[$label.os_code]=$((t + d))
    V[$label.os_ram]=$((d + b))
    V[$label.stack_ram]=$((16#$($nm -S "$out/probe.o" | awk '$4 == "tinytcp_stack_ram" {print $2}')))
    V[$label.cxx]=$("$HERE/attribute.sh" "$out/min_app.map" | awk '$1 == "libstdc++" {print $2}')
    V[$label.cxx]=${V[$label.cxx]:-0}
    V[$label.prog]=$(total3 $size "$out/min_app")
    V[$label.empty]=$(total3 $size "$out/empty")
    V[$label.net]=$((V[$label.prog] - V[$label.empty]))

    if [ "$DETAILS" = 1 ]; then
        compile_dir "$out" "$flags" packet_io web_server
        $cxx $flags $INCLUDES -DBINARY_DIR=\"/tmp/\" -c "$ROOT/test_app/TestApp.cpp" \
            -o "$out/test_app.o"
        $link -o "$out/test_app" "$out/test_app.o" "$out"/os_*.o "$out"/tinytcp_*.o \
            "$out"/packet_io_*.o "$out"/web_server_*.o -lpthread
        printf '#include <iostream>\nint main() { std::cout << "x"; return 0; }\n' >"$out/hello.cpp"
        $link -o "$out/hello" "$out/hello.cpp"
        V[$label.test_app]=$(total3 $size "$out/test_app")
        V[$label.hello]=$(total3 $size "$out/hello")
    fi
}

commas() { # 1234567 -> 1,234,567 (anything that is not a number is left alone)
    echo "$1" | sed -e :a -e 's/\(.*[0-9]\)\([0-9]\{3\}\)/\1,\2/;ta'
}

row() { # row <label text> <metric>
    local label text=$1 metric=$2 value
    printf '| %s |' "$text"
    for label in "${LABELS[@]}"; do
        value=${V[$label.$metric]:-n/a}
        printf ' %s |' "$(commas "$value")"
    done
    echo
}

table() {
    local label
    printf '| bytes |'
    for label in "${LABELS[@]}"; do printf ' %s |' "${TITLE[$label]}"; done
    echo
    printf '|---|'
    for label in "${LABELS[@]}"; do printf -- '---:|'; done
    echo
    row "Stack code (text + data)" stack_code
    row "Stack RAM (\`sizeof(DefaultStack)\`)" stack_ram
    row "OS layer code (text + data)" os_code
    row "OS layer RAM (data + bss)" os_ram
    row "C++ runtime (libstdc++) linked in" cxx
    row "Minimal program, static link" prog
    row "Empty program, same link" empty
    row "Net cost of the stack" net
}

details() {
    local entry label out
    for entry in "${TOOLCHAINS[@]}"; do
        IFS='|' read -r label _ <<<"$entry"
        out=$OUTROOT/$label
        [ -f "$out/min_app.map" ] || continue
        echo
        echo "== ${TITLE[$label]}: where the minimal program's linked bytes come from"
        "$HERE/attribute.sh" "$out/min_app.map"
        echo "  full test_app, static link:                 $(commas "${V[$label.test_app]}") bytes"
        echo "  for reference, an iostream hello world:     $(commas "${V[$label.hello]}") bytes"
    done
}

update_readme() { # update_readme <file> <table file>
    local file=$1 tablefile=$2 tmp
    if ! grep -q '<!-- size-table:begin -->' "$file" || ! grep -q '<!-- size-table:end -->' "$file"; then
        echo "measure.sh: size-table markers not found in $file" >&2
        exit 1
    fi
    tmp=$(mktemp)
    awk -v tablefile="$tablefile" '
        /<!-- size-table:begin -->/ { print; while ((getline line < tablefile) > 0) print line; skip = 1; next }
        /<!-- size-table:end -->/ { skip = 0 }
        !skip { print }' "$file" >"$tmp"
    cat "$tmp" >"$file"
    rm -f "$tmp"
    echo "measure.sh: updated $file" >&2
}

for entry in "${TOOLCHAINS[@]}"; do
    IFS='|' read -r label title cxx size strip extra package <<<"$entry"
    measure "$label" "$title" "$cxx" "$size" "$strip" "$extra" "$package"
done

tablefile=$(mktemp)
trap 'rm -f "$tablefile"' EXIT
table >"$tablefile"
if [ -n "$README" ]; then
    update_readme "$README" "$tablefile"
else
    cat "$tablefile"
fi
if [ "$DETAILS" = 1 ]; then
    details
fi
