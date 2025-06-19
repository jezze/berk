#!/bin/bash

set -e

WD="$(pwd)"
BERK="${WD}/berk"

rm -rf ${WD}/tmp
mkdir ${WD}/tmp
cd ${WD}/tmp

berk | grep -q "Usage: berk"
berk log 2>&1 | grep -q "berk: Could not find '.berk' directory."
berk init | grep -q "Initialized berk in '.berk'."
berk log 2>&1 | grep -q "berk: Unable to open state."
berk add -t local test | grep -q "Remote 'test' added."
berk exec -n test "uptime" > /dev/null
berk log | grep -q "id="
berk log | wc | tr -s ' ' | grep -q " 5 9 144"
berk log HEAD | grep -q "total=1"
berk log HEAD 0 | grep -q "days"
berk exec test "uptime" > /dev/null
sleep 1
berk log | grep -q "id="
berk log HEAD | grep -q "run="
berk log HEAD 0 | grep -q "days"

cd ${WD}
rm -rf ${WD}/tmp
