#!/bin/bash

fn=$(mktemp)

echo 'starting server'
./bin/server.bin localhost 8000 > "$fn" 2>&1 &
server_pid="$!"
disown

echo 'server started, pid' "$server_pid"

sleep 0.1

to_hex(){
  n="$1"
  hex=$(echo "obase=16; $n" | bc)
  hex_len=${#hex}
  # leftpad
  #if [[ $((hex_len % 2)) -ne "0" ]];
  #then
  #  hex="0$hex"
  #fi
  hex_escaped=$(echo "$hex" | sed 's/.\{2\}/&\\x/g;s/\\x$//')
  echo "\x$hex_escaped"
}

PAYLOAD=''
DELIM='\xff\xff'
ID='\x00'
DATA='\xff\xf1'
ACK='\xff\xf2'
REJECT='\xff\xf3'
REJECT_ID='\xff\xf5'
SEGN='\x00'
MESSAGE='Hello World!'
LEN_TEXT=${#MESSAGE} # only works for ASCII, need to set LANG=C otherwise
LEN=$(to_hex "$LEN_TEXT")

test_data="$DELIM$ID$DATA$SEGN$LEN$MESSAGE$DELIM"
test_ack="$DELIM$ID$ACK$SEGN$DELIM"
test_rej="$DELIM$ID$REJECT$REJECT_ID$SEGN$DELIM"

echo -n -e "$test_data" | nc -u 127.0.0.1 8000 &
echo -n -e "$test_ack" | nc -u 127.0.0.1 8000 &
echo -n -e "$test_rej" | nc -u 127.0.0.1 8000 &

trap ' ' INT
tail -f "$fn"
trap - INT

echo 'killing server process, removing temp file'
kill -9 "$server_pid" > /dev/null 2>&1
rm "$fn"
