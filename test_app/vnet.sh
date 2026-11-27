#!/usr/bin/env bash
# Run test_app on a virtual network inside the container.
#
# A veth pair is created. dnsmasq acts as the DHCP server on veth0 and test_app
# (tinytcp) runs on veth1, so tinytcp gets its address through real DHCP and can
# be reached from this container with curl.
#
#   vnet.sh test      start everything, fetch the web page with curl, ping it, tear down.
#                     The pings (a default and an odd sized one) fail on a lost or a
#                     duplicated reply
#   vnet.sh run       start everything and keep running (Ctrl-C to stop); the web
#                     server is also forwarded to port $FORWARD_PORT (default 8080)
#   vnet.sh up|down   create / remove the virtual network only
#
# Needs CAP_NET_ADMIN in the container (see "runArgs" in devcontainer.json).
# Environment: APP (path to test_app), FORWARD_PORT
set -euo pipefail

HOST_IF=veth0
STACK_IF=veth1
SUBNET=192.168.77
HOST_ADDR=${SUBNET}.1
STATE_DIR=/tmp/tinytcp-vnet
LOG=$STATE_DIR/test_app.log
FORWARD_PORT=${FORWARD_PORT:-8080}
APP=${APP:-$(cd "$(dirname "$0")/.." && pwd)/build/test_app/test_app}

SUDO=""
if [ "$(id -u)" -ne 0 ]; then
    SUDO=sudo
fi

up() {
    mkdir -p "$STATE_DIR"
    if ! $SUDO ip link show "$HOST_IF" >/dev/null 2>&1; then
        $SUDO ip link add "$HOST_IF" type veth peer name "$STACK_IF"
    fi
    # The kernel leaves TCP/IP checksums unfilled on veth (offload) but tinytcp
    # verifies them, so offloads must be off on both ends.
    for ifc in "$HOST_IF" "$STACK_IF"; do
        $SUDO ethtool -K "$ifc" tx off tso off gso off gro off >/dev/null
    done
    $SUDO ip addr replace "${HOST_ADDR}/24" dev "$HOST_IF"
    $SUDO ip link set "$HOST_IF" up
    $SUDO ip link set "$STACK_IF" up

    if [ ! -f "$STATE_DIR/dnsmasq.pid" ] || ! kill -0 "$(cat "$STATE_DIR/dnsmasq.pid")" 2>/dev/null; then
        $SUDO dnsmasq \
            --interface="$HOST_IF" --bind-interfaces --except-interface=lo \
            --port=0 --no-resolv --no-hosts \
            --dhcp-range=${SUBNET}.100,${SUBNET}.150,12h \
            --dhcp-authoritative \
            --dhcp-leasefile="$STATE_DIR/dnsmasq.leases" \
            --pid-file="$STATE_DIR/dnsmasq.pid" \
            --log-dhcp --log-facility="$STATE_DIR/dnsmasq.log"
    fi
}

down() {
    $SUDO pkill -x test_app 2>/dev/null || true
    pkill -f "socat TCP-LISTEN:${FORWARD_PORT}," 2>/dev/null || true
    if [ -f "$STATE_DIR/dnsmasq.pid" ]; then
        $SUDO kill "$(cat "$STATE_DIR/dnsmasq.pid")" 2>/dev/null || true
        $SUDO rm -f "$STATE_DIR/dnsmasq.pid"
    fi
    $SUDO ip link del "$HOST_IF" 2>/dev/null || true
}

# Starts test_app and sets TT_ADDR to the address it got from DHCP
start_app() {
    if [ ! -x "$APP" ]; then
        echo "vnet: $APP not found; build first (cmake + make)" >&2
        exit 1
    fi
    : >"$LOG"
    # setsid detaches from the controlling terminal. Otherwise sudo's use_pty puts the
    # terminal in raw mode for as long as it runs, and since we exit without waiting
    # for sudo the terminal is never restored (no echo, staircase output).
    setsid $SUDO env TINYTCP_IFACE="$STACK_IF" stdbuf -oL "$APP" >"$LOG" 2>&1 </dev/null &
    TT_ADDR=""
    for _ in $(seq 1 30); do
        TT_ADDR=$(sed -n 's/.*DHCP got address \([0-9.]*\).*/\1/p' "$LOG" | tail -n 1)
        [ -n "$TT_ADDR" ] && break
        sleep 1
    done
    if [ -z "$TT_ADDR" ]; then
        echo "vnet: test_app did not get a DHCP address; log:" >&2
        cat "$LOG" >&2
        return 1
    fi
    echo "vnet: test_app has address $TT_ADDR"
}

trap_cleanup() {
    trap - EXIT INT TERM
    down
}

case "${1:-}" in
up)
    up
    ;;
down)
    down
    ;;
test)
    trap trap_cleanup EXIT INT TERM
    up
    start_app
    curl --silent --show-error --fail --max-time 10 "http://${TT_ADDR}/" -o "$STATE_DIR/index.html"
    echo "vnet: PASS - fetched $(wc -c <"$STATE_DIR/index.html") bytes from http://${TT_ADDR}/"
    # Ping with a payload of the default size and of an odd size. Every request must get exactly one
    # reply: a lost reply is a failure, and so is a duplicated one, which means the stack is
    # handling a frame more than once.
    if command -v ping >/dev/null 2>&1; then
        for size in 56 1; do
            if ! output=$(ping -c 3 -W 2 -s "$size" "$TT_ADDR" 2>&1) || ! grep -q " 3 received," <<<"$output" ||
                grep -q "DUP!" <<<"$output"; then
                echo "vnet: FAIL - ping -s $size $TT_ADDR:" >&2
                echo "$output" >&2
                exit 1
            fi
        done
        echo "vnet: PASS - pinged $TT_ADDR with 56 and 1 octets of data, one reply to every request"
    else
        echo "vnet: ping not installed, skipping the ping checks" >&2
    fi
    ;;
run)
    trap trap_cleanup EXIT INT TERM
    up
    start_app
    socat TCP-LISTEN:"${FORWARD_PORT}",fork,reuseaddr TCP:"${TT_ADDR}":80 &
    echo "vnet: http://${TT_ADDR}/ inside the container, http://localhost:${FORWARD_PORT}/ via port forward"
    echo "vnet: Ctrl-C to stop"
    wait
    ;;
*)
    sed -n '2,15p' "$0" | sed 's/^# \{0,1\}//'
    exit 1
    ;;
esac
