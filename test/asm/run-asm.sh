#!/bin/bash -e

#
# Script for running all of the automated which will go from source to binary.
#

fail() {
	echo -e '[\e[1;31mFAIL\e[0m] '"$1:" "$2"
	has_failure=1
}

pass() {
	echo -e '[\e[1;32mPASS\e[0m] '"$1"
}

clean() {
	echo "Removing work dir $WORK"
	rm -r "$WORK"
}

if [ "$1" == "noclean" ]; then
	NO_CLEAN=1
else
	NO_CLEAN=0
fi
WORK=$(mktemp -d)
pushd $(dirname "$0") >/dev/null
source ../valgrind.sh
export ASM="$PWD/../../assembler"
export DISASM="$PWD/../../disassembler"
has_failure=0

test_should_fail() {
	local t="$1"
	local xc="$2"
	if (( xc > 0 && xc < 128 )); then
		pass "$t" "assembly xfailed"
	elif (( xc == 0 )); then
		fail "$t" "assembly didn't fail as expected"
	else
		fail "$t" "assembler was sent signal $(( xc - 128 ))"
	fi
}

test_should_pass() {
	local t="$1"
	local xc="$2"
	if (( xc == 0 )); then
		pass "$t"
	else
		fail "$t" "assembly failed"
	fi
}

echo "Should pass:"
for first_stage_asm in should-pass/*.asm ; do
	t=$(basename "$first_stage_asm")
	first_stage_bin="$WORK/${t}-first_stage.bin"
	log="$WORK/${t}.log"

	# Assemble test code
	set +e
	$VALGRIND $VALGRIND_OPTS "$ASM" "$first_stage_asm" "$first_stage_bin" 2>"$log"
	xc="$?"
	set -e
	test_should_pass "$t" "$xc"
done

echo "Should fail (pass means asm failed as expected):"
for first_stage_asm in should-fail/*.asm ; do
	t=$(basename "$first_stage_asm")
	first_stage_bin="$WORK/${t}-first_stage.bin"
	log="$WORK/${t}.log"

	# Assemble test code
	set +e
	$VALGRIND $VALGRIND_OPTS "$ASM" "$first_stage_asm" "$first_stage_bin" 2>"$log"
	xc="$?"
	set -e
	test_should_fail "$t" "$xc"
done
popd >/dev/null

if [[ "$failure" != "0" && "$NO_CLEAN" == "1"  ]] ; then
	echo "Warning: Leaving work dir $WORK in place. Please remove this yourself"
else
	clean
fi

exit "$has_failure"
