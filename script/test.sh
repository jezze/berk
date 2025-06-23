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

echo -n "Initialized... "
${BERK} init | grep -q "Initialized berk in '.berk'."
${BERK} log 2>&1 | grep -q "berk: Unable to open log."
echo "PASSED"

echo -n "Local machine... "
${BERK} add -t local ${REMOTE1} | grep -q "Remote '${REMOTE1}' added."
echo "PASSED"

echo -n "Synchronous exec... "
${BERK} exec -n "${REMOTE1}" "uptime" > /dev/null
${BERK} log | grep -q "id="
${BERK} log | wc -l | grep -q "5"
${BERK} log HEAD | grep -q "total=1"
${BERK} log HEAD 0 | grep -q "load average"
echo "PASSED"

echo -n "Asynchronous exec... "
${BERK} exec "${REMOTE1}" "uptime" > /dev/null
${BERK} wait HEAD
${BERK} log | grep -q "id="
${BERK} log HEAD | grep -q "run="
${BERK} log HEAD 0 | grep -q "load average"
echo "PASSED"

cd ${WD}
rm -rf ${WD}/tmp
