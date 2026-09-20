#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TEST_DIR=$(mktemp -d)
trap 'rm -rf "$TEST_DIR"' EXIT

mkdir -p "$TEST_DIR/build"
cat > "$TEST_DIR/build/.config" << 'CONF'
CONFIG_NET=y
CONFIG_NET_TCP=y
CONFIG_NET_UDP=y
CONFIG_NET_BLUETOOTH=n
CONFIG_FS_ROMFS=y
CONFIG_BOARD_LOOPSPERMSEC=160000
CONF

mkdir -p "$TEST_DIR/nuttx/net"
cat > "$TEST_DIR/nuttx/net/Kconfig" << 'KCONF'
config NET
	bool "Networking support"
	default y

config NET_TCP
	bool "TCP/IP support"
	depends on NET
	default y
	select NET_TCP_WRITE_BUFFERS
	help
	  Enable TCP/IP protocol support.

config NET_UDP
	bool "UDP support"
	depends on NET

config NET_BLUETOOTH
	bool "Bluetooth L2CAP networking"
	depends on NET
	help
	  Bluetooth L2CAP networking support.

config NET_ICMP
	bool "ICMP support"
	depends on NET
	help
	  ICMP echo (ping) support.
KCONF

echo "==== Test 1: Exact (enabled) ===="
python3 "$SCRIPT_DIR/vela_config_query.py" -q CONFIG_NET_TCP -w "$TEST_DIR" -b "$TEST_DIR/build"

echo ""
echo "==== Test 2: Exact (disabled) ===="
python3 "$SCRIPT_DIR/vela_config_query.py" -q CONFIG_NET_BLUETOOTH -w "$TEST_DIR" -b "$TEST_DIR/build"

echo ""
echo "==== Test 3: Kconfig only ===="
python3 "$SCRIPT_DIR/vela_config_query.py" -q CONFIG_NET_ICMP -w "$TEST_DIR" -b "$TEST_DIR/build"

echo ""
echo "==== Test 4: Fuzzy ===="
python3 "$SCRIPT_DIR/vela_config_query.py" -q NET -w "$TEST_DIR" -b "$TEST_DIR/build" -n 5

echo ""
echo "==== Test 5: Dispatch ===="
python3 "$SCRIPT_DIR/dispatch.py" "CONFIG_NET_TCP depends on what" "$TEST_DIR"

echo ""
echo "All tests passed!"
