#!/bin/bash

cd ${0%/*}

#sfn=$(mktemp)
sfn=server.log
cfn=client.log

echo 'starting server'
./bin/server.bin localhost 8000 > "$sfn" 2>&1 &
server_pid="$!"
disown

echo 'server started, pid' "$server_pid"

echo '************************************************************************'
echo 'RUNNING CLIENT DEMO, WRITING TO LOG FILES'
echo '************************************************************************'
echo -e 'CLIENT DEMO===============================================\n' > "$cfn"
./bin/client.bin localhost 8000 >> "$cfn" 2>&1

echo -e '\nACK_TIMER DEMO============================================\n' >> "$cfn"
echo '************************************************************************'
echo 'RUNNING ACK_DEMO, WRITING TO LOG FILES, IT WILL TAKE ~9 SECONDS TO RUN'
echo '************************************************************************'
# I use the wrong port here because it demonstrates the ACK_TIMER in action
./bin/client.bin localhost 8001 >> "$cfn" 2>&1

#trap ' ' INT
#tail -f "$sfn" "$cfn"
#trap - INT
#
#echo 'killing server process, removing temp file'
kill -9 "$server_pid" > /dev/null 2>&1
#rm "$sfn"
