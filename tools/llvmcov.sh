#!/bin/sh

set -e
set -u

die () {
	printf "\033\13331m%s\033\133\n" "${0}: ${1:-Exiting for unknown reason}" 1>&2
	exit 1
}

warn () {
	printf "\033\13333m%s\033\133\n" "${0}: ${1:-Warning for unknown reason}" 1>&2
}

top_level="$(git rev-parse --show-toplevel)" || die "Failed to obtain root of git repository"
cd -- "${top_level}" || die "Failed to change directory to '${top_level}'"

1>/dev/null 2>/dev/null command -v llvm-profdata || die "llvm-profdata is not installed, install llvm, or look at https://command-not-found.com/llvm-profdata"
1>/dev/null 2>/dev/null command -v llvm-cov || die "llvm-cov is not installed, install llvm, or look at https://command-not-found.com/llvm-cov"

LLVM_PROFDATA_DIR="llvm_profdata"
[ "$(printf %s "${LLVM_PROFDATA_DIR}" | tr -d '/')" = "${LLVM_PROFDATA_DIR}" ] || die "LLVM_PROFDATA_DIR (${LLVM_PROFDATA_DIR}) variable is probably unsafe because it contains a slash"
[ -n "${LLVM_PROFDATA_DIR}" ] || die "LLVM_PROFDATA_DIR variable is empty"
[ -e "${LLVM_PROFDATA_DIR}" ] && ([ -d "${LLVM_PROFDATA_DIR}" ] || die "LLVM_PROFDATA_DIR (${LLVM_PROFDATA_DIR}) exists and is not a directory")

export DEBUG=1
export _CXX='clang++'
export CFLAGS='-fprofile-instr-generate -fcoverage-mapping'
export LDFLAGS='-fprofile-instr-generate'
export LLVM_PROFILE_FILE="${LLVM_PROFDATA_DIR}/code-%p.profraw"

rm -rf "./${LLVM_PROFDATA_DIR}" || die "Could not remove LLVM_PROFDATA_DIR (${LLVM_PROFDATA_DIR})"
mkdir -p "${LLVM_PROFDATA_DIR}" || die "Could not create LLVM_PROFDATA_DIR (${LLVM_PROFDATA_DIR})"
make -sj unit_tests || die "Build failed"
# shellcheck disable=SC2046
llvm-profdata merge -output="${LLVM_PROFDATA_DIR}/code.profdata" $(find "${LLVM_PROFDATA_DIR}" -mindepth 1 -maxdepth 1 -type f -name 'code-*.profraw') || die "Could not merge prof data"
llvm-cov report -ignore-filename-regex='Catch2/src/catch2/' ./c2_unit_tests -instr-profile="${LLVM_PROFDATA_DIR}/code.profdata" -use-color || die "Could not generate coverage report"

printf "Show entire coverage data in less pager (y/N): "
read -r choice
[ "${choice}" = "y" ] || [ "${choice}" = "yes" ] || die "Choice was not y, exiting"

if 1>/dev/null 2>/dev/null command -v less; then
	llvm-cov show ./c2_unit_tests -instr-profile="${LLVM_PROFDATA_DIR}/code.profdata" src/*.cpp -use-color | less -SR
else
	warn "Can't show report in less pager, dumping to terminal"
	sleep .5
	llvm-cov show ./c2_unit_tests -instr-profile="${LLVM_PROFDATA_DIR}/code.profdata" src/*.cpp -use-color
fi

# use lcov.sh or gcovr.sh for html output, it's just better imho
# llvm-cov show ./c2_unit_tests -instr-profile="${LLVM_PROFDATA_DIR}/code.profdata" src/*.cpp -use-color --format html > "${LLVM_PROFDATA_DIR}/index.html"
