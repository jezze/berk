#!/bin/bash

set -e

WD="$(pwd)"
BERK="${WD}/berk"
VERSION="0.0.1"
REMOTE1="myhost1"
REMOTE2="myhost2"
TESTDATA="testdata123testdata456"

rm -rf ${WD}/testdir
mkdir ${WD}/testdir
cd ${WD}/testdir

echo -n "Uninitialized... "
${BERK} | grep -q "Usage: ${BERK}"
${BERK} version | grep -q "berk version ${VERSION}"
for cmd in add config exec list log remove send shell stop wait
do
    ${BERK} ${cmd} 2>&1 | grep -q "berk: Could not find '.berk' directory."
done
echo "OK"

echo -n "Initialized... "
${BERK} init | grep -q "Initialized berk in '.berk'."
${BERK} log 2>&1 | grep -q "berk: Unable to open log."
echo "OK"

echo -n "Add remotes... "
${BERK} add -t "local" "${REMOTE1}" | grep -q "Remote '${REMOTE1}' added."
${BERK} add -t "local" "${REMOTE2}" | grep -q "Remote '${REMOTE2}' added."
${BERK} list | wc -l | grep -q "2"
${BERK} list | grep -q "${REMOTE1}"
${BERK} list | grep -q "${REMOTE2}"
echo "OK"

echo -n "Config (single host)... "
${BERK} config "${REMOTE1}" | wc -l | grep -q "2"
${BERK} config "${REMOTE1}" | grep -q "name=${REMOTE1}"
${BERK} config "${REMOTE1}" | grep -q "type=local"
echo "OK"

echo -n "Config (multiple hosts)... "
${BERK} config "${REMOTE1} ${REMOTE2}" | wc -l | grep -q "4"
${BERK} config "${REMOTE1} ${REMOTE2}" | grep -q "name=${REMOTE1}"
${BERK} config "${REMOTE1} ${REMOTE2}" | grep -q "name=${REMOTE2}"
echo "OK"

echo -n "Config (tags)... "
${BERK} config "${REMOTE1}" tags "a c"
${BERK} config "${REMOTE2}" tags "b c"
${BERK} list -t "a" | wc -l | grep -q "1"
${BERK} list -t "b" | wc -l | grep -q "1"
${BERK} list -t "c" | wc -l | grep -q "2"
${BERK} list -t "a" | grep -q "${REMOTE1}"
${BERK} list -t "b" | grep -q "${REMOTE2}"
${BERK} list -t "c" | grep -q "${REMOTE1}"
${BERK} list -t "c" | grep -q "${REMOTE2}"
echo "OK"

echo -n "Send (single host, single file)... "
echo "${TESTDATA}" > file1.txt
${BERK} send "${REMOTE1}" "file1.txt" "file2.txt" > /dev/null
cat file2.txt | grep -q "${TESTDATA}"
echo "OK"

echo -n "Exec (synchronous, single host, status passed)... "
${BERK} exec -n "${REMOTE1}" "echo ${TESTDATA}" > /dev/null
${BERK} log HEAD | wc -l | grep -q "5"
${BERK} log HEAD | grep -q "id=.* datetime=.*"
${BERK} log HEAD | grep -q "total=1 complete=1 aborted=0 passed=1 failed=0"
${BERK} log HEAD | grep -q "    run=0 remote=${REMOTE1} status=passed"
${BERK} log HEAD 0 | grep -q "${TESTDATA}"
echo "OK"

echo -n "Exec (synchronous, multiple hosts, status passed)... "
${BERK} exec -n "${REMOTE1} ${REMOTE2}" "echo ${TESTDATA}" > /dev/null
${BERK} log HEAD | wc -l | grep -q "6"
${BERK} log HEAD | grep -q "id=.* datetime=.*"
${BERK} log HEAD | grep -q "total=2 complete=2 aborted=0 passed=2 failed=0"
${BERK} log HEAD | grep -q "    run=0 remote=${REMOTE1} status=passed"
${BERK} log HEAD | grep -q "    run=1 remote=${REMOTE2} status=passed"
${BERK} log HEAD 0 | grep -q "${TESTDATA}"
${BERK} log HEAD 1 | grep -q "${TESTDATA}"
echo "OK"

echo -n "Exec (synchronous, single host, status failed)... "
${BERK} exec -n "${REMOTE1}" "incorrectcommand" > /dev/null
${BERK} log HEAD | wc -l | grep -q "5"
${BERK} log HEAD | grep -q "id=.* datetime=.*"
${BERK} log HEAD | grep -q "total=1 complete=1 aborted=0 passed=0 failed=1"
${BERK} log HEAD | grep -q "    run=0 remote=${REMOTE1} status=failed"
${BERK} log -e HEAD 0 | grep -q "incorrectcommand"
echo "OK"

echo -n "Exec (synchronous, multiple hosts, status failed)... "
${BERK} exec -n "${REMOTE1} ${REMOTE2}" "incorrectcommand" > /dev/null
${BERK} log HEAD | wc -l | grep -q "6"
${BERK} log HEAD | grep -q "id=.* datetime=.*"
${BERK} log HEAD | grep -q "total=2 complete=2 aborted=0 passed=0 failed=2"
${BERK} log HEAD | grep -q "    run=0 remote=${REMOTE1} status=failed"
${BERK} log HEAD | grep -q "    run=1 remote=${REMOTE2} status=failed"
${BERK} log -e HEAD 0 | grep -q "incorrectcommand"
${BERK} log -e HEAD 1 | grep -q "incorrectcommand"
echo "OK"

echo -n "Exec (asynchronous, single host, status passed)... "
${BERK} exec "${REMOTE1}" "echo ${TESTDATA}" > /dev/null
${BERK} wait HEAD
${BERK} log HEAD | wc -l | grep -q "5"
${BERK} log HEAD | grep -q "id=.* datetime=.*"
${BERK} log HEAD | grep -q "total=1 complete=1 aborted=0 passed=1 failed=0"
${BERK} log HEAD | grep -q "    run=0 remote=${REMOTE1} status=passed"
${BERK} log HEAD 0 | grep -q "${TESTDATA}"
echo "OK"

echo -n "Exec (asynchronous, multiple hosts, status passed)... "
${BERK} exec "${REMOTE1} ${REMOTE2}" "echo ${TESTDATA}" > /dev/null
${BERK} wait HEAD
${BERK} log HEAD | wc -l | grep -q "6"
${BERK} log HEAD | grep -q "id=.* datetime=.*"
${BERK} log HEAD | grep -q "total=2 complete=2 aborted=0 passed=2 failed=0"
${BERK} log HEAD | grep -q "    run=0 remote=${REMOTE1} status=passed"
${BERK} log HEAD | grep -q "    run=1 remote=${REMOTE2} status=passed"
${BERK} log HEAD 0 | grep -q "${TESTDATA}"
${BERK} log HEAD 1 | grep -q "${TESTDATA}"
echo "OK"

echo -n "Exec (asynchronous, single host, status failed)... "
${BERK} exec "${REMOTE1}" "incorrectcommand" > /dev/null
${BERK} wait HEAD
${BERK} log HEAD | wc -l | grep -q "5"
${BERK} log HEAD | grep -q "id=.* datetime=.*"
${BERK} log HEAD | grep -q "total=1 complete=1 aborted=0 passed=0 failed=1"
${BERK} log HEAD | grep -q "    run=0 remote=${REMOTE1} status=failed"
${BERK} log -e HEAD 0 | grep -q "incorrectcommand"
echo "OK"

echo -n "Exec (asynchronous, multiple hosts, status failed)... "
${BERK} exec "${REMOTE1} ${REMOTE2}" "incorrectcommand" > /dev/null
${BERK} wait HEAD
${BERK} log HEAD | wc -l | grep -q "6"
${BERK} log HEAD | grep -q "id=.* datetime=.*"
${BERK} log HEAD | grep -q "total=2 complete=2 aborted=0 passed=0 failed=2"
${BERK} log HEAD | grep -q "    run=0 remote=${REMOTE1} status=failed"
${BERK} log HEAD | grep -q "    run=1 remote=${REMOTE2} status=failed"
${BERK} log -e HEAD 0 | grep -q "incorrectcommand"
${BERK} log -e HEAD 1 | grep -q "incorrectcommand"
echo "OK"

echo -n "Exec (asynchronous, single host, abort)... "
${BERK} exec "${REMOTE1}" "sleep 10" > /dev/null
${BERK} stop HEAD
${BERK} log HEAD | wc -l | grep -q "5"
${BERK} log HEAD | grep -q "id=.* datetime=.*"
${BERK} log HEAD | grep -q "total=1 complete=1 aborted=1 passed=0 failed=0"
${BERK} log HEAD | grep -q "    run=0 remote=${REMOTE1} status=aborted"
echo "OK"

cd ${WD}
