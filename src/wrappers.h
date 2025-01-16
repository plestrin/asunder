#ifndef WRAPPERS_H
#define WRAPPERS_H

#include <sys/types.h>
#include <stdbool.h>

extern pid_t cdparanoia_pid;
extern pid_t lame_pid;
extern pid_t oggenc_pid;
extern pid_t opusenc_pid;
extern pid_t flac_pid;
extern pid_t wavpack_pid;
extern pid_t monkey_pid;
extern pid_t musepack_pid;
extern pid_t aac_pid;

extern int numCdparanoiaFailed;
extern int numLameFailed;
extern int numOggFailed;
extern int numOpusFailed;
extern int numFlacFailed;
extern int numWavpackFailed;
extern int numMonkeyFailed;
extern int numMusepackFailed;
extern int numAacFailed;

extern int numCdparanoiaOk;
extern int numLameOk;
extern int numOggOk;
extern int numOpusOk;
extern int numFlacOk;
extern int numWavpackOk;
extern int numMonkeyOk;
extern int numMusepackOk;
extern int numAacOk;

// signal handler to find out when out child has exited
void sigchld(int signum);

// uses cdparanoia to rip a WAV track from a cdrom
//
// cdrom - the path to the cdrom device
// track_num - the track to rip
// filename - the name of the output WAV file
// progress - the percent done
void cdparanoia(const char *cdrom, int track_num, const char *filename, double *progress);

void mp3_enc(int tracknum, char *artist, char *album, char *title, char *year, char *genre,
			 char *wavfilename, char *file_name, int vbr, int bitrate, double *progress);
void ogg_enc(int tracknum, char *artist, char *album, char *title, char *year, char *genre,
			 char *wavfilename, char *file_name, int quality_level, double *progress);
void opus_enc(int tracknum, char *artist, char *album, char *title, char *year, char *genre,
			  char *wavfilename, char *file_name, int bitrate);
void flac_enc(int tracknum, char *artist, char *album, char *title, char *year, char *genre,
			  char *wavfilename, char *file_name, int compression_level, double *progress);
void wavpack_enc(char *wavfilename, int compression, bool hybrid, int bitrate, double *progress);
void monkey_enc(char *wavfilename, char *file_name, int compression, double *progress);
void musepack_enc(char *wavfilename, char *file_name, int quality, double *progress);
void aac_enc(int tracknum, char *artist, char *album, char *title, char *year, char *genre,
			 char *wavfilename, char *file_name, int quality);

#endif
