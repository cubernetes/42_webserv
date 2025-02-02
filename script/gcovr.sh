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

1>/dev/null 2>/dev/null command -v gcovr || die "gcovr is not installed, install gcovr, or look at https://command-not-found.com/gcovr, or run this command:$(printf '\n\n\t')pip install gcovr (might need to add --break-system-packages)"

GCOVR_OUT_DIR="gcovr_report"
[ "$(printf %s "${GCOVR_OUT_DIR}" | tr -d '/')" = "${GCOVR_OUT_DIR}" ] || die "GCOVR_OUT_DIR (${GCOVR_OUT_DIR}) variable is probably unsafe because it contains a slash"
[ -n "${GCOVR_OUT_DIR}" ] || die "GCOVR_OUT_DIR variable is empty"
[ -e "${GCOVR_OUT_DIR}" ] && ([ -d "${GCOVR_OUT_DIR}" ] || die "GCOVR_OUT_DIR (${GCOVR_OUT_DIR}) exists and is not a directory") 

export DEBUG=1
export _CXX='g++'
export CFLAGS='-fprofile-arcs -ftest-coverage'
export LDFLAGS='-fprofile-arcs -ftest-coverage'

rm -rf "./${GCOVR_OUT_DIR}" || die "Could not remove GCOVR_OUT_DIR (${GCOVR_OUT_DIR})"
mkdir -p "${GCOVR_OUT_DIR}" || die "Could not create GCOVR_OUT_DIR (${GCOVR_OUT_DIR})"
make -sj unit_tests || die "Build failed"
gcovr --root . --html --html-details --output "${GCOVR_OUT_DIR}/index.html" || die "Failed to generate gcovr html"

printf "Run 'python3 -m http.server 8888 -d \"${GCOVR_OUT_DIR}\"' (y/N): "
read -r choice
[ "${choice}" = "y" ] || [ "${choice}" = "yes" ] || die "Choice was not y, exiting"

if 1>/dev/null 2>/dev/null command -v python3; then
	python3 -m http.server 8888 -d "${GCOVR_OUT_DIR}"
else
	die "python3 binary not found, can't start server"
fi
