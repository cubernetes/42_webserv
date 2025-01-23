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

1>/dev/null 2>/dev/null command -v lcov || die "lcov is not installed, install lcov, or look at https://command-not-found.com/genhtml"
1>/dev/null 2>/dev/null command -v genhtml || die "genhtml is not installed, install lcov, or look at https://command-not-found.com/genhtml"

LCOV_OUT_DIR="lcov_report"
[ "$(printf %s "${LCOV_OUT_DIR}" | tr -d '/')" = "${LCOV_OUT_DIR}" ] || die "LCOV_OUT_DIR (${LCOV_OUT_DIR}) variable is probably unsafe because it contains a slash"
[ -n "${LCOV_OUT_DIR}" ] || die "LCOV_OUT_DIR variable is empty"
[ -e "${LCOV_OUT_DIR}" ] && ([ -d "${LCOV_OUT_DIR}" ] || die "LCOV_OUT_DIR (${LCOV_OUT_DIR}) exists and is not a directory")

export DEBUG=1
export CXX='g++'
export CFLAGS='-fprofile-arcs -ftest-coverage'
export LDFLAGS='-fprofile-arcs -ftest-coverage'

rm -rf "./${LCOV_OUT_DIR}" || die "Could not remove LCOV_OUT_DIR (${LCOV_OUT_DIR})"
mkdir -p "${LCOV_OUT_DIR}" || die "Could not create LCOV_OUT_DIR (${LCOV_OUT_DIR})"
make -sj unit_test || die "Build failed"
lcov --quiet --no-external --demangle-cpp --capture --base-directory . --directory obj_c2 --output-file "${LCOV_OUT_DIR}/coverage.info" || die "Failed to generate lcov report"
genhtml --quiet "${LCOV_OUT_DIR}/coverage.info" --output-directory "${LCOV_OUT_DIR}" || die "Failed to generate html from lcov report"

printf "Run 'python3 -m http.server 9999 -d \"${LCOV_OUT_DIR}\"' (y/N): "
read -r choice
[ "${choice}" = "y" ] || [ "${choice}" = "yes" ] || die "Choice was not y, exiting"

if 1>/dev/null 2>/dev/null command -v python3; then
	python3 -m http.server 9999 -d "${LCOV_OUT_DIR}"
else
	die "python3 binary not found, can't start server"
fi
