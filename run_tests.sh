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
  hex_escaped=$(echo "$hex" | sed 's/.\{2\}/&\\x/g;s/\\x$//')
  echo "\x$hex_escaped"
}

DELIM='\xff\xff'
DATA='\xff\xf1'
ACK='\xff\xf2'
REJECT='\xff\xf3'

data_packet() {
	client_id=$(to_hex "$1")
	segment_id=$(to_hex "$2")
	message="$3"
	len_text=${#message}
	len=$(to_hex "$len_text")
	echo $DELIM$client_id$DATA$segment_id$len$message$DELIM
}

test_data_1=$(data_packet 0 0 'Hello')
test_data_2=$(data_packet 0 1 'Hello')
test_data_3=$(data_packet 0 2 'Hello')
test_data_4=$(data_packet 0 2 'Hello')
test_data_5=$(data_packet 0 2 'Hello')

echo $test_data_1


#echo -n -e "$test_data_1" | nc -u 127.0.0.1 8000 &
#disown
#sleep 1
#echo -n -e "$test_data_2" | nc -u 127.0.0.1 8000 &
#disown
#sleep 1
#echo -n -e "$test_data_3" | nc -u 127.0.0.1 8000 &
#disown
#echo -n -e "$test_data_4" | nc -u 127.0.0.1 8000 &
#sleep 0.01
#echo -n -e "$test_data_5" | nc -u 127.0.0.1 8000 &

#echo -n -e "$test_ack" | nc -u 127.0.0.1 8000 &
#echo -n -e "$test_rej" | nc -u 127.0.0.1 8000 &

trap ' ' INT
tail -f "$fn"
trap - INT

echo 'killing server process, removing temp file'
kill -9 "$server_pid" > /dev/null 2>&1
rm "$fn"
