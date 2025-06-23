#!/bin/bash

set -e

WD="$(pwd)"
BERK="${WD}/berk"
REMOTE1="myhost"

rm -rf ${WD}/tmp
mkdir ${WD}/tmp
cd ${WD}/tmp

echo -n "Uninitialized... "
${BERK} | grep -q "Usage: berk"
${BERK} version | grep -q "berk version 0.0.1"
${BERK} log 2>&1 | grep -q "berk: Could not find '.berk' directory."
echo "PASSED"

echo -n "Initialized... "
${BERK} init | grep -q "Initialized berk in '.berk'."
${BERK} log 2>&1 | grep -q "berk: Unable to open log."
echo "PASSED"

echo -n "Local machine... "
${BERK} add -t "local" "${REMOTE1}" | grep -q "Remote '${REMOTE1}' added."
echo "PASSED"

echo -n "Synchronous exec (pass)... "
${BERK} exec -n "${REMOTE1}" "echo test123" > /dev/null
${BERK} log HEAD | wc -l | grep -q "5"
${BERK} log HEAD | grep -q "id=.* datetime=.*"
${BERK} log HEAD | grep -q "total=1 complete=1 aborted=0 passed=1 failed=0"
${BERK} log HEAD | grep -q "    run=0 remote=myhost status=passed"
${BERK} log HEAD 0 | grep -q "test123"
echo "PASSED"

echo -n "Synchronous exec (fail)... "
${BERK} exec -n "${REMOTE1}" "incorrectcommand" > /dev/null
${BERK} log HEAD | wc -l | grep -q "5"
${BERK} log HEAD | grep -q "id=.* datetime=.*"
${BERK} log HEAD | grep -q "total=1 complete=1 aborted=0 passed=0 failed=1"
${BERK} log HEAD | grep -q "    run=0 remote=myhost status=failed"
${BERK} log -e HEAD 0 | grep -q "incorrectcommand: command not found"
echo "PASSED"

echo -n "Asynchronous exec (pass)... "
${BERK} exec "${REMOTE1}" "echo test123" > /dev/null
${BERK} wait HEAD
${BERK} log HEAD | wc -l | grep -q "5"
${BERK} log HEAD | grep -q "id=.* datetime=.*"
${BERK} log HEAD | grep -q "total=1 complete=1 aborted=0 passed=1 failed=0"
${BERK} log HEAD | grep -q "    run=0 remote=myhost status=passed"
${BERK} log HEAD 0 | grep -q "test123"
echo "PASSED"

echo -n "Asynchronous exec (fail)... "
${BERK} exec "${REMOTE1}" "incorrectcommand" > /dev/null
${BERK} wait HEAD
${BERK} log HEAD | wc -l | grep -q "5"
${BERK} log HEAD | grep -q "id=.* datetime=.*"
${BERK} log HEAD | grep -q "total=1 complete=1 aborted=0 passed=0 failed=1"
${BERK} log HEAD | grep -q "    run=0 remote=myhost status=failed"
${BERK} log -e HEAD 0 | grep -q "incorrectcommand: command not found"
echo "PASSED"

cd ${WD}
rm -rf ${WD}/tmp
