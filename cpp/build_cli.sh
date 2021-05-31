#!/bin/bash
# https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html
readonly ROOT="$(dirname "${BASH_SOURCE[0]}")"
# readonly ROOT="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

# if [[ "$@" =~ '--clang' ]]
# then
# 	readonly COMPILER='clang++ -target x86_64-w64-mingw64 -fcolor-diagnostics -fansi-escape-codes -ferror-limit=5'
# else
	# Use g++ by default.
	readonly COMPILER='g++ -std=c++20 -pthread -fdiagnostics-color=always -fmax-errors=5'
# fi

# readonly CPP=$(find src -type f -name '*.cpp')
readonly CPP="$(echo "$ROOT/src/"{\
main_cli,\
cli/repl,\
lib/gen/batch,\
lib/gen/mod,\
lib/gen/path,\
lib/print\
}.cpp)"

${COMPILER}\
	-iquote "${ROOT}/include"\
	-Wall -Werror -Wpedantic -Wimplicit-fallthrough=5 -O3\
	"$@"\
	${CPP} -o solvent.exe\
