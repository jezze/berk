#!/bin/bash

set -e

WD="$(pwd)"
BERK="${WD}/berk"
REMOTE1="myhost1"
REMOTE2="myhost2"

rm -rf ${WD}/tmp
mkdir ${WD}/tmp
cd ${WD}/tmp

echo -n "Uninitialized... "
${BERK} | grep -q "Usage: berk"
${BERK} version | grep -q "berk version 0.0.1"
for cmd in "add config exec list log remove send shell stop wait"
do
    ${BERK} ${cmd} 2>&1 | grep -q "berk: Could not find '.berk' directory."
done
echo "PASSED"

echo -n "Initialized... "
${BERK} init | grep -q "Initialized berk in '.berk'."
${BERK} log 2>&1 | grep -q "berk: Unable to open log."
echo "PASSED"

echo -n "Local machine... "
${BERK} add -t "local" "${REMOTE1}" | grep -q "Remote '${REMOTE1}' added."
${BERK} add -t "local" "${REMOTE2}" | grep -q "Remote '${REMOTE2}' added."
echo "PASSED"

echo -n "Synchronous exec (single, pass)... "
${BERK} exec -n "${REMOTE1}" "echo test123" > /dev/null
${BERK} log HEAD | wc -l | grep -q "5"
${BERK} log HEAD | grep -q "id=.* datetime=.*"
${BERK} log HEAD | grep -q "total=1 complete=1 aborted=0 passed=1 failed=0"
${BERK} log HEAD | grep -q "    run=0 remote=${REMOTE1} status=passed"
${BERK} log HEAD 0 | grep -q "test123"
echo "PASSED"

echo -n "Synchronous exec (multiple, pass)... "
${BERK} exec -n "${REMOTE1} ${REMOTE2}" "echo test123" > /dev/null
${BERK} log HEAD | wc -l | grep -q "6"
${BERK} log HEAD | grep -q "id=.* datetime=.*"
${BERK} log HEAD | grep -q "total=2 complete=2 aborted=0 passed=2 failed=0"
${BERK} log HEAD | grep -q "    run=0 remote=${REMOTE1} status=passed"
${BERK} log HEAD | grep -q "    run=1 remote=${REMOTE2} status=passed"
${BERK} log HEAD 0 | grep -q "test123"
${BERK} log HEAD 1 | grep -q "test123"
echo "PASSED"

echo -n "Synchronous exec (single, fail)... "
${BERK} exec -n "${REMOTE1}" "incorrectcommand" > /dev/null
${BERK} log HEAD | wc -l | grep -q "5"
${BERK} log HEAD | grep -q "id=.* datetime=.*"
${BERK} log HEAD | grep -q "total=1 complete=1 aborted=0 passed=0 failed=1"
${BERK} log HEAD | grep -q "    run=0 remote=${REMOTE1} status=failed"
${BERK} log -e HEAD 0 | grep -q "incorrectcommand: command not found"
echo "PASSED"

echo -n "Synchronous exec (multiple, fail)... "
${BERK} exec -n "${REMOTE1} ${REMOTE2}" "incorrectcommand" > /dev/null
${BERK} log HEAD | wc -l | grep -q "6"
${BERK} log HEAD | grep -q "id=.* datetime=.*"
${BERK} log HEAD | grep -q "total=2 complete=2 aborted=0 passed=0 failed=2"
${BERK} log HEAD | grep -q "    run=0 remote=${REMOTE1} status=failed"
${BERK} log HEAD | grep -q "    run=1 remote=${REMOTE2} status=failed"
${BERK} log -e HEAD 0 | grep -q "incorrectcommand: command not found"
${BERK} log -e HEAD 1 | grep -q "incorrectcommand: command not found"
echo "PASSED"

echo -n "Asynchronous exec (single, pass)... "
${BERK} exec "${REMOTE1}" "echo test123" > /dev/null
${BERK} wait HEAD
${BERK} log HEAD | wc -l | grep -q "5"
${BERK} log HEAD | grep -q "id=.* datetime=.*"
${BERK} log HEAD | grep -q "total=1 complete=1 aborted=0 passed=1 failed=0"
${BERK} log HEAD | grep -q "    run=0 remote=${REMOTE1} status=passed"
${BERK} log HEAD 0 | grep -q "test123"
echo "PASSED"

echo -n "Asynchronous exec (multiple, pass)... "
#${BERK} exec "${REMOTE1} ${REMOTE2}" "echo test123" > /dev/null
#${BERK} wait HEAD
#${BERK} log HEAD | wc -l | grep -q "6"
#${BERK} log HEAD | grep -q "id=.* datetime=.*"
#${BERK} log HEAD | grep -q "total=2 complete=2 aborted=0 passed=2 failed=0"
#${BERK} log HEAD | grep -q "    run=0 remote=${REMOTE1} status=passed"
#${BERK} log HEAD | grep -q "    run=1 remote=${REMOTE2} status=passed"
#${BERK} log HEAD 0 | grep -q "test123"
#${BERK} log HEAD 1 | grep -q "test123"
echo "PASSED"

echo -n "Asynchronous exec (single, fail)... "
${BERK} exec "${REMOTE1}" "incorrectcommand" > /dev/null
${BERK} wait HEAD
${BERK} log HEAD | wc -l | grep -q "5"
${BERK} log HEAD | grep -q "id=.* datetime=.*"
${BERK} log HEAD | grep -q "total=1 complete=1 aborted=0 passed=0 failed=1"
${BERK} log HEAD | grep -q "    run=0 remote=${REMOTE1} status=failed"
${BERK} log -e HEAD 0 | grep -q "incorrectcommand: command not found"
echo "PASSED"

echo -n "Asynchronous exec (multiple, fail)... "
#${BERK} exec "${REMOTE1}" "incorrectcommand" > /dev/null
#${BERK} wait HEAD
#${BERK} log HEAD | wc -l | grep -q "6"
#${BERK} log HEAD | grep -q "id=.* datetime=.*"
#${BERK} log HEAD | grep -q "total=2 complete=2 aborted=0 passed=0 failed=2"
#${BERK} log HEAD | grep -q "    run=0 remote=${REMOTE1} status=failed"
#${BERK} log HEAD | grep -q "    run=1 remote=${REMOTE2} status=failed"
#${BERK} log -e HEAD 0 | grep -q "incorrectcommand: command not found"
#${BERK} log -e HEAD 1 | grep -q "incorrectcommand: command not found"
echo "PASSED"

echo -n "Asynchronous exec (single, abort)... "
${BERK} exec "${REMOTE1}" "sleep 10" > /dev/null
${BERK} stop HEAD
${BERK} log HEAD | wc -l | grep -q "5"
${BERK} log HEAD | grep -q "id=.* datetime=.*"
${BERK} log HEAD | grep -q "total=1 complete=1 aborted=1 passed=0 failed=0"
${BERK} log HEAD | grep -q "    run=0 remote=${REMOTE1} status=aborted"
echo "PASSED"

cd ${WD}
rm -rf ${WD}/tmp
