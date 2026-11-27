#!/usr/bin/env bash
# usage: attribute.sh <linker map file>
# Attributes the bytes of the kept input sections in a linker map to where they came from.
# Written for mawk, which has no strtonum, so hex values are parsed by hand.
awk '
function hex(s,   i, c, v, d)
{
    s = tolower(substr(s, 3)); v = 0
    for (i = 1; i <= length(s); i++) { c = substr(s, i, 1); d = index("0123456789abcdef", c) - 1; v = v * 16 + d }
    return v
}
/^Linker script and memory map/ { on = 1; next }
!on { next }
/^ \.[A-Za-z0-9_.$]+/ {
    n = split($0, f, " ")
    # long section names put the address, size and origin on the following line
    if (n == 1) { getline nxt; $0 = $0 " " nxt; n = split($0, f, " ") }
    if (n >= 4 && f[2] ~ /^0x/ && f[3] ~ /^0x/) {
        sz = hex(f[3]); src = f[4]
        if (sz == 0) next
        if (src ~ /libstdc\+\+/) k = "libstdc++"
        else if (src ~ /libc\.a/) k = "libc"
        else if (src ~ /libm|libgcc|libpthread/) k = "libgcc/libm/other libs"
        else if (src ~ /\/(os|tinytcp|packet_io|web_server|test_app|min_app)[_.]/) k = "tinytcp project code"
        else if (src ~ /\.a\(/) k = "other archives"
        else k = "crt/other"
        tot[k] += sz; all += sz
    }
}
END {
    for (k in tot) printf "  %-24s %9d\n", k, tot[k]
    printf "  %-24s %9d\n", "TOTAL (kept sections)", all
}' "$1" | sort
