if [ -z "$DISABLE_VALGRIND" ]; then
	VALGRIND=valgrind
	VALGRIND_OPTS="-q --error-exitcode=1 --leak-check=full --show-reachable=yes"
fi
