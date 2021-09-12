/*
 Author: Robert Graham

 This code is a demonstration of how to extract records from Dennis
 Montgomery's "BLX" files. This code is based on the spec in the
 README.md file located in this directory.

 In short, the BLX are full of randomized bytes into which lines
 of text (specifically, CSV text) have been inserted in random
 locations, marked by a "start" and "end" delimeter. To extract the
 CSV data from these files, you search until you find a start-of-record
 delimeter, and then print all the bytes until the end-of-record
 delimeter.

 In addition to all this, everything has been rotated 3 positions to
 the left.

 All of this is bizarre. There's no reason to do such things as rotate
 bytes.

 This code is ANSI C99. It'll compile everything, especially Windows, Linux,
 and macOS. It'll work on 64-bit and 32-bit platforms, even with files that
 are larger than 4-gigabytes (which often exceed the limits of 32-bit
 platforms).
*/
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>

/**
 * These are the strings that mark the start of records. Each of four
 * passes use a different string. Since ROT3 is used to encode the file,
 * either the data or these strings must be rotated to match. */
char start_of_record[4][16] = {
    "xT1y22", "tx16!!", "eTreppid1!", "shaitan123"
};
char start_len[4] = {6, 6, 10, 10};
char is_first[256] = {0};

/**
 * Rotate all bytes in a string by 3 to the left. This is called when
 * we find a record and need to "decode" it.
 */
void ROT3_left(unsigned char *dst, const unsigned char *src, size_t length) {
    size_t i;
    for (i=0; i<length; i++)
        dst[i] = src[i] - 3; /* this can wrap */
}

/**
 * Rotate all the bytes in a string by 3 to the right. Called only
 * on startup to convert the `start_of_record` patterns.
 */
void ROT3_right(char* dst, const char *src, size_t length) {
    size_t i;
    for (i = 0; i < length; i++)
        dst[i] = src[i] + 3; /* may wrap */
}

/**
 * Searches the buffer for the pattern, returning the number of bytes up
 * to the start of that pattern.
 */
int delim_end(const unsigned char *buf, size_t length,
        const char *delim, size_t delim_length) {
    int i;

    if (delim_length > length)
        return -1;
    length -= (delim_length-1);

    for (i=0; i<length; i++) {
        if (buf[i] == delim[0] && memcmp(buf+i, delim, delim_length) == 0)
            return i;
    }

    return -1;
}

/**
 * Searches from this point forward for a start-of-record delimiter.
 * There are some quirks to this:
 * 1. We only read a chunk of the file at a time, and it's possible that a
 *    delimeter may cross chunk boundaries. Therefore, we stop the search
 *    before we reach the very end of the buffer, but instead copy the
 *    remaining bytes to the front of the next chunk.
 * 2. The algorithm is optimized to search one-byte-at-a-time. This is where
 *    99% of the time is taken reading the file. This could still be
 *    optimized substantially, such as using a Boyer-Moore technique,
 *    but the goal is more clarity than speed.
 * 3. There are 4 start-of-record delimiters that we are looking for at the
 *    same time. However, the 'mask' parameter can be used to limit search
 *    for only one of the delimeters.
 */
void delim_search(const unsigned char *buf, size_t *offset, size_t remaining, unsigned mask) {
    size_t i = *offset;

    /* Search from this point until the end of the buffer */
    while (i < remaining) {
        size_t j;

        /* Search forward until the byte matches one of the patterns. Note
         * that we've placed a delimiter character ('{') past the end of the
         * buffer, so that this loop terminates at the end of the buffer
         * without us having to also check a length variable. It is this loop
         * that consumes 99% of the CPU power of the program. */
        while (!is_first[buf[i]]) {
            i++;
        }

        /* If we are too close to the end, then break. This triggers when we
         * have reached the end of the buffer without finding anything but
         * also when the first character of a delimiter has matched but we
         * don't have enough space left for a record. In this second case,
         * we break out of this loop, refill the buffer, and continue
         * scanning where we left off.*/
        if (remaining - i < 10 + 1024) {
            break;
        }

        /* At this point, we've matched the first byte of a delimeter. Now
         * to a match of the entire pattern. */
        for (j=0; j<4; j++) {

            /* Instead of doing all 4 patterns simulatneously, the 'mask'
             * parameter can be used to restrict search for only one. */
            if (((1<<j) & mask) == 0)
                continue;

            if (memcmp(start_of_record[j], buf+i, start_len[j]) == 0) {
                *offset = i + start_len[j];
                return;
            }
        }

        /* If we match the first byte of a delimiter, but didn't match the
         * remaining bytes, then we skip past this byte and continue
         * searching. */
        i++;
    }

    /* We didn't find anything. So just return the new offset. */
    *offset = i;
}

/**
 * Given a filename, search through all the records and print the results.
 * This has a quirk for efficiency. The original program does 4 passes through
 * a file, each time with a different start-of-delimeter pattern. We do a
 * single-pass by default. This means records may be produced in a different
 * order tha nthe origianl program. If the original order is desired,
 * then the 'mask' parameter can be changed to do one pass at a time.
 */
int extract_file(const char* filename, FILE* out, unsigned mask) {
    size_t i = 0;
    unsigned char *buf;
    FILE *fp;
    size_t remaining = 0;
    size_t total_bytes = 0;
    size_t total_offset  = 0;
    static const size_t BUFSIZE = 16*1024*1024;

    /* Open the file */
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "[-] %s:%s: %s\n", filename, "open", strerror(errno));
        return 1;
    }
    fprintf(stderr, "[-] %s: opened [pass:%x]\n", filename, mask);

    /* Create a buffer big enough to hold 1-megabyte at a time, plus a
     * litte extra at the end for when we cross boundaries. */
    buf = malloc(BUFSIZE+4096);

    /* Search for records. Each iteration of this loop finds a single
     * record and prints it. Whenever we near the end of the buffer,
     * but before we reach the end, we refill the buffer with more
     * bytes. Thus, we make sure there's always space for start-of-record
     * delimiter (10 bytes) plus a full record (1024 bytes). */
    for (i=0, remaining=0; ; ) {
        unsigned char record[1024];
        int record_length;

        /* If the remaining data in the buffer isn't long enough, move
         * the remaining bytes to the front, then fill out the buffer.
         * Note: this happens in the first first iteration of the loop,
         * and then each time we near the end of the buffer in
         * subsequent iterations. */
        if (i + 16 + 1024 > remaining) {
            size_t bytes_read;

            /* Move the remaining bytes at the end of the buffer to the
             * front. */
            remaining -= i;
            memmove(buf, buf + i, remaining);
            total_offset += i;
            i = 0;

            /* Fill the buffer with additional data past that point */
            bytes_read = fread(buf + remaining, 1, BUFSIZE-remaining, fp);
            if (bytes_read == 0)
                break;
            total_bytes += bytes_read;
            remaining += bytes_read;

            /* This happens when we reach the end of the file.
             * Note: this means the last few bytes of a file may not be
             * searched when there aren't enough bytes left to hold a reocrd */
            if (remaining < 10 + 1024)
                break;

            /* Optimization: We place a byte that we search for one past the
             * end of the buffer. This means the search step ends at the
             * end of the bufrer simply by checking the byte value instead
             * also checking the length. */
            buf[remaining] = start_of_record[0][0];
        }

        /*
         * search forward for one of the leading patterns. This function
         * terminates either:
         * 1. we've reached the end of the buffer.
         * 2. we've found a start-of-record delimiter.
         */
        delim_search(buf, &i, remaining, mask);

        /*
         * If we've reached the end of the buffer without finding a pattern,
         * then loop back around. If this triggers, then the buffer-refill
         * step at the top of the loop wil lbe executed.
         */
        if (remaining - i < 10 + 1024)
            continue;

        /*
         * Rotate all the bytes.
         */
        ROT3_left(record, buf+i, 1024);

        /*
         * Search for the end-of-record delimeter. This won't always succeed,
         * possibly because the random bytes in the file can sometimes
         * trigger false-positives in the start-of-record delimiter.
         */
        record_length = delim_end(record, 1024, ".dev@7964", 9);
        if (record_length == -1)
            continue;

        /*
         * Write the output. The original program first parses the CSV fields
         * and then writes them as a normalized CSV format. However, in the
         * existing samples, they are all identical anyway.
         */
        fprintf(out, "%.*s\r\n", record_length, record);
    }

    fprintf(stderr, "[+] %s: processed\n", filename);

    fclose(fp);
    free(buf);
    return 0;
}

/**
 * The file-format works by having a 'delimeter" at the start of each record.
 * We search forward to find those delimiters. We want to optimize this search
 * a bit with static tables (see above). We could just hard-code these values,
 * but I initialize them programatically instead to document what they are
 * doing.
 */
static void delim_initialize(void) {
    size_t i;

    /* We search for start-of-record patternss. The data has been rotated by 3.
     * This means we can either rotate all incoming bytes to match the patterns,
     * or we can rotate the patterns themselves, requiring no change to the input.
     * This second approach is faster. Now we could just hard-code the rotated
     * patterns into this program, but the original start-of-delimeter patterns
     * seems more readable. */
    for (i=0; i<4; i++) {
        ROT3_right(start_of_record[i],
                   start_of_record[i],
                   strlen(start_of_record[i]));
    }

    /* The search for patterns is the limit on speed, so we want to optimize
     * this. One simple optimization is to create this table for the first
     * byte, so that it's a single CPU operationg to check if the first
     * byte matches. */
    memset(is_first, 0, sizeof(is_first));
    for (i=0; i<4; i++)
        is_first[start_of_record[i][0]&0xFF] = 1;
}

int main(int argc, char *argv[]) {
    int i;
    int err = 0;
    bool is_ordered = false;

    /* If no arguments given, print help */
    if (argc <= 1 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        fprintf(stderr, "usage:\n blxtract <filename> [<filename> ...]\n");
        fprintf(stderr, "Prints results to stdout\n");
        return -1;
    }

    /*
     * Initialize the record starting delimiters. The program will search
     * for one of four delimiters to start each record.
     */
    delim_initialize();

    /* On Windows, we have to remove implicit adding of '\r' to ends of
     * of line so we can add it explicitly ourselves, so that the rest 
     * of the code is the same between Linux and Windows. */
#ifdef _WIN32
    (void)_setmode(1, _O_BINARY);
#endif

    /* Process all files specified on the command-line */
    fprintf(stderr, "[+] blxtract 1.1 - extracting %d files to stdout\n", argc-1);
    for (i=1; i<argc; i++) {
        /* The original algorithm does 4 separate passes over the file,
         * once for each start-of-record pattern. This is needlessly
         * slow, we by default we do a single pass. You can get the original
         * behavior by saying "--ordered" on the command-line */
        if (strcmp(argv[i], "--ordered") == 0) {
            is_ordered = true;
            continue;
        }

        if (is_ordered) {
            err += extract_file(argv[i], stdout, 0x01);
            err += extract_file(argv[i], stdout, 0x02);
            err += extract_file(argv[i], stdout, 0x04);
            err += extract_file(argv[i], stdout, 0x08);
        } else {
            err += extract_file(argv[i], stdout, 0xFF);
        }
    }

    return err;
}
