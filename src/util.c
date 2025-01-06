#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>

#include "util.h"

void fatalError(const char* message){
    fprintf(stderr, "Fatal error: %s\n", message);
    exit(EXIT_FAILURE);
}

int int_to_monkey_int(int i)
{
    switch (i)
    {
    case 0:
        return 1000;
    case 1:
        return 2000;
    case 2:
        return 3000;
    case 3:
        return 4000;
    case 4:
        return 5000;
    }

    fprintf(stderr, "int_to_monkey_int() called with bad parameter\n");
    return 3000;
}

int int_to_vbr_int(int i)
{
    switch (i)
    {
    case 0:
    case 1:
    case 2:
    case 3:
        return 9;
    case 4:
        return 8;
    case 5:
        return 7;
    case 6:
        return 6;
    case 7:
        return 5;
    case 8:
        return 4;
    case 9:
        return 3;
    case 10:
        return 2;
    case 11:
        return 1;
    case 12:
        return 0;
    case 13:
    case 14:
        return 0;
    }

    fprintf(stderr, "int_to_vbr_int() called with bad parameter\n");
    return 4;
}

int int_to_musepack_int(int i)
{
    switch (i)
    {
    case 0:
        return 3;
    case 1:
        return 4;
    case 2:
        return 5;
    case 3:
        return 6;
    case 4:
        return 7;
    }

    fprintf(stderr, "int_to_musepack_int() called with bad parameter\n");
    return 5;
}

// converts a gtk slider's integer range to a meaningful bitrate
//
// NOTE: i grabbed these bitrates from the list in the LAME man page
// and from http://wiki.hydrogenaudio.org/index.php?title=LAME#VBR_.28variable_bitrate.29_settings
int int_to_bitrate(int i, bool vbr)
{
    switch (i)
    {
    case 0:
        if (vbr)
            return 65;
        else
            return 32;
    case 1:
        if (vbr)
            return 65;
        else
            return 40;
    case 2:
        if (vbr)
            return 65;
        else
            return 48;
    case 3:
        if (vbr)
            return 65;
        else
            return 56;
    case 4:
        if (vbr)
            return 85;
        else
            return 64;
    case 5:
        if (vbr)
            return 100;
        else
            return 80;
    case 6:
        if (vbr)
            return 115;
        else
            return 96;
    case 7:
        if (vbr)
            return 130;
        else
            return 112;
    case 8:
        if (vbr)
            return 165;
        else
            return 128;
    case 9:
        if (vbr)
            return 175;
        else
            return 160;
    case 10:
        if (vbr)
            return 190;
        else
            return 192;
    case 11:
        if (vbr)
            return 225;
        else
            return 224;
    case 12:
        if (vbr)
            return 245;
        else
            return 256;
    case 13:
        if (vbr)
            return 245;
        else
            return 320;
    }

    fprintf(stderr, "int_to_bitrate() called with bad parameter (%d)\n", i);
    return 32;
}

int int_to_wavpack_bitrate(int i)
{
    switch (i)
    {
    case 0:
        return 196;
    case 1:
        return 256;
    case 2:
        return 320;
    case 3:
        return 384;
    case 4:
        return 448;
    case 5:
        return 512;
    case 6: // some format_wavpack_bitrate() weirdness
        return 512;
    }

    fprintf(stderr, "int_to_wavpack_bitrate() called with bad parameter (%d)\n", i);
    return 192;
}

int int_to_musepack_bitrate(int i)
{
    switch (i)
    {
    case 0:
        return 90;
    case 1:
        return 130;
    case 2:
        return 180;
    case 3:
        return 210;
    case 4:
        return 240;
    }

    fprintf(stderr, "int_to_wavpack_bitrate() called with bad parameter (%d)\n", i);
    return 90;
}

// construct a filename from various parts
//
// path - the path the file is placed in (don't include a trailing '/')
// dir - the parent directory of the file (don't include a trailing '/')
// file - the filename
// extension - the suffix of a file (don't include a leading '.')
//
// NOTE: caller must free the returned string!
// NOTE: any of the parameters may be NULL to be omitted
char* make_filename(const char* path, const char* dir, const char* file, const char* extension){
    size_t  len = 1;
    char*   ret;
    int     pos = 0;

    if (path != NULL){
        len += strlen(path) + 1;
    }
    if (dir != NULL){
        len += strlen(dir) + 1;
    }
    if (file != NULL){
        len += strlen(file);
    }
    if (extension != NULL){
        len += strlen(extension) + 1;
    }

    if ((ret = malloc(sizeof(char) * len)) == NULL){
        fatalError("malloc(sizeof(char) * len) failed. Out of memory.");
    }

    if (path != NULL){
        strcpy(ret + pos, path);
        pos += strlen(path);
        ret[pos ++] = '/';
    }
    if (dir != NULL){
        strcpy(ret + pos, dir);
        pos += strlen(dir);
        ret[pos ++] = '/';
    }
    if (file != NULL){
        strcpy(ret + pos, file);
        pos += strlen(file);
    }
    if (extension != NULL){
        ret[pos ++] = '.';
        strcpy(ret + pos, extension);
        pos += strlen(extension);
    }
    ret[pos] = '\0';

    return ret;
}

// substitute various items into a formatted string (similar to printf)
//
// format - the format of the filename
// tracknum - gets substituted for %N in format
// year - gets substituted for %Y in format
// artist - gets substituted for %A in format
// album - gets substituted for %L in format
// title - gets substituted for %T in format
//
// NOTE: caller must free the returned string!
char* parse_format(const char* format, int tracknum, const char* year, const char* artist,
                    const char* album, const char* genre, const char* title)
{
    size_t      i;
    int         len = 1;
    char*       ret;
    int         pos = 0;

    for (i = 0; i < strlen(format); i++){
        if ((format[i] == '%') && (i + 1 < strlen(format))){
            switch (format[i+1]){
                case 'A':
                    if (artist) len += strlen(artist);
                    break;
                case 'L':
                    if (album) len += strlen(album);
                    break;
                case 'N':
                    if ((tracknum > 0) && (tracknum < 100)) len += 2;
                    break;
                case 'Y':
                    if (year) len += strlen(year);
                    break;
                case 'T':
                    if (title) len += strlen(title);
                    break;
                case 'G':
                    if (genre) len += strlen(genre);
                    break;
                case '%':
                    len ++;
                    break;
            }

            i ++; // skip the character after the %
        }
        else{
            len ++;
        }
    }

    if ((ret = malloc(sizeof(char) * len)) == NULL){
        fatalError("malloc(sizeof(char) * (len+1)) failed. Out of memory.");
    }

    for (i = 0; i < strlen(format); i++){
        if ((format[i] == '%') && (i+1 < strlen(format))){
            switch (format[i+1]){
                case 'A':
                    if (artist != NULL){
                        strcpy(ret + pos, artist);
                        pos += strlen(artist);
                    }
                    break;
                case 'L':
                    if (album != NULL){
                        strcpy(ret + pos, album);
                        pos += strlen(album);
                    }
                    break;
                case 'N':
                    if ((tracknum > 0) && (tracknum < 100)){
                        ret[pos ++] = '0' + (tracknum / 10);
                        ret[pos ++] = '0' + (tracknum % 10);
                    }
                    break;
                case 'Y':
                    if (year != NULL){
                        strcpy(ret + pos, year);
                        pos += strlen(year);
                    }
                    break;
                case 'T':
                    if (title != NULL){
                        strcpy(ret + pos, title);
                        pos += strlen(title);
                    }
                    break;
                case 'G':
                    if (genre != NULL){
                        strcpy(ret + pos, genre);
                        pos += strlen(genre);
                    }
                    break;
                case '%':
                    ret[pos ++] = '%';
            }

            i ++; // skip the character after the %
        }
        else{
            ret[pos ++] = format[i];
        }
    }
    ret[pos] = '\0';

    return ret;
}

int program_exists(const char* name){
    unsigned    i;
    unsigned    numpaths = 1;
    char*       path;
    char*       strings;
    char**      paths;
    struct stat s;
    int         ret = 0;
    char*       filename;
    size_t      length;

    path = getenv("PATH");
    if ((strings = strdup(path)) == NULL){
        fatalError("strdup failed. Out of memory.");
    }

    length = strlen(strings);

    for (i = 0; i < length; i++){
        if (strings[i] == ':'){
            numpaths++;
        }
    }

    if ((paths = malloc(sizeof(char *) * numpaths)) == NULL){
        fatalError("malloc(sizeof(char *) * numpaths) failed. Out of memory.");
    }

    numpaths = 1;
    paths[0] = strings;
    for (i = 0; i < length; i++){
        if (strings[i] == ':'){
            strings[i] = '\0';
            paths[numpaths ++] = strings + i + 1;
        }
    }

    for (i = 0; i < numpaths; i++){
        filename = make_filename(paths[i], NULL, name, NULL);
        if (stat(filename, &s) == 0){
            ret = 1;
            free(filename);
            break;
        }
        free(filename);
    }

    free(strings);
    free(paths);

    return ret;
}

// reads an entire line from a file and returns it
//
// NOTE: caller must free the returned string!
char * read_line(int fd)
{
    int pos = 0;
    char cur;
    char * ret;
    int rc;

    do
    {
        rc = read(fd, &cur, 1);
        if (rc != 1)
        {
            // If a read fails before a newline, the file is corrupt
            // or it's from an old version.
            pos = 0;
            break;
        }
        pos++;
    } while (cur != '\n');

    if (pos == 0)
        return NULL;

    ret = malloc(sizeof(char) * pos);
    if (ret == NULL)
        fatalError("malloc(sizeof(char) * pos) failed. Out of memory.");

    lseek(fd, -pos, SEEK_CUR);
    rc = read(fd, ret, pos);
    if (rc != pos)
    {
        free(ret);
        return NULL;
    }
    ret[pos-1] = '\0';

    return ret;
}

// reads an entire line from a file and turns it into a number
int read_line_num(int fd)
{
    long ret = 0;

    char * line = read_line(fd);
    if (line == NULL)
        return 0;

    ret = strtol(line, NULL, 10);
    if (ret == LONG_MIN || ret == LONG_MAX)
        ret = 0;

    free(line);

    return ret;
}

// Uses mkdir() for every component of the path
// and returns if any of those fails with anything other than EEXIST.
int recursive_mkdir(char* pathAndName, mode_t mode)
{
    int count;
    int pathAndNameLen = strlen(pathAndName);
    int rc;
    char charReplaced;

    for (count = 0; count < pathAndNameLen; count++)
    {
        if (pathAndName[count] == '/')
        {
            charReplaced = pathAndName[count + 1];
            pathAndName[count + 1] = '\0';

            rc = mkdir(pathAndName, mode);

            pathAndName[count + 1] = charReplaced;

            if (rc != 0 && !(errno == EEXIST || errno == EISDIR))
                return rc;
        }
    }

    // in case the path doesn't have a trailing slash:
    return mkdir(pathAndName, mode);
}

// Uses mkdir() for every component of the path except the last one,
// and returns if any of those fails with anything other than EEXIST.
int recursive_parent_mkdir(char* pathAndName, mode_t mode)
{
    int count;
    bool haveComponent = false;
    int rc = 1; // guaranteed fail unless mkdir is called

    // find the last component and cut it off
    for (count = strlen(pathAndName) - 1; count >= 0; count--)
    {
        if (pathAndName[count] != '/')
            haveComponent = true;

        if (pathAndName[count] == '/' && haveComponent)
        {
            pathAndName[count] = 0;
            rc = mkdir(pathAndName, mode);
            pathAndName[count] = '/';
        }
    }

    return rc;
}

void trim_chars(char* str, const char* bad){
    size_t i;
    size_t j;
    size_t k;

    for (j = 0; bad[j]; j ++){
        for (i = 0, k = 0; str[i]; i ++){
            if (str[i] != bad[j]){
                str[k ++] = str[i];
            }
        }
        str[k] = 0;
    }
}

void trim_whitespace(char* str){
    size_t i;
    size_t j;

    for (i = 0, j = 0; str[i]; i ++){
        if (!isspace(str[i]) || j){
            str[j ++] = str[i];
        }
    }

    str[j] = 0;

    for (; j; ){
        j --;
        if (!isspace(str[j])){
            break;
        }
        str[j] = 0;
    }
}
