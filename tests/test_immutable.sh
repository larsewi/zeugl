#!/bin/bash
set -e

# Test immutable bit handling across platforms
# Usage: test_immutable.sh
# WARNING: This test is not safe and requires you to install zeugl CLI tool

TESTFILE="$(mktemp "immutable.XXXXXX")"

case "$(uname -s)" in
Linux*)
	function set_immutable {
		chattr +i "$TESTFILE"
	}
	function clear_immutable {
		chattr -i "$TESTFILE"
	}
	function is_immutable {
		lsattr "$TESTFILE" | grep -q '^----i'
	}
	;;
FreeBSD* | OpenBSD* | NetBSD* | Darwin*)
	function set_immutable {
		chflags uchg "$TESTFILE"
	}
	function clear_immutable {
		chflags nouchg "$TESTFILE"
	}
	function is_immutable {
		# shellcheck disable=SC2010
		ls -lO "$TESTFILE" | grep -q 'uchg'
	}
	;;
*)
	echo "Unsupported platform $(uname -s)"
	exit 0
	;;
esac

# Set trap to remove immutable bit
function cleanup {
	if [ -f "$TESTFILE" ]; then
		clear_immutable
	fi
	rm -f "$TESTFILE"
}
trap cleanup EXIT

# Test 1: Basic immutable handling
echo "Test 1: Basic immutable handling"
echo "original" >"$TESTFILE"
set_immutable
echo "modified" | zeugl -di "$TESTFILE" | grep -E "immutable_(chflags|ioctl).c"
content=$(cat "$TESTFILE")
if [ "$content" != "modified" ]; then
	echo "ERROR: Expected content 'modified', got '$content' for file '$TESTFILE'"
	exit 1
fi
if ! is_immutable; then
	echo "ERROR: File '$TESTFILE' is not immutable"
	exit 1
fi
clear_immutable
rm "$TESTFILE"

# Test 2: Without Z_IMMUTABLE flag (should fail)
echo "Test 2: Without Z_IMMUTABLE flag"
echo "original" >"$TESTFILE"
set_immutable
if echo "modified" | zeugl -d "$TESTFILE"; then
	echo "ERROR: Should have failed without -i flag"
	exit 1
fi
clear_immutable
rm "$TESTFILE"

# Test 3: Concurrent access with immutable
# TODO: Make immutable bit flipping thread safe

# echo "Test 3: Concurrent access"
# echo "original" >"$TESTFILE"
# set_immutable
#
# # Launch multiple concurrent writes
# for i in {1..5}; do
# 	echo "process$i" | zeugl -di "$TESTFILE" &
# done
# wait
#
# # Verify file is still immutable and has content
# if ! is_immutable; then
# 	echo "ERROR: File '$TESTFILE' is not immutable"
# 	exit 1
# fi
# if ! [ -s "$TESTFILE" ]; then
# 	echo "ERROR: Expected content for file '$TESTFILE', found '$(cat "$TESTFILE")'"
# fi
# clear_immutable
# rm "$TESTFILE"
#
# echo "All tests passed!"
