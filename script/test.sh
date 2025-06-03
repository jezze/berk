#!/bin/bash

WD="$(pwd)"
BERK="${WD}/berk"

pass() {
    echo "PASSED: ${1}"
}

fail() {
    echo "FAILED: ${1}"
}

check() {
    cmd="${1}"
    expected_rc="${2}"
    expected_result="${3}"
    result="$($cmd)"
    rc="${?}"

    if test "${rc}" == "${expected_rc}"
    then
        if test -n "${expected_result}"
        then
            echo "${result}" | grep "${expected_result}"

            rc="${?}"
        fi
    fi

    test "${rc}" == "${expected_rc}" && pass "${cmd}" || fail "${cmd}"
}

rm -rf ${WD}/tmp
mkdir ${WD}/tmp
cd ${WD}/tmp

check "${BERK}"                             0
check "${BERK} log"                         1
check "${BERK} init"                        0
check "${BERK} log"                         1
check "${BERK} add test test.com"           0
check "${BERK} config test password xxx"    0

cd ${WD}
rm -rf ${WD}/tmp
