#!/bin/bash

DHOST=127.0.0.1
DPORT=8000

#           |   -   | i |   t   | j | l | d |   -   |
echo -n -e '\xff\xff\x01\xff\xf1\x00\x10\x10\xff\xff' > /dev/udp/$DHOST/$DPORT
