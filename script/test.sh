#!/bin/bash

set -e

WD="$(pwd)"
BERK="${WD}/berk"

rm -rf ${WD}/tmp
mkdir ${WD}/tmp
cd ${WD}/tmp

echo -n "Basic... "
berk | grep -q "Usage: berk"
berk log 2>&1 | grep -q "berk: Could not find '.berk' directory."
berk init | grep -q "Initialized berk in '.berk'."
berk log 2>&1 | grep -q "berk: Unable to open state."
echo "PASSED"

echo -n "Add local... "
berk add -t local test | grep -q "Remote 'test' added."
echo "PASSED"

echo -n "Synchronous exec... "
berk exec -n test "uptime" > /dev/null
berk log | grep -q "id="
berk log | wc | tr -s ' ' | grep -q " 5 9 144"
berk log HEAD | grep -q "total=1"
berk log HEAD 0 | grep -q "days"
echo "PASSED"

echo -n "Asynchronous exec... "
berk exec test "uptime" > /dev/null
sleep 2
berk log | grep -q "id="
berk log HEAD | grep -q "run="
berk log HEAD 0 | grep -q "days"
echo "PASSED"

cd ${WD}
rm -rf ${WD}/tmp
