#!/bin/sh
set -efu

if ! [ -x rep ]; then
	echo 'rep executable not found' >&2
	exit 1
fi

touch file_a.txt file_b.txt
trap 'rm file_a.txt file_b.txt' EXIT

tests=''

tests="$tests test_file"
test_file() {
	echo 'hello world' > file_a.txt
	local actual="$(rep file_a.txt)"
	local expected="$(for i in $(seq 10); do echo 'hello world'; done)"
	[ "x$actual" = "x$expected" ]
	return $?
}

tests="$tests test_multiple_files"
test_multiple_files() {
	echo 'hello world' > file_a.txt
	echo 'goodbye world' > file_b.txt
	local actual="$(rep file_a.txt file_b.txt)"
	local expected="$(for i in $(seq 10); do echo 'hello world'; echo 'goodbye world'; done)"
	[ "x$actual" = "x$expected" ]
	return $?
}

tests="$tests test_stdin"
test_stdin() {
	local actual="$(echo 'hello world' | rep)"
	local expected="$(for i in $(seq 10); do echo 'hello world'; done)"
	[ "x$actual" = "x$expected" ]
	return $?
}

tests="$tests test_stdin_with_files"
test_stdin_with_files() {
	echo 'hello world' > file_a.txt
	echo 'goodbye world' > file_b.txt
	local actual="$(echo 'good day world' | rep file_a.txt - file_b.txt)"
	local expected="$(for i in $(seq 10); do echo 'hello world'; echo 'good day world'; echo 'goodbye world'; done)"
	[ "x$actual" = "x$expected" ]
	return $?
}

tests="$tests test_large_stdin"
test_large_stdin() {
	local actual="$(yes 'hello world' | head -n 1000 | rep)"
	local expected="$(for i in $(seq 10); do yes 'hello world' | head -n 1000; done)"
	[ "x$actual" = "x$expected" ]
	return $?
}

tests="$tests test_nflag"
test_nflag() {
	echo 'hello world' > file_a.txt
	local actual="$(rep -n 20 file_a.txt)"
	local expected="$(for i in $(seq 20); do echo 'hello world'; done)"
	[ "x$actual" = "x$expected" ]
	return $?
}

n=0
fails=0
for t in $tests; do
	: $((n += 1))
	if ! eval "$t"; then
		echo "$t failed"
		: $((fails += 1))
	fi
done
echo "$fails/$n tests failed"
