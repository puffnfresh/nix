# shellcheck shell=bash

set -eu -o pipefail

if [[ -z "${COMMON_SH_SOURCED-}" ]]; then

COMMON_SH_SOURCED=1

functionalTestsDir="$(readlink -f "$(dirname "${BASH_SOURCE[0]-$0}")")"

source "$functionalTestsDir/common/vars.sh"
source "$functionalTestsDir/common/functions.sh"
source "$functionalTestsDir/common/init.sh"

if [[ -n "${NIX_DAEMON_PACKAGE:-}" ]]; then
    startDaemon
fi

# TODO: The list of tests we pass on Windows is much shorter than the list we don't.
# Move the skips to each file once we make more progress.
if [ "$system" = "x86_64-windows" ]; then
    echo "WINDOWS $0"
    [[ "$0" = "store-info.sh" ]] || skipTest "not ready for Windows yet"
fi

fi # COMMON_SH_SOURCED
