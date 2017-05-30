C99 is required for variable length stack allocated buffers / arrays and
compound literals.

Segments of byte arrays are converted to 2 and 4 byte integers. This was done
using the helper functions noths, htons, and ntohl, which is intended to be
used for converting between host byte order (little or big endian) and network
order (big endian).
