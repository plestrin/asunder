#include <stdbool.h>
#include <syslog.h>

#define debugLog(...) if (global_prefs->do_log){syslog(LOG_USER|LOG_INFO, __VA_ARGS__);}

void fatalError(const char* message) __attribute__ ((noreturn));

int int_to_monkey_int(int i);
int int_to_vbr_int(int i);
int int_to_bitrate(int i, bool vbr);
int int_to_wavpack_bitrate(int i);
int int_to_musepack_bitrate(int i);
int int_to_musepack_int(int i);

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
char * parse_format(const char* format, int tracknum, const char* year, const char* artist,
                    const char* album, const char* genre, const char* title);

// construct a filename from various parts
//
// path - the path the file is placed in (don't include a trailing '/')
// dir - the parent directory of the file (don't include a trailing '/')
// file - the filename
// extension - the suffix of a file (don't include a leading '.')
//
// NOTE: caller must free the returned string!
// NOTE: any of the parameters may be NULL to be omitted
char * make_filename(const char * path, const char * dir, const char * file, const char * extension);

// reads an entire line from a file and returns it
//
// NOTE: caller must free the returned string!
char * read_line(int fd);

// reads an entire line from a file and turns it into a number
int read_line_num(int fd);

int recursive_mkdir(char* pathAndName, mode_t mode);

int recursive_parent_mkdir(char* pathAndName, mode_t mode);

// searches $PATH for the named program
// returns 1 if found, 0 otherwise
int program_exists(const char * name);

void trim_chars(char* str, const char* bad);
void trim_whitespace(char* str);

// LNR - It's possible that some files may end up on a MS file system,
// so it's best to disallow MS invalid chars as well. I also disallow
// period (dot) because it screws up my file name database software. YMMV
// 13may2013: removed '.' from the list, it's a valid character.
#define	BADCHARS	"/?*|><:\"\\"
