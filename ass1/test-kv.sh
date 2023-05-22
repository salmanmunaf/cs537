#! /bin/bash

if ! [[ -x kv ]]; then
    echo "kv executable does not exist"
    exit 1
fi

/p/course/cs537-remzi/tests/tester/run-tests.sh $*
