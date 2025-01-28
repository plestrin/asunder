/*
Asunder

Copyright(C) 2005 Eric Lathrop <eric@ericlathrop.com>
Copyright(C) 2007 Andrew Smith <http://littlesvr.ca/misc/contactandrew.php>

Any code in this file may be redistributed or modified under the terms of
the GNU General Public Licence as published by the Free Software
Foundation; version 2 of the licence.

*/

#define _GNU_SOURCE

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

#include "main.h"
#include "wrappers.h"
#include "threads.h"
#include "prefs.h"
#include "util.h"

pid_t cdparanoia_pid;
pid_t lame_pid;
pid_t oggenc_pid;
pid_t opusenc_pid;
pid_t flac_pid;
pid_t wavpack_pid;
pid_t monkey_pid;
pid_t musepack_pid;
pid_t aac_pid;

int numCdparanoiaFailed;
int numLameFailed;
int numOggFailed;
int numOpusFailed;
int numFlacFailed;
int numWavpackFailed;
int numMonkeyFailed;
int numMusepackFailed;
int numAacFailed;

int numCdparanoiaOk;
int numLameOk;
int numOggOk;
int numOpusOk;
int numFlacOk;
int numWavpackOk;
int numMonkeyOk;
int numMusepackOk;
int numAacOk;

int numchildren = 0;
static bool waitBeforeSigchld;

static void blockSigChld(void)
{
    sigset_t block_chld;

    sigemptyset(&block_chld);
    sigaddset(&block_chld, SIGCHLD);

    sigprocmask(SIG_BLOCK, &block_chld, NULL);

    /*!! for some reason the blocking above doesn't work, so do this for now */
    waitBeforeSigchld = true;
}

static void unblockSigChld(void)
{
    sigset_t block_chld;

    sigemptyset(&block_chld);
    sigaddset(&block_chld, SIGCHLD);

    sigprocmask(SIG_UNBLOCK, &block_chld, NULL);

    waitBeforeSigchld = false;
}

// Signal handler to find out when our child has exited.
// Do not pot any printf or syslog into here, it causes a deadlock.
//
void sigchld(int signum)
{
    int status;
    pid_t pid;

    pid = wait(&status);

    /* this is because i can't seem to be able to block sigchld: */
    while (waitBeforeSigchld)
        usleep(100);

    if (status != 0)
    {
        if (pid == cdparanoia_pid)
        {
            cdparanoia_pid = 0;
            if (global_prefs->rip_wav)
                numCdparanoiaFailed++;
        }
        else if (pid == lame_pid)
        {
            lame_pid = 0;
            numLameFailed++;
        }
        else if (pid == oggenc_pid)
        {
            oggenc_pid = 0;
            numOggFailed++;
        }
        else if (pid == opusenc_pid)
        {
            opusenc_pid = 0;
            numOpusFailed++;
        }
        else if (pid == flac_pid)
        {
            flac_pid = 0;
            numFlacFailed++;
        }
        else if (pid == wavpack_pid)
        {
            wavpack_pid = 0;
            numWavpackFailed++;
        }
        else if (pid == monkey_pid)
        {
            monkey_pid = 0;
            numMonkeyFailed++;
        }
        else if (pid == musepack_pid)
        {
            musepack_pid = 0;
            numMusepackFailed++;
        }
        else if (pid == aac_pid)
        {
            aac_pid = 0;
            numAacFailed++;
        }
    }
    else
    {
        if (pid == cdparanoia_pid)
        {
            cdparanoia_pid = 0;
            if (global_prefs->rip_wav)
                numCdparanoiaOk++;
        }
        else if (pid == lame_pid)
        {
            lame_pid = 0;
            numLameOk++;
        }
        else if (pid == oggenc_pid)
        {
            oggenc_pid = 0;
            numOggOk++;
        }
        else if (pid == opusenc_pid)
        {
            opusenc_pid = 0;
            numOpusOk++;
        }
        else if (pid == flac_pid)
        {
            flac_pid = 0;
            numFlacOk++;
        }
        else if (pid == wavpack_pid)
        {
            wavpack_pid = 0;
            numWavpackOk++;
        }
        else if (pid == monkey_pid)
        {
            monkey_pid = 0;
            numMonkeyOk++;
        }
        else if (pid == musepack_pid)
        {
            musepack_pid = 0;
            numMusepackOk++;
        }
        else if (pid == aac_pid)
        {
            aac_pid = 0;
            numAacOk++;
        }
    }
}

// fork() and exec() the file listed in "args"
//
// args - a valid array for execvp()
// toread - the file descriptor to pipe back to the parent
// p - a place to write the PID of the exec'ed process
//
// returns - a file descriptor that reads whatever the program outputs on "toread"
static int exec_with_output(const char * args[], int toread, pid_t * p)
{
    char logStr[1024];
    int pipefd[2];

    blockSigChld();

    if (pipe(pipefd) != 0)
        fatalError("exec_with_output(): failed to create a pipe");

    if ((*p = fork()) == 0)
    {
        // im the child
        // i get to execute the command

        if (gbl_null_fd != -1)
        {
            dup2(gbl_null_fd, STDIN_FILENO);
            dup2(gbl_null_fd, STDOUT_FILENO);
            dup2(gbl_null_fd, STDERR_FILENO);
        }

        // setup output
        dup2(pipefd[1], toread);

        // close all other file descriptor
        int fdlimit = (int)sysconf(_SC_OPEN_MAX);
        for (int i = 0; i < fdlimit; i++) {
            if (i != STDIN_FILENO && i != STDOUT_FILENO && i != STDERR_FILENO) {
                close(i);
            }
        }

        // call execvp
        execvp(args[0], (char **)args);

        // should never get here
        fatalError("exec_with_output(): execvp() failed");
    }

    int count;
    snprintf(logStr, sizeof logStr, "%d started:", *p);
    for (count = 0; args[count] != NULL; count++)
    {
        if (strlen(logStr) + 1 + strlen(args[count]) < sizeof logStr)
        {
            strcat(logStr, " ");
            strcat(logStr, args[count]);
        }
    }
    debugLog("%s", logStr);

    // i'm the parent, get ready to wait for children
    numchildren++;

    // close the side of the pipe we don't need
    close(pipefd[1]);

    unblockSigChld();

    return pipefd[0];
}

/* uses cdparanoia to rip a WAV track from a cdrom
 *
 * cdrom     - the path to the cdrom device
 * track_num - the track to rip
 * filename  - the name of the output WAV file
 * progress  - the percent done
 */

void cdparanoia(const char* cdrom, int track_num, const char * filename, double* progress){
    const char* args[8];
    int pos;
    int fd;
    char buf[256];
    int size;

    int start;
    int end;
    int code;
    char type[200];
    int sector;

    char track_str[4];

    snprintf(track_str, sizeof(track_str), "%d", track_num);

    pos = 0;
    args[pos++] = "cdparanoia";
    if (global_prefs->do_fast_rip){
        args[pos++] = "-Z";
    }
    args[pos++] = "-e";
    args[pos++] = "-d";
    args[pos++] = cdrom;
    args[pos++] = track_str;
    args[pos++] = filename;
    args[pos++] = NULL;

    fd = exec_with_output(args, STDERR_FILENO, &cdparanoia_pid);

    // to convert the progress number stat cdparanoia spits out
    // into sector numbers divide by 1176
    // note: only use the "[wrote]" numbers
    do
    {
        pos = -1;
        bool interrupted;
        do
        {
            interrupted = FALSE;

            pos++;
            size = read(fd, buf + pos, 1);

            if (size == -1 && errno == EINTR)
            /* signal interrupted read(), try again */
            {
                pos--;
                debugLog("cdparanoia() interrupted")
                interrupted = TRUE;
            }
        } while ((size > 0 && pos < 255 && buf[pos] != '\n') || interrupted);
        buf[pos] = '\0';

        if ((buf[0] == 'R') && (buf[1] == 'i'))
        {
            sscanf(buf, "Ripping from sector %d", &start);
        } else if (buf[0] == '\t') {
            sscanf(buf, "\t to sector %d", &end);
        } else if (buf[0] == '#') {
            sscanf(buf, "##: %d %s @ %d", &code, type, &sector);
            sector /= 1176;
            if (strncmp("[wrote]", type, 7) == 0)
            {
                *progress = (double)(sector-start)/(end-start);
            }
        }
    } while (size > 0);

    debugLog("Ripping %s finished\n", track_str)

    close(fd);
    /* don't go on until the signal for the previous call is handled */
    while (cdparanoia_pid != 0)
    {
        debugLog("w3\n")
        usleep(100000);
    }
}

void mp3_enc(int tracknum, char* artist, char* album, char* title, char* year, char* genre, char* wavfilename, char* file_name, int vbr, int bitrate, double* progress){
    int fd;

    char buf[256];
    int size;
    int pos;

    int sector;
    int end;

    char tracknum_text[4];
    char bitrate_text[4];
    const char * args[19];

    snprintf(tracknum_text, 4, "%d", tracknum);

    pos = 0;
    args[pos++] = "lame";
    if (vbr)
    {
        args[pos++] = "-V";
        snprintf(bitrate_text, 4, "%d", int_to_vbr_int(bitrate));
    } else {
        args[pos++] = "-b";
        snprintf(bitrate_text, 4, "%d", int_to_bitrate(bitrate, vbr));
    }
    args[pos++] = bitrate_text;
    args[pos++] = "--id3v2-only";
    if ((tracknum > 0) && (tracknum < 100))
    {
        args[pos++] = "--tn";
        args[pos++] = tracknum_text;
    }
    if ((artist != NULL) && (strlen(artist) > 0))
    {
        args[pos++] = "--ta";
        args[pos++] = artist;
    }
    if ((album != NULL) && (strlen(album) > 0))
    {
        args[pos++] = "--tl";
        args[pos++] = album;
    }
    if ((title != NULL) && (strlen(title) > 0))
    {
        args[pos++] = "--tt";
        args[pos++] = title;
    }

    // lame refuses to accept some genres that come from cddb, and users get upset
    // No longer an issue - users can now edit the genre field -lnr

    // if (false && (genre != NULL) && (strlen(genre) > 0))
    if (( genre != NULL )								// lnr
    &&  ( strlen( genre )))
    {
        args[pos++] = "--tg";
        args[pos++] = genre;
    }

    if ((year != NULL) && (strlen(year) > 0))
    {
        args[pos++] = "--ty";
        args[pos++] = year;
    }
    args[pos++] = wavfilename;
    args[pos++] = file_name;
    args[pos++] = NULL;

    fd = exec_with_output(args, STDERR_FILENO, &lame_pid);

    do
    {
        pos = -1;
        bool interrupted;
        do
        {
            interrupted = FALSE;

            pos++;
            size = read(fd, &buf[pos], 1);

            if (size == -1 && errno == EINTR)
            /* signal interrupted read(), try again */
            {
                size = 1;
                debugLog("lame() interrupted");
                interrupted = TRUE;
            }

        } while ((size > 0 && pos < 255 && buf[pos] != '\r' && buf[pos] != '\n') || interrupted);
        buf[pos] = '\0';

        if (sscanf(buf, "%d/%d", &sector, &end) == 2)
        {
            *progress = (double)sector/end;
        }
    } while (size > 0);

    close(fd);
    /* don't go on until the signal for the previous call is handled */
    while (lame_pid != 0)
    {
        debugLog("w4\n");
        usleep(100000);
    }

}

void ogg_enc(int tracknum, char* artist, char* album, char* title, char* year, char* genre, char* wavfilename, char* file_name, int quality_level, double* progress){
    int fd;

    char buf[256];
    int size;
    int pos;

    int sector;
    int end;

    char tracknum_text[4];
    char quality_level_text[3];
    const char * args[19];

    snprintf(tracknum_text, 4, "%d", tracknum);
    snprintf(quality_level_text, 3, "%d", quality_level);

    pos = 0;
    args[pos++] = "oggenc";
    args[pos++] = "-q";
    args[pos++] = quality_level_text;

    if ((tracknum > 0) && (tracknum < 100))
    {
        args[pos++] = "-N";
        args[pos++] = tracknum_text;
    }
    if ((artist != NULL) && (strlen(artist) > 0))
    {
        args[pos++] = "-a";
        args[pos++] = artist;
    }
    if ((album != NULL) && (strlen(album) > 0))
    {
        args[pos++] = "-l";
        args[pos++] = album;
    }
    if ((title != NULL) && (strlen(title) > 0))
    {
        args[pos++] = "-t";
        args[pos++] = title;
    }
    if ((year != NULL) && (strlen(year) > 0))
    {
        args[pos++] = "-d";
        args[pos++] = year;
    }
    if ((genre != NULL) && (strlen(genre) > 0))
    {
        args[pos++] = "-G";
        args[pos++] = genre;
    }
    args[pos++] = wavfilename;
    args[pos++] = "-o";
    args[pos++] = file_name;
    args[pos++] = NULL;

    fd = exec_with_output(args, STDERR_FILENO, &oggenc_pid);

    do {
        pos = -1;
        bool interrupted;
        do {
            interrupted = FALSE;

            pos++;
            size = read(fd, &buf[pos], 1);

            if (size == -1 && errno == EINTR)
            /* signal interrupted read(), try again */
            {
                pos--;
                debugLog("oggenc() interrupted")
                interrupted = TRUE;
            }

        } while ((size > 0 && pos < 255 && buf[pos] != '\r' && buf[pos] != '\n') || interrupted);
        buf[pos] = '\0';

        if (sscanf(buf, "\t[\t%d.%d%%]", &sector, &end) == 2)
        {
            *progress = (double)(sector + (end*0.1))/100;
        }
        else if (sscanf(buf, "\t[\t%d,%d%%]", &sector, &end) == 2)
        {
            *progress = (double)(sector + (end*0.1))/100;
        }
    } while (size > 0);

    close(fd);
    /* don't go on until the signal for the previous call is handled */
    while (oggenc_pid != 0)
    {
        debugLog("w6\n")
        usleep(100000);
    }
}

void opus_enc(int tracknum, char* artist, char* album, char* title, char* year, char* genre, char* wavfilename, char* file_name, int bitrate){
    int fd;

    int pos;
    char bitrate_text[4];
    char tracknum_text[16];
    char album_text[128];
    char year_text[32];
    char genre_text[64];
    const char * args[19];

    snprintf(bitrate_text, 4, "%d", int_to_bitrate(bitrate,FALSE));

    pos = 0;
    args[pos++] = "opusenc";
    args[pos++] = "--bitrate";
    args[pos++] = bitrate_text;

    if ((tracknum > 0) && (tracknum < 100))
    {
        snprintf(tracknum_text,16,"TRACKNUMBER=%d",tracknum);
        args[pos++] = "--comment";
        args[pos++] = tracknum_text;
    }
    if ((artist != NULL) && (strlen(artist) > 0))
    {
        args[pos++] = "--artist";
        args[pos++] = artist;
    }
    if ((album != NULL) && (strlen(album) > 0))
    {
        snprintf(album_text,128,"ALBUM=%s",album);
        args[pos++] = "--comment";
        args[pos++] = album_text;
    }
    if ((title != NULL) && (strlen(title) > 0))
    {
        args[pos++] = "--title";
        args[pos++] = title;
    }
    if ((year != NULL) && (strlen(year) > 0))
    {
        snprintf(year_text,32,"DATE=%s",year);
        args[pos++] = "--comment";
        args[pos++] = year_text;
    }
    if ((genre != NULL) && (strlen(genre) > 0))
    {
        snprintf(genre_text,64,"GENRE=%s",genre);
        args[pos++] = "--comment";
        args[pos++] = genre_text;
    }
    args[pos++] = wavfilename;
    args[pos++] = file_name;
    args[pos++] = NULL;

    fd = exec_with_output(args, STDERR_FILENO, &oggenc_pid);

    int opussize;
    char opusbuf[256];
    do
    {
        /* The opus encoder doesn't give me an estimate for completion
        * or any way to estimate it myself, just the number of seconds
        * done. So just sit in here until the program exits */
        opussize = read(fd, &opusbuf[0], 256);

        if (opussize == -1 && errno == EINTR)
        /* signal interrupted read(), try again */
            opussize = 1;
    } while (opussize > 0);

    close(fd);
    /* don't go on until the signal for the previous call is handled */
    while (oggenc_pid != 0)
    {
        debugLog("w12\n");
        usleep(100000);
    }
}

void flac_enc(int tracknum, char* artist, char* album, char* title, char* year, char* genre, char* wavfilename, char* file_name, int compression_level, double* progress){
    int fd;

    char buf[256];
    int size;
    int pos;

    int sector;

    char tracknum_text[16];
    char * artist_text = NULL;
    char * album_text = NULL;
    char * title_text = NULL;
    char * genre_text = NULL;
    char * year_text = NULL;
    char compression_level_text[3];
    const char * args[19];

    snprintf(tracknum_text, 16, "TRACKNUMBER=%d", tracknum);

    if (artist != NULL)
    {
        artist_text = malloc(strlen(artist) + 8);
        if (artist_text == NULL)
            fatalError("malloc(sizeof(char) * (strlen(artist)+8)) failed. Out of memory.");
        snprintf(artist_text, strlen(artist) + 8, "ARTIST=%s", artist);
    }

    if (album != NULL)
    {
        album_text = malloc(strlen(album) + 7);
        if (album_text == NULL)
            fatalError("malloc(sizeof(char) * (strlen(album)+7)) failed. Out of memory.");
        snprintf(album_text, strlen(album) + 7, "ALBUM=%s", album);
    }

    if (title != NULL)
    {
        title_text = malloc(strlen(title) + 7);
        if (title_text == NULL)
            fatalError("malloc(sizeof(char) * (strlen(title)+7) failed. Out of memory.");
        snprintf(title_text, strlen(title) + 7, "TITLE=%s", title);
    }

    if (genre != NULL)
    {
        genre_text = malloc(strlen(genre) + 7);
        if (genre_text == NULL)
            fatalError("malloc(sizeof(char) * (strlen(genre)+7) failed. Out of memory.");
        snprintf(genre_text, strlen(genre) + 7, "GENRE=%s", genre);
    }

    if (year != NULL)
    {
        year_text = malloc(strlen(year) + 6);
        if (year_text == NULL)
            fatalError("malloc(sizeof(char) * (strlen(year)+6) failed. Out of memory.");
        snprintf(year_text, strlen(year) + 6, "DATE=%s", year);
    }

    snprintf(compression_level_text, 3, "-%d", compression_level);

    pos = 0;
    args[pos++] = "flac";
    args[pos++] = "-f";
    args[pos++] = compression_level_text;
    if ((tracknum > 0) && (tracknum < 100))
    {
        args[pos++] = "-T";
        args[pos++] = tracknum_text;
    }
    if ((artist != NULL) && (strlen(artist) > 0))
    {
        args[pos++] = "-T";
        args[pos++] = artist_text;
    }
    if ((album != NULL) && (strlen(album) > 0))
    {
        args[pos++] = "-T";
        args[pos++] = album_text;
    }
    if ((title != NULL) && (strlen(title) > 0))
    {
        args[pos++] = "-T";
        args[pos++] = title_text;
    }
    if ((genre != NULL) && (strlen(genre) > 0))
    {
        args[pos++] = "-T";
        args[pos++] = genre_text;
    }
    if ((year != NULL) && (strlen(year) > 0))
    {
        args[pos++] = "-T";
        args[pos++] = year_text;
    }
    args[pos++] = wavfilename;
    args[pos++] = "-o";
    args[pos++] = file_name;
    args[pos++] = NULL;

    fd = exec_with_output(args, STDERR_FILENO, &flac_pid);

    free(artist_text);
    free(album_text);
    free(title_text);
    free(genre_text);
    free(year_text);

    do{
        pos = -1;
        bool interrupted;
        do
        {
            interrupted = FALSE;

            pos++;
            size = read(fd, &buf[pos], 1);

            if (size == -1 && errno == EINTR)
            /* signal interrupted read(), try again */
            {
                pos--;
                debugLog("flac() interrupted");
                interrupted = TRUE;
            }

        } while ((size > 0 && pos < 255 && buf[pos] != '\r' && buf[pos] != '\n') || interrupted);
        buf[pos] = '\0';

        for (; pos>0; pos--)
        {
            if (buf[pos] == ':')
            {
                pos++;
                break;
            }
        }

        if (sscanf(&buf[pos], "%d%%", &sector) == 1)
        {
            *progress = (double)sector/100;
        }
    } while (size > 0);

    close(fd);

    /* don't go on until the signal for the previous call is handled */
    while (flac_pid != 0)
    {
        debugLog("w7\n");
        usleep(100000);
    }
}

void wavpack_enc(char* wavfilename, int compression, bool hybrid, int bitrate, double* progress){
    const char* args[10];
    int fd;
    int pos;
    int size;
    char buf[256];
    char bitrateTxt[7];

    pos = 0;
    args[pos++] = "wavpack";

    if (hybrid)
    {
        snprintf(bitrateTxt, sizeof bitrateTxt, "-b%d", bitrate);
        args[pos++] = bitrateTxt;

        args[pos++] = "-c";
    }

    args[pos++] = "-y";
    if (compression == 0)
        args[pos++] = "-f";
    else if (compression == 2)
        args[pos++] = "-h";
    else if (compression == 3)
        args[pos++] = "-hh";
    // default is no parameter (normal compression)

    args[pos++] = "-x3";

    args[pos++] = wavfilename;
    args[pos++] = NULL;

    fd = exec_with_output(args, STDERR_FILENO, &wavpack_pid);

    do
    {
        pos = -1;
        bool interrupted;
        do
        {
            interrupted = FALSE;

            pos++;
            size = read(fd, &buf[pos], 1);

            if (size == -1 && errno == EINTR)
            /* signal interrupted read(), try again */
            {
                pos--;
                debugLog("wavpack() interrupted");
                interrupted = TRUE;
            }
        } while ((size > 0 && pos < 255 && buf[pos] != '\b') || interrupted);

        buf[pos] = '\0';

        for (; pos>0; pos--)
        {
            if (buf[pos] == ',')
            {
                pos++;
                break;
            }
        }

        int percent;
        /* That extra first condition is because wavpack ends encoding with a
        * line like this:
        * created 01 - Unknown Artist - Track 1.wv (+.wvc) in 50.22 secs (lossless, 73.91%)
        */
        if (buf[strlen(buf) - 1] != ')' && sscanf(&buf[pos], "%d%%", &percent) == 1)
        {
            *progress = (double)percent/100;
            //!! This was commented out, possibly because the last line is in some
            // different format than the normal progress
            //~ fprintf(stderr, "line '%s' percent %.2lf\n", &buf[pos], *progress);
        }
    } while (size > 0);

    close(fd);
    /* don't go on until the signal for the previous call is handled */
    while (wavpack_pid != 0)
    {
        debugLog("w8\n");
        usleep(100000);
    }
}

void monkey_enc(char* wavfilename, char* file_name, int compression, double* progress){
    const char* args[5];
    int fd;
    int pos;

    pos = 0;
    args[pos++] = "mac";
    args[pos++] = wavfilename;
    args[pos++] = file_name;

    char compressParam[10];
    snprintf(compressParam, 10, "-c%d", compression);
    args[pos++] = compressParam;

    args[pos++] = NULL;

    fd = exec_with_output(args, STDERR_FILENO, &monkey_pid);

    int size;
    char buf[256];
    do
    {
        pos = -1;
        do
        {
            pos++;
            size = read(fd, &buf[pos], 1);

            if (size == -1 && errno == EINTR)
            /* signal interrupted read(), try again */
            {
                pos--;
                size = 1;
            }

        } while ((buf[pos] != '\r') && (buf[pos] != '\n') && (size > 0) && (pos < 255));
        buf[pos] = '\0';

        double percent;
        if (sscanf(buf, "Progress: %lf", &percent) == 1)
        {
            *progress = percent / 100;
        }
    } while (size > 0);

    close(fd);
    /* don't go on until the signal for the previous call is handled */
    while (monkey_pid != 0)
    {
        debugLog("w9\n");
        usleep(100000);
    }
}

void musepack_enc(char* wavfilename, char* file_name, int quality, double* progress){
    const char* args[7];
    int fd;
    int pos;

    pos = 0;
    args[pos++] = "mpcenc";
    args[pos++] = "--overwrite";

    args[pos++] = "--quality";
    char qualityParam[6];
    snprintf(qualityParam, 6, "%d.00", quality);
    args[pos++] = qualityParam;

    args[pos++] = wavfilename;
    args[pos++] = file_name;
    args[pos++] = NULL;

    fd = exec_with_output(args, STDERR_FILENO, &musepack_pid);

    int size;
    char buf[256];
    do
    {
        pos = -1;
        bool interrupted;
        do
        {
            interrupted = FALSE;

            pos++;
            size = read(fd, &buf[pos], 1);

            if (size == -1 && errno == EINTR)
            /* signal interrupted read(), try again */
            {
                pos--;
                debugLog("musepack() interrupted");
                interrupted = TRUE;
            }

        } while ((size > 0 && pos < 255 && buf[pos] != '\r' && buf[pos] != '\n') || interrupted);
        buf[pos] = '\0';

        double percent;
        if (sscanf(buf, " %lf", &percent) == 1)
        {
            *progress = percent / 100;
        }
    } while (size > 0);

    close(fd);
    /* don't go on until the signal for the previous call is handled */
    while (musepack_pid != 0)
    {
        debugLog("w10\n");
        usleep(100000);
    }
}

void aac_enc(int tracknum, char* artist, char* album, char* title, char* year, char* genre, char* wavfilename, char* file_name, int quality){
    const char* args[9];
    char* dynamic_args[6];
    int fd;
    int pos;
    int dyn_pos;

    pos = 0;
    args[pos++] = "neroAacEnc";

    args[pos++] = "-q";
    char qualityParam[5];
    snprintf(qualityParam, sizeof qualityParam, "0.%d", quality);
    args[pos++] = qualityParam;

    args[pos++] = "-if";
    args[pos++] = wavfilename;
    args[pos++] = "-of";
    args[pos++] = file_name;
    args[pos++] = NULL;

    fd = exec_with_output(args, STDERR_FILENO, &aac_pid);

    int size;
    char buf[256];
    do
    {
        /* The Nero encoder doesn't give me an estimate for completion
        * or any way to estimate it myself, just the number of seconds
        * done. So just sit in here until the program exits */
        size = read(fd, &buf[0], 256);

        if (size == -1 && errno == EINTR)
        /* signal interrupted read(), try again */
            size = 1;
    } while (size > 0);

    close(fd);
    /* don't go on until the signal for the previous call is handled */
    while (aac_pid != 0)
    {
        debugLog("w11\n");
        usleep(100000);
    }

    /* Now add tags to the encoded track */

    pos = 0;
    args[pos++] = "neroAacTag";

    args[pos++] = file_name;

    dyn_pos = 0;
    if (asprintf(&dynamic_args[dyn_pos], "-meta:artist=%s", artist) > 0)
    {
        args[pos++] = dynamic_args[dyn_pos++];
    }
    if (asprintf(&dynamic_args[dyn_pos], "-meta:title=%s", title) > 0)
    {
        args[pos++] = dynamic_args[dyn_pos++];
    }
    if (asprintf(&dynamic_args[dyn_pos], "-meta:album=%s", album) > 0)
    {
        args[pos++] = dynamic_args[dyn_pos++];
    }
    if (asprintf(&dynamic_args[dyn_pos], "-meta:year=%s", year) > 0)
    {
        args[pos++] = dynamic_args[dyn_pos++];
    }
    if (asprintf(&dynamic_args[dyn_pos], "-meta:genre=%s", genre) > 0)
    {
        args[pos++] = dynamic_args[dyn_pos++];
    }
    if (asprintf(&dynamic_args[dyn_pos], "-meta:track=%d", tracknum) > 0)
    {
        args[pos++] = dynamic_args[dyn_pos++];
    }

    args[pos++] = NULL;

    fd = exec_with_output(args, STDERR_FILENO, &aac_pid);

    do
    {
        /* The Nero tag writer doesn't take very long to run. Just slurp the output. */
        size = read(fd, &buf[0], 256);

        if (size == -1 && errno == EINTR)
        /* signal interrupted read(), try again */
            size = 1;
    } while (size > 0);

    close(fd);
    /* don't go on until the signal for the previous call is handled */
    while (aac_pid != 0)
    {
        debugLog("w12\n");
        usleep(100000);
    }

    while (dyn_pos)
    {
        free(dynamic_args[--dyn_pos]);
    }
}
