#!/bin/bash

set -e

BERK=${1}
TESTDIR=${2}

VERSION="0.0.1"
REMOTE1="myhost1"
REMOTE2="myhost2"
TESTDATA="testdata123testdata456"

cd "${TESTDIR}"

echo -n "Uninitialized... "
${BERK} | grep -q "Usage: ${BERK}"
${BERK} version | grep -q "berk version ${VERSION}"
for cmd in config exec log remote send show shell stop wait
do
    ${BERK} ${cmd} 2>&1 | grep -q "berk: Could not find '.berk' directory."
done
echo "OK"

echo -n "Initialized... "
${BERK} init | grep -q "Initialized berk in '.berk'."
${BERK} log 2>&1 | grep -q "berk: Could not open log."
echo "OK"

echo -n "Add remotes... "
${BERK} remote add -t "local" "${REMOTE1}" | grep -q "Remote '${REMOTE1}' added."
${BERK} remote add -t "local" "${REMOTE2}" | grep -q "Remote '${REMOTE2}' added."
${BERK} remote | wc -l | grep -q "2"
${BERK} remote | grep -q "${REMOTE1}"
${BERK} remote | grep -q "${REMOTE2}"
echo "OK"

echo -n "Config (single host)... "
${BERK} config list "${REMOTE1}" | wc -l | grep -q "2"
${BERK} config list "${REMOTE1}" | grep -q "name=${REMOTE1}"
${BERK} config list "${REMOTE1}" | grep -q "type=local"
echo "OK"

echo -n "Config (multiple hosts)... "
${BERK} config list "${REMOTE1}" "${REMOTE2}" | wc -l | grep -q "4"
${BERK} config list "${REMOTE1}" "${REMOTE2}" | grep -q "name=${REMOTE1}"
${BERK} config list "${REMOTE1}" "${REMOTE2}" | grep -q "name=${REMOTE2}"
echo "OK"

echo -n "Config (tags)... "
${BERK} config set tags "a c" "${REMOTE1}"
${BERK} config set tags "b c" "${REMOTE2}"
${BERK} remote -t "a" | wc -l | grep -q "1"
${BERK} remote -t "b" | wc -l | grep -q "1"
${BERK} remote -t "c" | wc -l | grep -q "2"
${BERK} remote -t "a" | grep -q "${REMOTE1}"
${BERK} remote -t "b" | grep -q "${REMOTE2}"
${BERK} remote -t "c" | grep -q "${REMOTE1}"
${BERK} remote -t "c" | grep -q "${REMOTE2}"
echo "OK"

echo -n "Send (single host, single file)... "
echo "${TESTDATA}" > file1.txt
${BERK} send "file1.txt" "file2.txt" "${REMOTE1}" > /dev/null
cat file2.txt | grep -q "${TESTDATA}"
echo "OK"

echo -n "Exec (synchronous, single host, status passed)... "
${BERK} exec -n -c "echo ${TESTDATA}" "${REMOTE1}" > /dev/null
${BERK} show | wc -l | grep -q "5"
${BERK} show | grep -q "id=.* datetime=.*"
${BERK} show | grep -q "total=1 complete=1 aborted=0 passed=1 failed=0"
${BERK} show | grep -q "    run=0 remote=${REMOTE1} status=passed"
${BERK} show -o -r 0 | grep -q "${TESTDATA}"
echo "OK"

echo -n "Exec (synchronous, multiple hosts, status passed)... "
${BERK} exec -n -c "echo ${TESTDATA}" "${REMOTE1}" "${REMOTE2}" > /dev/null
${BERK} show | wc -l | grep -q "6"
${BERK} show | grep -q "id=.* datetime=.*"
${BERK} show | grep -q "total=2 complete=2 aborted=0 passed=2 failed=0"
${BERK} show | grep -q "    run=0 remote=${REMOTE1} status=passed"
${BERK} show | grep -q "    run=1 remote=${REMOTE2} status=passed"
${BERK} show -o -r 0 | grep -q "${TESTDATA}"
${BERK} show -o -r 1 | grep -q "${TESTDATA}"
echo "OK"

echo -n "Exec (synchronous, single host, status failed)... "
${BERK} exec -n -c "incorrectcommand" "${REMOTE1}" > /dev/null
${BERK} show | wc -l | grep -q "5"
${BERK} show | grep -q "id=.* datetime=.*"
${BERK} show | grep -q "total=1 complete=1 aborted=0 passed=0 failed=1"
${BERK} show | grep -q "    run=0 remote=${REMOTE1} status=failed"
${BERK} show -e -r 0 | grep -q "incorrectcommand"
echo "OK"

echo -n "Exec (synchronous, multiple hosts, status failed)... "
${BERK} exec -n -c "incorrectcommand" "${REMOTE1}" "${REMOTE2}" > /dev/null
${BERK} show | wc -l | grep -q "6"
${BERK} show | grep -q "id=.* datetime=.*"
${BERK} show | grep -q "total=2 complete=2 aborted=0 passed=0 failed=2"
${BERK} show | grep -q "    run=0 remote=${REMOTE1} status=failed"
${BERK} show | grep -q "    run=1 remote=${REMOTE2} status=failed"
${BERK} show -e -r 0 | grep -q "incorrectcommand"
${BERK} show -e -r 1 | grep -q "incorrectcommand"
echo "OK"

echo -n "Exec (asynchronous, single host, status passed)... "
${BERK} exec -c "echo ${TESTDATA}" "${REMOTE1}" > /dev/null
${BERK} wait
${BERK} show | wc -l | grep -q "5"
${BERK} show | grep -q "id=.* datetime=.*"
${BERK} show | grep -q "total=1 complete=1 aborted=0 passed=1 failed=0"
${BERK} show | grep -q "    run=0 remote=${REMOTE1} status=passed"
${BERK} show -o -r 0 | grep -q "${TESTDATA}"
echo "OK"

echo -n "Exec (asynchronous, multiple hosts, status passed)... "
${BERK} exec -c "echo ${TESTDATA}" "${REMOTE1}" "${REMOTE2}" > /dev/null
${BERK} wait
${BERK} show | wc -l | grep -q "6"
${BERK} show | grep -q "id=.* datetime=.*"
${BERK} show | grep -q "total=2 complete=2 aborted=0 passed=2 failed=0"
${BERK} show | grep -q "    run=0 remote=${REMOTE1} status=passed"
${BERK} show | grep -q "    run=1 remote=${REMOTE2} status=passed"
${BERK} show -o -r 0 | grep -q "${TESTDATA}"
${BERK} show -o -r 1 | grep -q "${TESTDATA}"
echo "OK"

echo -n "Exec (asynchronous, single host, status failed)... "
${BERK} exec -c "incorrectcommand" "${REMOTE1}" > /dev/null
${BERK} wait
${BERK} show | wc -l | grep -q "5"
${BERK} show | grep -q "id=.* datetime=.*"
${BERK} show | grep -q "total=1 complete=1 aborted=0 passed=0 failed=1"
${BERK} show | grep -q "    run=0 remote=${REMOTE1} status=failed"
${BERK} show -e -r 0 | grep -q "incorrectcommand"
echo "OK"

echo -n "Exec (asynchronous, multiple hosts, status failed)... "
${BERK} exec -c "incorrectcommand" "${REMOTE1}" "${REMOTE2}" > /dev/null
${BERK} wait
${BERK} show | wc -l | grep -q "6"
${BERK} show | grep -q "id=.* datetime=.*"
${BERK} show | grep -q "total=2 complete=2 aborted=0 passed=0 failed=2"
${BERK} show | grep -q "    run=0 remote=${REMOTE1} status=failed"
${BERK} show | grep -q "    run=1 remote=${REMOTE2} status=failed"
${BERK} show -e -r 0 | grep -q "incorrectcommand"
${BERK} show -e -r 1 | grep -q "incorrectcommand"
echo "OK"

echo -n "Exec (asynchronous, single host, abort)... "
${BERK} exec -c "sleep 10" "${REMOTE1}" > /dev/null
${BERK} stop
${BERK} show | wc -l | grep -q "5"
${BERK} show | grep -q "id=.* datetime=.*"
${BERK} show | grep -q "total=1 complete=1 aborted=1 passed=0 failed=0"
${BERK} show | grep -q "    run=0 remote=${REMOTE1} status=aborted"
echo "OK"
