# blxtract

Mike Lindell is a rich American businessman who claims he has "absolute proof" the Nov 2020 election
was hacked -- proof in the form of "pcaps" on the days around the election from all over the U.S.
On August 10 2021 he held a "cyber-symposium" where he invited "cyber-experts" to review the pcaps.
He didn't provide pcaps. Instead, he provided almost 300 gigabytes of `.bin` files in a format
known as "BLX" created by a guy named [Dennis Montgomery](https://https://en.wikipedia.org/wiki/Dennis_L._Montgomery).
The data included the source for a program called `CExtractor` that would extract data from that
file format. But, this code is written in a deliberately obfuscatory manner that is very hard to
read.

This project rewrites that code in a simpler manner that programmers can read. I'm in the process
of removing or changing things so that it continues to produce the identical output, but in
a more readable way.

The original 300 gigabytes of data is available at this BitTorrent magnet link:

  magnet:?xt=urn:btih:39a9590de21e77687fdf7eacee4dd743f2683d72&dn=cyber-symposium&tr=udp://9.rarbg.me:2780/announce

The code is in Microsoft's C++/CLR language. 

## Specification

The extract program works like the following.

The data has been encoded with ROT3, meaning they've been rotated 3 positions
to the left, meaning the number 3 has been subtracted from each character.

The data we are able to extract accounts for less than 1% of size of the
files we have, like `rnx-000001.bin`. The remainder of the files consist of
either random junk data or encrypted records that we cannot extract without
the key.

The code does 4 passes over the file looking for records embedded in the file.
Each pass starts at the beginning of the file proceeding to the end.

A pass looks for a start-of-record pattern, a different pass for each pattern.
The list of start-of-record delimeters are:
- "xT1y22"
- "tx16!!"
- "eTreppid1!"
- "shaitan123"

Note that these are the plain-text patterns. When scanning the file for the
pattern, you must either rotate-left each incoming byte, or rotate-right the
bytes of the patterns. In other words, the actual start-of-record patterns
in the raw file look like:
- "{W4|55"
- "w{49$$"
- "hWuhsslg4$"
- "vkdlwdq456"

When a delimeter is found, it then reads the next 1024 bytes of the file that
follow the start-of-record.

ROT3 (subtract 3 from each byte) is then applied to all 1024 bytes, meaing, 
the value 3 is subtracted from all the bytes.

It then looks for an end-of-record delimeter of ".dev@7964" and truncates the record
at that point (removing the end delimeter and everything after). This is the plain-text
delimeter that matches after ROT3 conversion of the data.

The remaining record is then written to the output. Each record is written with an additional
CRLF ("\r\n") at the end of the line.

It would be 4 times faster to do a single pass searching for all 4 delimiters at once,
instead of 4 separate passes. However, this would produce data in a different order.

