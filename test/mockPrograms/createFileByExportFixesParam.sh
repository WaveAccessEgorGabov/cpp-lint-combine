#!/bin/bash

exportFixesStr="--export-fixes="
if [[ "$1" == $exportFixesStr* ]]; then
    touch ${1#$exportFixesStr}
fi
exit 0
