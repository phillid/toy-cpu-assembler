#!/bin/bash -e

#
# Script for running all of the automated which will go from source to binary,
# to source to binary, and compare the two binaries.
# These tests won't find symmetrical errors (e.g. incorrectly translating 'add'
# into the wrong opcode, and that same opcode back to 'add'), but provide some
# level of confidence that the disassembler and assembler at least have parity.
# Checking lower level things is the job of other tests.
#

fail() {
	echo -e '[\e[1;31mFAIL\e[0m] '"$1:" "$2"
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
export ASM="$PWD/../../assembler"
export DISASM="$PWD/../../disassembler"
has_failure=0

for first_stage_asm in *.asm ; do
	first_stage_bin="$WORK/first_stage.bin"
	second_stage_asm="$WORK/second_stage.asm"
	second_stage_bin="$WORK/second_stage.bin"

	# Assemble test code
	if ! "$ASM" "$first_stage_asm" "$first_stage_bin" ; then
		fail "$first_stage_asm" "first stage assembly failed"
		continue
	fi

	# Disassemble test code and re-assemble that disassembly
	if ! "$DISASM" "$first_stage_bin" > "$second_stage_asm" ; then
		fail "$first_stage_asm" "first stage disassembly failed"
		continue
	fi
	if ! "$ASM" "$second_stage_asm" "$second_stage_bin" ; then
		fail "$first_stage_asm" "second stage assembly failed"
		continue
	fi

	# first stage bin and second stage identical for test pass
	if diff "$first_stage_bin" "$second_stage_bin" >/dev/null; then
		pass "$first_stage_asm"
	else
		fail "$first_stage_asm" "binary mismatch"
		has_failure=1
	fi

done
popd >/dev/null

if [[ "$failure" != "0" && "$NO_CLEAN" == "1"  ]] ; then
	echo "Warning: Leaving work dir $WORK in place. Please remove this yourself"
else
	clean
fi

exit "$has_failure"
