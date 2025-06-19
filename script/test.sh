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

echo "Basic"
check "${BERK}"                             0
check "${BERK} log"                         1
check "${BERK} init"                        0
check "${BERK} log"                         1
echo "Add"
check "${BERK} add -t local test"           0
check "${BERK} config test"                 0
echo "Synchronous execution"
check "${BERK} exec -n test 'uptime'"       0
check "${BERK} log"                         0
check "${BERK} log HEAD"                    0
check "${BERK} log HEAD 0"                  0
echo "Aynchronous execution"
check "${BERK} exec test 'uptime'"          0
sleep 1
check "${BERK} log HEAD"                    0
check "${BERK} log HEAD 0"                  0

cd ${WD}
rm -rf ${WD}/tmp
