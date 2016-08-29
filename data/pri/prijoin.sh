#!/bin/bash

for FILE in 20t10scans cfartest; do
    [[ -f ${FILE}.pri ]] && exit 1
    cat ${FILE}.pri.* > ${FILE}.pri
done

