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
  #if [[ $((hex_len % 2)) -ne "0" ]];
  #then
  #  hex="0$hex"
  #fi
  hex_escaped=$(echo "$hex" | sed 's/.\{2\}/&\\x/g;s/\\x$//')
  echo "\x$hex_escaped"
}

PAYLOAD=''
DELIM='\xff\xff'
ID='\x01'
DATA='\xff\xf1'
SEGN='\x01'
MESSAGE='Hello World!'
LEN_TEXT=${#MESSAGE} # only works for ASCII, need to set LANG=C otherwise
LEN=$(to_hex "$LEN_TEXT")

test1="$DELIM$ID$DATA$SEGN$LEN$MESSAGE$DELIM"

echo -n -e "$test1" | nc -u 127.0.0.1 8000 &
disown
echo -n -e "$test1" | nc -u 127.0.0.1 8000 &
disown

#exec 3<>/dev/udp/127.0.0.1/8000
#echo -n -e "$test1" >&3 &
#exec 3>&-

#sleep 1
#echo -n -e "$test1" > /dev/udp/127.0.0.1/8000
#sleep 1
#echo -n -e "$test1" > /dev/udp/127.0.0.1/8000

trap ' ' INT
tail -f "$fn"
trap - INT

echo 'killing server process, removing temp file'
kill -9 "$server_pid" > /dev/null 2>&1
rm "$fn"
