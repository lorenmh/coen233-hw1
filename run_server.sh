#!/bin/bash

cd ${0%/*}

sfn=$(mktemp)

echo 'starting server'
./bin/server.bin localhost 8000 > "$sfn" 2>&1 &
server_pid="$!"
disown

trap ' ' INT
tail -f "$sfn"
trap - INT

echo 'killing server process, removing temp file'
kill -9 "$server_pid" > /dev/null 2>&1
rm "$sfn"
