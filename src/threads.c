#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <libgen.h>

#include "threads.h"
#include "main.h"
#include "prefs.h"
#include "util.h"
#include "wrappers.h"
#include "support.h"
#include "interface.h"
#include "completion.h"

/* TODO
- clean the thread structure with g_thread_unref()
- clean the album_meta variable
- move playlist file to encode thread
*/

static GMutex barrier;
static GCond available;
static unsigned int counter;

/* ripping or encoding, so that can know not to clear the tracklist on eject */
bool working;
/* for canceling */
bool aborted;
/* for stopping the tracking thread */
static bool allDone;

static GThread *ripper;
static GThread *encoder;
static GThread *tracker;

static unsigned int tracks_to_rip;
static double rip_percent;
static double mp3_percent;
static double ogg_percent;
static double flac_percent;
static double wavpack_percent;
static double monkey_percent;
static double musepack_percent;
static unsigned int rip_tracks_completed;
static unsigned int encode_tracks_completed;

static gpointer encode(gpointer data);
static gpointer rip(gpointer data);
static gpointer track(gpointer data);

// aborts ripping- stops all the threads and return to normal execution
void abort_threads(void)
{
	aborted = true;

	if (cdparanoia_pid != 0)
		kill(cdparanoia_pid, SIGKILL);
	if (lame_pid != 0)
		kill(lame_pid, SIGKILL);
	if (oggenc_pid != 0)
		kill(oggenc_pid, SIGKILL);
	if (opusenc_pid != 0)
		kill(opusenc_pid, SIGKILL);
	if (flac_pid != 0)
		kill(flac_pid, SIGKILL);
	if (wavpack_pid != 0)
		kill(wavpack_pid, SIGKILL);
	if (monkey_pid != 0)
		kill(monkey_pid, SIGKILL);
	if (musepack_pid != 0)
		kill(musepack_pid, SIGKILL);
	if (aac_pid != 0)
		kill(aac_pid, SIGKILL);

	/* wait until all the worker threads are done */
	while (cdparanoia_pid | lame_pid | oggenc_pid | opusenc_pid | flac_pid | wavpack_pid |
		   monkey_pid | musepack_pid | aac_pid) {
		usleep(100000);
	}

	g_cond_broadcast(&available);

	debugLog("Aborting: 1\n");
	g_thread_join(ripper);
	debugLog("Aborting: 2\n");
	g_thread_join(encoder);
	debugLog("Aborting: 3\n");
	g_thread_join(tracker);
	debugLog("Aborting: 4 (All threads joined)\n");

	gtk_window_set_title(GTK_WINDOW(win_main), "Asunder");

	gtk_widget_hide(win_ripping);
	gdk_flush();
	working = false;

	show_completed_dialog(numCdparanoiaOk + numLameOk + numOggOk + numOpusOk + numFlacOk +
							  numWavpackOk + numMonkeyOk + numMusepackOk + numAacOk,
						  numCdparanoiaFailed + numLameFailed + numOggFailed + numOpusFailed +
							  numFlacFailed + numWavpackFailed + numMonkeyFailed +
							  numMusepackFailed + numAacFailed);
}

struct trackMeta {
	int num_src;
	int num_dst;
	unsigned int time;
	char *artist;
	char *title;
	char *rip_name;
	struct trackMeta *next;
};

#define trackMeta_delete(track_meta) \
	trackMeta_clean(track_meta);     \
	free(track_meta);

static void trackMeta_clean(struct trackMeta *track_meta)
{
	if (track_meta->artist != NULL) {
		free(track_meta->artist);
		track_meta->artist = NULL;
	}
	if (track_meta->title != NULL) {
		free(track_meta->title);
		track_meta->title = NULL;
	}
	if (track_meta->rip_name != NULL) {
		free(track_meta->rip_name);
		track_meta->rip_name = NULL;
	}
	if (track_meta->next != NULL) {
		trackMeta_delete(track_meta->next);
		track_meta->next = NULL;
	}
}

struct albumMeta {
	char *artist;
	char *title;
	char *genre;
	char *year;
	char *directory;
	int single_artist;
	struct trackMeta *track_meta;
};

#define albumMeta_delete(album_meta) \
	albumMeta_clean(album_meta);     \
	free(album_meta);

static void albumMeta_clean(struct albumMeta *album_meta)
{
	if (album_meta->artist != NULL) {
		free(album_meta->artist);
		album_meta->artist = NULL;
	}
	if (album_meta->title != NULL) {
		free(album_meta->title);
		album_meta->title = NULL;
	}
	if (album_meta->genre != NULL) {
		free(album_meta->genre);
		album_meta->genre = NULL;
	}
	if (album_meta->year != NULL) {
		free(album_meta->year);
		album_meta->year = NULL;
	}
	if (album_meta->directory != NULL) {
		free(album_meta->directory);
		album_meta->directory = NULL;
	}
	if (album_meta->track_meta != NULL) {
		trackMeta_delete(album_meta->track_meta);
		album_meta->track_meta = NULL;
	}
}

static struct trackMeta *trackMeta_create(const struct albumMeta *album_meta)
{
	GtkTreeIter iter;
	GtkListStore *store;
	int i;
	gboolean valid_row;
	int track_rip;
	int track_num;
	const char *track_artist;
	const char *track_title;
	const char *track_time;
	struct trackMeta track_meta_local;
	struct trackMeta *root;
	struct trackMeta *current;
	char *tmp;
	unsigned int min;
	unsigned int sec;

	store = GTK_LIST_STORE(
		gtk_tree_view_get_model(GTK_TREE_VIEW(lookup_widget(win_main, "tracklist"))));

	for (i = 0, root = NULL, current = NULL,
		valid_row = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
		 valid_row; valid_row = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter), i++) {
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, COL_RIPTRACK, &track_rip, COL_TRACKNUM,
						   &track_num, COL_TRACKARTIST, &track_artist, COL_TRACKTITLE, &track_title,
						   COL_TRACKTIME, &track_time, -1);

		if (!track_rip) {
			continue;
		}

		track_meta_local.num_src = i + 1;
		track_meta_local.num_dst = track_num;

		sscanf(track_time, "%u:%u", &min, &sec);
		track_meta_local.time = sec + min * 60;

		if (album_meta->single_artist) {
			track_meta_local.artist = strdup(album_meta->artist);
		} else {
			track_meta_local.artist = strdup(track_artist);
		}
		track_meta_local.title = strdup(track_title);
		track_meta_local.rip_name = NULL;
		track_meta_local.next = NULL;

		if (track_meta_local.artist == NULL || track_meta_local.title == NULL) {
			trackMeta_clean(&track_meta_local);
			continue;
		}

		if (!album_meta->single_artist) {
			trim_chars(track_meta_local.artist, BADCHARS);
		}
		trim_chars(track_meta_local.title, BADCHARS);

		tmp = parse_format(global_prefs->format_music, track_meta_local.num_dst, album_meta->year,
						   track_meta_local.artist, album_meta->title, album_meta->genre,
						   track_meta_local.title);
		if (tmp != NULL) {
			track_meta_local.rip_name =
				make_filename(prefs_get_music_dir(global_prefs), album_meta->directory, tmp, "wav");
			free(tmp);

			if (track_meta_local.rip_name == NULL) {
				fprintf(stderr, " unable to make filename\n");
				trackMeta_clean(&track_meta_local);
				continue;
			}
		} else {
			fprintf(stderr, "unable to parse music format\n");
			trackMeta_clean(&track_meta_local);
			continue;
		}

		if (current != NULL) {
			if ((current->next = malloc(sizeof(struct trackMeta))) == NULL) {
				fprintf(stderr, "cannot allocate memory\n");
				trackMeta_clean(&track_meta_local);
				continue;
			}

			current = current->next;
		} else {
			if ((root = malloc(sizeof(struct trackMeta))) == NULL) {
				fprintf(stderr, "cannot allocate memory\n");
				trackMeta_clean(&track_meta_local);
				continue;
			}

			current = root;
		}

		memcpy(current, &track_meta_local, sizeof(struct trackMeta));
	}

	return root;
}

static struct albumMeta *albumMeta_create(void)
{
	struct albumMeta *album_meta;
	GtkWidget *album_artist_widget;
	GtkWidget *album_genre_widget;

	if ((album_meta = calloc(1, sizeof(struct albumMeta))) == NULL) {
		fprintf(stderr, "cannot allocate memory\n");
		return NULL;
	}

	album_artist_widget = lookup_widget(win_main, "album_artist");
	album_genre_widget = lookup_widget(win_main, "album_genre");

	album_meta->artist = strdup(gtk_entry_get_text(GTK_ENTRY(album_artist_widget)));
	album_meta->title =
		strdup(gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_main, "album_title"))));
	album_meta->genre = strdup(gtk_entry_get_text(GTK_ENTRY(album_genre_widget)));
	album_meta->year = strdup(gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_main, "album_year"))));
	album_meta->directory = NULL;
	album_meta->single_artist =
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_main, "single_artist")));

	if (album_meta->artist == NULL || album_meta->title == NULL || album_meta->genre == NULL ||
		album_meta->year == NULL) {
		albumMeta_delete(album_meta);
		return NULL;
	}

	trim_chars(album_meta->artist, BADCHARS);
	trim_chars(album_meta->title, BADCHARS);
	trim_chars(album_meta->genre, BADCHARS);

	add_completion(album_artist_widget);
	save_completion(album_artist_widget);

	add_completion(album_genre_widget);
	save_completion(album_genre_widget);

	album_meta->directory = parse_format(global_prefs->format_albumdir, 0, album_meta->year,
										 album_meta->artist, album_meta->title, album_meta->genre,
										 NULL);
	if (album_meta->directory == NULL) {
		albumMeta_delete(album_meta);
		return NULL;
	}

	album_meta->track_meta = trackMeta_create(album_meta);

	return album_meta;
}

static unsigned int albumMeta_get_nb_track(const struct albumMeta *album_meta)
{
	unsigned int result;
	struct trackMeta *current;

	for (current = album_meta->track_meta, result = 0; current != NULL; current = current->next) {
		result++;
	}

	return result;
}

void dorip(void)
{
	struct albumMeta *album_meta;

	working = true;
	aborted = false;
	allDone = false;
	counter = 0;
	rip_percent = 0.0;
	mp3_percent = 0.0;
	ogg_percent = 0.0;
	flac_percent = 0.0;
	wavpack_percent = 0.0;
	monkey_percent = 0.0;
	musepack_percent = 0.0;
	rip_tracks_completed = 0;
	encode_tracks_completed = 0;

	if (!(global_prefs->rip_wav | global_prefs->rip_mp3 | global_prefs->rip_ogg |
		  global_prefs->rip_opus | global_prefs->rip_flac | global_prefs->rip_wavpack |
		  global_prefs->rip_monkey | global_prefs->rip_musepack | global_prefs->rip_aac)) {
		GtkWidget *dialog;
		dialog = gtk_message_dialog_new(
			GTK_WINDOW(win_main), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
			_("No ripping/encoding method selected. Please enable one from the "
			  "'Preferences' menu."));
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return;
	}

	if ((album_meta = albumMeta_create()) == NULL) {
		fprintf(stderr, "cannot init album metadata structure\n");
		return;
	}

	if (!(tracks_to_rip = albumMeta_get_nb_track(album_meta))) {
		GtkWidget *dialog;
		dialog = gtk_message_dialog_new(GTK_WINDOW(win_main), GTK_DIALOG_DESTROY_WITH_PARENT,
										GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
										_("No tracks were selected for ripping/encoding. "
										  "Please select at least one track."));
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);

		albumMeta_delete(album_meta);
		return;
	}

	/* create  the album directory */
	{
		char *directory_path;

		if ((directory_path = make_filename(prefs_get_music_dir(global_prefs),
											album_meta->directory, NULL, NULL)) == NULL) {
			fprintf(stderr, "unable to make filename\n");
			albumMeta_delete(album_meta);
			return;
		}

		if (recursive_mkdir(directory_path, S_IRWXU | S_IRWXG | S_IRWXO) != 0 && errno != EEXIST) {
			GtkWidget *dialog;
			dialog = gtk_message_dialog_new(GTK_WINDOW(win_main), GTK_DIALOG_DESTROY_WITH_PARENT,
											GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
											"Unable to create directory '%s': %s", directory_path,
											strerror(errno));
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);

			free(directory_path);
			albumMeta_delete(album_meta);
			return;
		}

		free(directory_path);
	}

	gtk_widget_show(win_ripping);

	numCdparanoiaFailed = 0;
	numLameFailed = 0;
	numOggFailed = 0;
	numOpusFailed = 0;
	numFlacFailed = 0;
	numWavpackFailed = 0;
	numMonkeyFailed = 0;
	numMusepackFailed = 0;
	numAacFailed = 0;

	numCdparanoiaOk = 0;
	numLameOk = 0;
	numOggOk = 0;
	numOpusOk = 0;
	numFlacOk = 0;
	numWavpackOk = 0;
	numMonkeyOk = 0;
	numMusepackOk = 0;
	numAacOk = 0;

	ripper = g_thread_new(NULL, rip, album_meta);
	encoder = g_thread_new(NULL, encode, album_meta);
	tracker = g_thread_new(NULL, track, NULL);
}

static gpointer rip(gpointer data)
{
	const struct albumMeta *album_meta = (const struct albumMeta *)data;
	struct trackMeta *cursor;

	for (cursor = album_meta->track_meta; cursor != NULL; cursor = cursor->next) {
		if (aborted) {
			return NULL;
		}
		cdparanoia(global_prefs->cdrom, cursor->num_src, cursor->rip_name, &rip_percent);

		rip_percent = 0.0;
		rip_tracks_completed++;

		g_mutex_lock(&barrier);
		counter++;
		g_mutex_unlock(&barrier);
		g_cond_signal(&available);
	}

	if (global_prefs->eject_on_done) {
		if (!fork()) {
			const char *args[] = { "eject", global_prefs->cdrom, NULL };
			execvp(args[0], (char *const *)args);
		}
	}

	return NULL;
}

#define make_playlist(playlist, playlist_name, ext)                                             \
	{                                                                                           \
		char *filename;                                                                         \
                                                                                                \
		if ((filename = make_filename(prefs_get_music_dir(global_prefs), album_meta->directory, \
									  (playlist_name), (ext))) == NULL) {                       \
			fprintf(stderr, "unable to make filename\n");                                       \
		} else {                                                                                \
			(playlist) = fopen(filename, "w");                                                  \
			if ((playlist) == NULL) {                                                           \
				fprintf(stderr, "unable to open file: %s\n", filename);                         \
			}                                                                                   \
			free(filename);                                                                     \
		}                                                                                       \
	}

#define write_playlist(playlist)                                                      \
	{                                                                                 \
		if ((playlist) != NULL) {                                                     \
			if (aborted) {                                                            \
				goto exit;                                                            \
			}                                                                         \
                                                                                      \
			fprintf((playlist), "#EXTINF:%u,%s - %s\n", cursor->time, cursor->artist, \
					cursor->title);                                                   \
			fprintf((playlist), "%s\n", basename(file_name));                         \
		}                                                                             \
	}

#define close_playlist(playlist)  \
	{                             \
		if ((playlist) != NULL) { \
			fclose(playlist);     \
		}                         \
	}

static gpointer encode(gpointer data)
{
	struct albumMeta *album_meta = (struct albumMeta *)data;
	struct trackMeta *cursor;
	char file_name[PATH_MAX];
	char *ext_ptr;

	FILE *playlist_wav = NULL;
	FILE *playlist_mp3 = NULL;
	FILE *playlist_ogg = NULL;
	FILE *playlist_opus = NULL;
	FILE *playlist_flac = NULL;
	FILE *playlist_wavpack = NULL;
	FILE *playlist_monkey = NULL;
	FILE *playlist_musepack = NULL;
	FILE *playlist_aac = NULL;

	if (global_prefs->make_playlist) {
		char *playlist_name;

		playlist_name = parse_format(global_prefs->format_playlist, 0, album_meta->year,
									 album_meta->artist, album_meta->title, album_meta->genre,
									 NULL);
		if (playlist_name != NULL) {
			if (global_prefs->rip_wav) {
				make_playlist(playlist_wav, playlist_name, "wav.m3u")
			}
			if (global_prefs->rip_mp3) {
				make_playlist(playlist_mp3, playlist_name, "mp3.m3u")
			}
			if (global_prefs->rip_ogg) {
				make_playlist(playlist_ogg, playlist_name, "ogg.m3u")
			}
			if (global_prefs->rip_opus) {
				make_playlist(playlist_opus, playlist_name, "opus.m3u")
			}
			if (global_prefs->rip_flac) {
				make_playlist(playlist_flac, playlist_name, "flac.m3u")
			}
			if (global_prefs->rip_wavpack) {
				make_playlist(playlist_wavpack, playlist_name, "wv.m3u")
			}
			if (global_prefs->rip_monkey) {
				make_playlist(playlist_monkey, playlist_name, "ape.m3u")
			}
			if (global_prefs->rip_musepack) {
				make_playlist(playlist_musepack, playlist_name, "mpc.m3u")
			}
			if (global_prefs->rip_aac) {
				make_playlist(playlist_aac, playlist_name, "m4a.m3u")
			}

			free(playlist_name);
		} else {
			fprintf(stderr, "Error: unable to parse format\n");
		}
	}

	for (cursor = album_meta->track_meta; cursor != NULL; cursor = cursor->next) {
		g_mutex_lock(&barrier);
		while ((counter < 1) && (!aborted)) {
			g_cond_wait(&available, &barrier);
		}
		counter--;
		g_mutex_unlock(&barrier);

		if (aborted) {
			goto exit;
		}

		strcpy(file_name, cursor->rip_name);
		ext_ptr = file_name + strlen(file_name);

		while (ext_ptr != file_name && *ext_ptr != '.') {
			ext_ptr--;
		}

		if (global_prefs->rip_mp3) {
			if (aborted) {
				return NULL;
			}

			strcpy(ext_ptr, ".mp3");

			mp3_enc(cursor->num_dst, cursor->artist, album_meta->title, cursor->title,
					album_meta->year, album_meta->genre, cursor->rip_name, file_name,
					global_prefs->mp3_vbr, global_prefs->mp3_bitrate, &mp3_percent);

			write_playlist(playlist_mp3)
		}
		if (global_prefs->rip_ogg) {
			if (aborted) {
				goto exit;
			}

			strcpy(ext_ptr, ".ogg");

			ogg_enc(cursor->num_dst, cursor->artist, album_meta->title, cursor->title,
					album_meta->year, album_meta->genre, cursor->rip_name, file_name,
					global_prefs->ogg_quality, &ogg_percent);

			write_playlist(playlist_ogg)
		}
		if (global_prefs->rip_opus) {
			if (aborted) {
				goto exit;
			}

			strcpy(ext_ptr, ".opus");

			opus_enc(cursor->num_dst, cursor->artist, album_meta->title, cursor->title,
					 album_meta->year, album_meta->genre, cursor->rip_name, file_name,
					 global_prefs->opus_bitrate);

			write_playlist(playlist_opus)
		}
		if (global_prefs->rip_flac) {
			if (aborted) {
				goto exit;
			}

			strcpy(ext_ptr, ".flac");

			flac_enc(cursor->num_dst, cursor->artist, album_meta->title, cursor->title,
					 album_meta->year, album_meta->genre, cursor->rip_name, file_name,
					 global_prefs->flac_compression, &flac_percent);

			write_playlist(playlist_flac)
		}
		if (global_prefs->rip_wavpack) {
			if (aborted) {
				goto exit;
			}

			strcpy(ext_ptr, ".wv");

			wavpack_enc(cursor->rip_name, global_prefs->wavpack_compression,
						global_prefs->wavpack_hybrid,
						int_to_wavpack_bitrate(global_prefs->wavpack_bitrate), &wavpack_percent);

			write_playlist(playlist_wavpack)
		}
		if (global_prefs->rip_monkey) {
			if (aborted) {
				goto exit;
			}

			strcpy(ext_ptr, ".ape");

			monkey_enc(cursor->rip_name, file_name,
					   int_to_monkey_int(global_prefs->monkey_compression), &monkey_percent);

			write_playlist(playlist_monkey)
		}
		if (global_prefs->rip_musepack) {
			if (aborted) {
				goto exit;
			}

			strcpy(ext_ptr, ".mpc");

			musepack_enc(cursor->rip_name, file_name,
						 int_to_musepack_int(global_prefs->musepack_bitrate), &musepack_percent);

			write_playlist(playlist_musepack)
		}
		if (global_prefs->rip_aac) {
			if (aborted) {
				goto exit;
			}

			strcpy(ext_ptr, ".m4a");

			aac_enc(cursor->num_dst, cursor->artist, album_meta->title, cursor->title,
					album_meta->year, album_meta->genre, cursor->rip_name, file_name,
					global_prefs->aac_quality);

			write_playlist(playlist_aac)
		}
		if (!global_prefs->rip_wav) {
			if (unlink(cursor->rip_name)) {
				debugLog("Unable to delete WAV file \"%s\": %s\n", cursor->rip_name,
						 strerror(errno));
			}
		} else {
			write_playlist(playlist_wav)
		}

		mp3_percent = 0.0;
		ogg_percent = 0.0;
		flac_percent = 0.0;
		wavpack_percent = 0.0;
		monkey_percent = 0.0;
		musepack_percent = 0.0;
		encode_tracks_completed++;
	}

	/* wait until all the worker threads are done */
	while (cdparanoia_pid != 0 || lame_pid != 0 || oggenc_pid != 0 || opusenc_pid != 0 ||
		   flac_pid != 0 || wavpack_pid != 0 || monkey_pid != 0 || musepack_pid != 0 ||
		   aac_pid != 0) {
		debugLog("w2");
		usleep(100000);
	}

	allDone = true; // so the tracker thread will exit
	working = false;

	gdk_threads_enter();
	gtk_widget_hide(win_ripping);
	gdk_flush();

	show_completed_dialog(numCdparanoiaOk + numLameOk + numOggOk + numOpusOk + numFlacOk +
							  numWavpackOk + numMonkeyOk + numMusepackOk + numAacOk,
						  numCdparanoiaFailed + numLameFailed + numOggFailed + numOpusFailed +
							  numFlacFailed + numWavpackFailed + numMonkeyFailed +
							  numMusepackFailed + numAacFailed);
	gdk_threads_leave();

exit:

	close_playlist(playlist_wav);
	close_playlist(playlist_mp3);
	close_playlist(playlist_ogg);
	close_playlist(playlist_opus);
	close_playlist(playlist_flac);
	close_playlist(playlist_wavpack);
	close_playlist(playlist_monkey);
	close_playlist(playlist_musepack);
	close_playlist(playlist_aac);

	return NULL;
}

static gpointer track(gpointer data)
{
	int parts = 1;
	if (global_prefs->rip_mp3)
		parts++;
	if (global_prefs->rip_ogg)
		parts++;
	if (global_prefs->rip_opus)
		parts++;
	if (global_prefs->rip_flac)
		parts++;
	if (global_prefs->rip_wavpack)
		parts++;
	if (global_prefs->rip_monkey)
		parts++;
	if (global_prefs->rip_musepack)
		parts++;
	if (global_prefs->rip_aac)
		parts++;

	gdk_threads_enter();
	GtkProgressBar *progress_total = GTK_PROGRESS_BAR(lookup_widget(win_ripping, "progress_total"));
	GtkProgressBar *progress_rip = GTK_PROGRESS_BAR(lookup_widget(win_ripping, "progress_rip"));
	GtkProgressBar *progress_encode =
		GTK_PROGRESS_BAR(lookup_widget(win_ripping, "progress_encode"));

	gtk_progress_bar_set_fraction(progress_total, 0.0);
	gtk_progress_bar_set_text(progress_total, _("Waiting..."));
	gtk_progress_bar_set_fraction(progress_rip, 0.0);
	gtk_progress_bar_set_text(progress_rip, _("Waiting..."));
	if (parts > 1) {
		gtk_progress_bar_set_fraction(progress_encode, 0.0);
		gtk_progress_bar_set_text(progress_encode, _("Waiting..."));
	} else {
		gtk_progress_bar_set_fraction(progress_encode, 1.0);
		gtk_progress_bar_set_text(progress_encode, "100% (0/0)");
	}
	gdk_threads_leave();

	double prip;
	char srip[13];
	double pencode = 0;
	char sencode[13];
	double ptotal;
	char stotal[5];
	char windowTitle[15]; /* "Asunder - 100%" */

	while (!allDone) {
		if (aborted)
			g_thread_exit(NULL);

		prip = (rip_tracks_completed + rip_percent) / tracks_to_rip;
		snprintf(srip, 13, "%d%% (%u/%u)", (int)(prip * 100),
				 (rip_tracks_completed < tracks_to_rip) ? (rip_tracks_completed + 1) :
														  tracks_to_rip,
				 tracks_to_rip);
		if (parts > 1) {
			pencode = ((double)encode_tracks_completed / (double)tracks_to_rip) +
					  ((mp3_percent + ogg_percent + flac_percent + wavpack_percent +
						monkey_percent + musepack_percent) /
					   (parts - 1) / tracks_to_rip);
			snprintf(sencode, 13, "%d%% (%u/%u)", (int)(pencode * 100),
					 (encode_tracks_completed < tracks_to_rip) ? (encode_tracks_completed + 1) :
																 tracks_to_rip,
					 tracks_to_rip);
			ptotal = prip / parts + pencode * (parts - 1) / parts;
		} else {
			ptotal = prip;
		}
		snprintf(stotal, 5, "%d%%", (int)(ptotal * 100));

		strcpy(windowTitle, "Asunder - ");
		strcat(windowTitle, stotal);

		if (aborted)
			g_thread_exit(NULL);

		gdk_threads_enter();
		gtk_progress_bar_set_fraction(progress_rip, prip);
		gtk_progress_bar_set_text(progress_rip, srip);
		if (parts > 1) {
			gtk_progress_bar_set_fraction(progress_encode, pencode);
			gtk_progress_bar_set_text(progress_encode, sencode);
		}

		gtk_progress_bar_set_fraction(progress_total, ptotal);
		gtk_progress_bar_set_text(progress_total, stotal);

		gtk_window_set_title(GTK_WINDOW(win_main), windowTitle);
		gdk_threads_leave();

		usleep(100000);
	}

	gdk_threads_enter();
	gtk_window_set_title(GTK_WINDOW(win_main), "Asunder");
	gdk_threads_leave();

	return NULL;
}
