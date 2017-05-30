Lauren Howard COEN 233 Spring 2017

To compile, run `make`.

To run the demo, run `./run_demo.sh` bash script.

If you would like to run the server, run `./run_server.sh`. By default this
uses localhost 8000 to listen for UDP packets on. If you want to run the
client, you should therefore run `./bin/client.bin localhost 8000`.

You can manually run the server as well (make sure Verification_Database.txt is
in your working path):
`./bin/server.bin localhost 8000`

Then you can run the client with:
`./bin/client.bin localhost 8000`

NOTES:
C99 is required for variable length stack allocated buffers / arrays and
compound literals.

Segments of byte arrays are converted to 2 and 4 byte integers. This was done
using the helper functions noths, htons, and ntohl, which is intended to be
used for converting between host byte order (little or big endian) and network
order (big endian).
