#!/bin/bash -e

# Check arguments
if [ $# -lt 3 ]; then
	echo "error: Expected 3 arguments, got $#" >&2
	exit 1
fi

# Global variables
NUM_PROCESSES=$1
NUM_BYTES=$2
FILENAME=$3
ZEUGL="$(dirname "$0")"/../cli/zeugl
PIDS=()

# Initialize test file to correct size
head -c "$NUM_BYTES" /dev/urandom >"$FILENAME"

function child_task {
	# Check that the file size is correct before running zeugl
	num_bytes=$(stat -c %s "$FILENAME")
	if [ "$num_bytes" -ne "$NUM_BYTES" ]; then
		echo "error: Expected $NUM_BYTES, got $num_bytes" >&2
		exit 1
	fi

	# Atomically write NUM_BYTES to FILENAME using zeugl
	head -c "$NUM_BYTES" /dev/urandom | "$ZEUGL" "$FILENAME"

	# Check that the file size is correct after running zeugl
	num_bytes=$(stat -c %s "$FILENAME")
	if [ "$num_bytes" -ne "$NUM_BYTES" ]; then
		echo "error: Expected $NUM_BYTES, got $num_bytes" >&2
		exit 1
	fi
}

# Spawn multiple processes running zeugl
for _ in $(seq 0 "$NUM_PROCESSES"); do
	child_task & # Run child process in background
	echo "Spawned child process $!"
	PIDS+=($!) # Store child PID
done

# Wait for child processes to finish
for pid in "${PIDS[@]}"; do
	wait "$pid"
	echo "Child process $pid has finished"
done

# Check that the file size is correct after all child processes are finished
num_bytes=$(stat -c %s "$FILENAME")
if [ "$num_bytes" -ne "$NUM_BYTES" ]; then
	echo "error: Expected $NUM_BYTES, got $num_bytes" >&2
	exit 1
fi
