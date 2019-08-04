#!/bin/bash -e

#
# Script for running all of the automated tests that involve the assembler
# and emulator.
# These tests are assembly files with postconditions for registers specified
# in special syntax within comments. They contain code to run, and the post-
# conditions are checked in order to determine the test result.
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

WORK=$(mktemp -d)
pushd $(dirname "$0") >/dev/null
source ../valgrind.sh
export ASM="$PWD/../../assembler"
export EMUL="$PWD/../../emulator"
has_failure=0

for asmfile in *.asm ; do
	binfile="$WORK/$(sed -e 's/\.asm$/.bin/' <<< "$asmfile")"
	outfile="$WORK/$(sed -e 's/\.asm$/.out/' <<< "$asmfile")"
	# Assemble test code
	if ! "$ASM" "$asmfile" "$binfile" ; then
		fail "$asmfile" "test assembly failed"
		continue
	fi

	if $VALGRIND $VALGRIND_OPTS "$EMUL" "$binfile" > "$outfile" ; then
		# Each postcondition line must hold true, and forms a separate test to
		# help track down failures
		(echo '; POST $0 = 0' ;
		 echo '; POST $H = 0xFFFF' ;
		 grep '^;\s\+POST\s\+' "$asmfile" ) | while read line ; do
			reg=$(awk -F= '{print $1}' <<< "$line" | awk '{print $(NF)}')
			val=$(awk -F= '{print $2}' <<< "$line"| awk '{print $1}')
			subtest="${asmfile}:${reg}"
			# Scrape output of emulator for register value
			actual=$(grep "$reg" "$outfile" | awk '{print $2}')
			if [[ "$actual" -eq "$val" ]]; then
				pass "$subtest"
			else
				fail "$subtest" "postcondition (expect $val, got $actual)"
				has_failure=1
			fi
		done
	else
		fail "$asmfile" "non-zero exit code"
	fi
done
popd >/dev/null

if [[ "$failure" != "0" && "$NO_CLEAN" == "1"  ]] ; then
	echo "Warning: Leaving work dir $WORK in place. Please remove this yourself"
else
	clean
fi

exit "$has_failure"
