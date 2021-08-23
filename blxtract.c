/*
    Author: Robert Graham

 This code extrats the CSV records from Dennis Montgomery's BLX files.
 This code is based upon the spec written up in the README.md
 file.

 Just compile it on Windows or Linux. It doesn't have any special
 dependencies.
*/
#define _FILE_OFFSET_BITS 64
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#if defined(_WIN32)
# include <windows.h>
# include <winerror.h>
# include <io.h>
# define open _open
# define close _close
# define fstat _fstat64
# define stat __stat64
# define fdopen _fdopen
#else
# include <unistd.h>
# include <sys/mman.h>
#endif

/**
 * These are the strings that mark the start of records. Each of four
 * passes use a different string. Since ROT3 is used to encode the file,
 * either the data or these strings must be rotated to match. */
char start_of_record[4][16] = {
    "xT1y22", "tx16!!", "eTreppid1!", "shaitan123"
};

/**
 * Rotate all bytes in a string by 3 to the left. This is called when
 * we find a record and need to "decode" it.
 */
void ROT3_left(char *buf, unsigned long long length) {
    unsigned long long i;
    for (i=0; i<length; i++)
        buf[i] -= 3;
}

/**
 * Rotate all the bytes in a string by 3 to the right. Called only
 * on startup to convert the `start_of_record` patterns.
 */
void ROT3_right(char* buf, unsigned long long length) {
    unsigned long long i;
    for (i = 0; i < length; i++)
        buf[i] += 3;
}


/**
 * Cross-platform memory-map function. 
 */
char* map_file(int fd, unsigned long long filesize) {
#ifdef _WIN32
    HANDLE hFile = (HANDLE)_get_osfhandle(fd);
    HANDLE hFileMap;
    char* buf;

    hFileMap = CreateFileMappingA(hFile,
        NULL,
        PAGE_READONLY,
        0,
        0,
        NULL);
    if (hFileMap == NULL)
        return NULL;

    buf = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0);
    if (buf == NULL) {
        fprintf(stderr, "[-] MapViewOfFile() %d\n", GetLastError());
    }

    return buf;
#else
    int pagesize = getpagesize();
    char* buf;
    filesize = filesize;
    fprintf(stderr, "[+] pagesize = %d\n", pagesize);
    buf = mmap(0, filesize, PROT_READ, MAP_SHARED, fd, 0);
    if (buf == NULL) {
        fprintf(stderr, "[-] %s: %s\n", "mmap", strerror(errno));
        return NULL;
    }
    return buf;
#endif

}

/**
 * Cross-platform memory-map unmap function.
 */
void unmap_file(void* buf, unsigned long long filesize) {
#ifdef _WIN32
    UnmapViewOfFile(buf);
#else
    munmap(buf, filesize);
#endif
}

/**
 * Tests if the delimeter matches at this point in the buffer. This is an
 * inline function so should in theory simply match the first byte most
 * of the time.
 */
static inline bool is_matched(const char *buf, unsigned long long offset, unsigned long long length, const char *delim, size_t delim_length) {
    if (offset + delim_length >= length)
        return false;
    if (buf[offset] != delim[0])
        return false;
    if (memcmp(buf+offset, delim, delim_length) != 0)
        return false;
    return true;
}

/**
 * Searches a buffer for a pattern, returning the location in that buffer of the pattern.
 */
int index_of(const char *buf, unsigned long long offset, unsigned long long length, const char *delim, size_t delim_length) {
    unsigned long long i;

    for (i=offset; i<length; i++) {
        if (is_matched(buf, i, length, delim, delim_length))
            return (int)(i - offset);
    }
    return -1;
}

/**
 * Do a single pass through the file. Called 4 times per file, each time
 * with a different delimiter.
 */
void extract_once(const char *buf, unsigned long long length, const char *delim, FILE *out) {
    unsigned long long offset = 0;
    size_t delim_length = strlen(delim);

    for (offset=0; offset<length; offset++) {
        char record[1024];
        int record_length;

        /* Search through file for a starting delimiter */
        if (!is_matched(buf, offset, length, delim, delim_length))
            continue;
        
        /* We found a starting delimeter, so "decrypt" the record
         * that follows */
        offset += delim_length;
        if (offset + 1024 >= length)
            break;
        memcpy(record, buf + offset, 1024);
        ROT3_left(record, 1024);
        offset += 1024;

        /* Find the end-of-record delimeter and truncate the record there */
        record_length = index_of(record, 0, 1024, ".dev@7964", 9);
        if (record_length == -1)
            continue;

        /* Write the output */
        fprintf(out, "%.*s\r\n", record_length, record);
    }
}


/**
 * Map the file into memory, then process it four times
 * for each record delimiter.
 */
int extract_file(const char *filename, FILE *out) {
    int i;
    struct stat s = {0};
    int err;
    unsigned long long filesize;
    int fd = -1;
    char *buf;

    /* Open the file */
    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "[-] %s:%s: %s\n", filename, "open", strerror(errno));
        goto fail;
    }
    fprintf(stderr, "[+] %s: opened\n", filename);

    /* Find the size of the file */
    err = fstat(fd, &s);
    if (err) {
        fprintf(stderr, "[-] %s:%s: %s\n", filename, "fstat", strerror(errno));
        goto fail;
    }
    filesize = s.st_size;
    fprintf(stderr, "[+] %s: filesize=%llu\n", filename, (unsigned long long)filesize);

    /* memory map the file, do it differently for Windows/Linux/macOS */
    buf = map_file(fd, filesize);
    if (buf == NULL)
        goto fail;
    fprintf(stderr, "[+] %s: mapped into memory\n", filename);

    /* Perform 4 passes over the file */
    for (i=0; i<4; i++) {
        fprintf(stderr, "[+] pass#%d delim=%s\n", i, start_of_record[i]);
        extract_once(buf, filesize, start_of_record[i], out);
    }

    /* Unmap and close file */
    unmap_file(buf, filesize);
    close(fd);
    return 0;
fail:
    if (fd != -1)
        close(fd);
    return 1;
}


void extract_once2(FILE* fp, const char* delim, FILE* out) {
    size_t delim_length = strlen(delim);

    for (;;) {
        size_t matched_count = 0;
        int c = getc(fp);

        if (c == EOF)
            break;

        /* Search through file for a starting delimiter */
        if (delim[matched_count] == c)
            matched_count++;
        else
            matched_count = 0; /* reset back to zero */
        if (matched_count != delim_length)
            continue;

        /* After the start-of-record delimiter, read the record*/
        char record[1024];
        int record_length;
        size_t bytes_read;
        
        bytes_read = fread(record, 1, sizeof(record), fp);
        if (bytes_read != sizeof(record))
            break;
        ROT3_left(record, 1024);
     
        /* Find the end-of-record delimeter and truncate the record there */
        record_length = index_of(record, 0, 1024, ".dev@7964", 9);
        if (record_length == -1)
            continue;

        /* Write the output */
        fprintf(out, "%.*s\r\n", record_length, record);
    }
}

int extract_file2(const char* filename, FILE* out) {
    int i;

    /* Perform 4 passes over the file */
    for (i = 0; i < 4; i++) {
        FILE* fp;

        /* Open the file */
        fp = fopen(filename, "rb");
        if (fp == NULL) {
            fprintf(stderr, "[-] %s:%s: %s\n", filename, "open", strerror(errno));
            return 1;
        }
        fprintf(stderr, "[+] %s: opened\n", filename);
        fprintf(stderr, "[+] pass#%d delim=%s\n", i, start_of_record[i]);
        extract_once2(fp, start_of_record[i], out);
        fclose(fp);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    int i;
    int err = 0;

    /* If no arguments given, print help */
    if (argc <= 1 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        fprintf(stderr, "usage:\n blxtract <filename> [<filename> ...]\n");
        fprintf(stderr, "Prints results to stdout\n");
        return -1;
    }

    /* Transform the starting delimeters, so we don't have to keep subtracting 3
     * as we match. */
    for (i=0; i<4; i++) {
        ROT3_right(start_of_record[i], strlen(start_of_record[i]));
        printf("%s\n", start_of_record[i]);
    }
    fflush(stdout);

    /* On Windows, we have to remove implicit adding of '\r' to ends of
     * of line so we can add it explicitly ourselves, so that the rest 
     * of the code is the same between Linux and Windows. */
#ifdef _WIN32
    (void)_setmode(1, _O_BINARY);
#endif

    /* Process all files specified on the command-line */
    fprintf(stderr, "[+] blxtract 1.0 - extracting %d files to stdout\n", argc-1);
    for (i=1; i<argc; i++) {
        err += extract_file(argv[i], stdout);
    }

    return err;
}
