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


