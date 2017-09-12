#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "prefs.h"
#include "main.h"
#include "util.h"
#include "support.h"
#include "interface.h"

#define CONFIG_FILENAME "asunder"

#define DEFAULT_PROXY               "10.0.0.1"
#define DEFAULT_PROXY_PORT          8080
#define DEFAULT_CDDB_SERVER         "freedb.freedb.org"
#define DEFAULT_CDDB_SERVER_PORT    8880

struct prefs* global_prefs;

static struct prefs* new_prefs(void){
    struct prefs* p;

    if ((p = calloc(1, sizeof(struct prefs))) == NULL){
        fatalError("calloc() failed. Out of memory.");
    }

    return p;
}

static void clear_prefs(struct prefs* p){
    if (p->cdrom != NULL){
        free(p->cdrom);
        p->cdrom = NULL;
    }

    if (p->music_dir != NULL){
        free(p->music_dir);
        p->music_dir = NULL;
    }

    if (p->format_music != NULL){
        free(p->format_music);
        p->format_music = NULL;
    }

    if (p->format_playlist != NULL){
        free(p->format_playlist);
        p->format_playlist = NULL;
    }

    if (p->format_albumdir != NULL){
        free(p->format_albumdir);
        p->format_albumdir = NULL;
    }

    if (p->server_name != NULL){
        free(p->server_name);
        p->server_name = NULL;
    }

    if (p->cddb_server_name != NULL){
        free(p->cddb_server_name);
        p->cddb_server_name = NULL;
    }
}

void delete_prefs(struct prefs* p){
    clear_prefs(p);
    free(p);
}

struct prefs * get_default_prefs(void){
    struct prefs* p = new_prefs();

    p->cdrom                        = strdup("/dev/cdrom");
    p->music_dir                    = strdup(getenv("HOME"));
    p->make_playlist                = 1;
    p->format_music                 = strdup("%N - %A - %T");
    p->format_playlist              = strdup("%A - %L");
    p->format_albumdir              = strdup("%A - %L");
    p->rip_wav                      = 0;
    p->rip_mp3                      = 0;
    p->rip_ogg                      = 1;
    p->rip_flac                     = 0;
    p->rip_wavpack                  = 0;
    p->mp3_vbr                      = 1;
    p->mp3_bitrate                  = 10;
    p->ogg_quality                  = 6;
    p->flac_compression             = 5;
    p->wavpack_compression          = 1;
    p->wavpack_hybrid               = 1;
    p->wavpack_bitrate              = 3;
    p->rip_monkey                   = 0;
    p->monkey_compression           = 2;
    p->rip_aac                      = 0;
    p->aac_quality                  = 60;
    p->rip_musepack                 = 0;
    p->musepack_bitrate             = 2;
    p->rip_opus                     = 0;
    p->opus_bitrate                 = 9;
    p->eject_on_done                = 0;
    p->do_cddb_updates              = 1;
    p->use_proxy                    = 0;
    p->do_log                       = 0;
    p->server_name                  = strdup(DEFAULT_PROXY);
    p->port_number                  = DEFAULT_PROXY_PORT;
    p->cddb_server_name             = strdup(DEFAULT_CDDB_SERVER);
    p->cddb_port_number             = DEFAULT_CDDB_SERVER_PORT;
    p->more_formats_expanded        = 0;
    p->proprietary_formats_expanded = 0;
    p->do_fast_rip                  = 0;

    if (p->cdrom == NULL || p->music_dir == NULL || p->format_music == NULL || p->format_playlist == NULL || p->format_albumdir == NULL || p->server_name == NULL || p->cddb_server_name == NULL){
        fatalError("strdup() failed. Out of memory.");
    }

    return p;
}

void set_widgets_from_prefs(struct prefs* p){
    char tempStr[10];

    gtk_entry_set_text(GTK_ENTRY(lookup_widget(win_prefs, "cdrom")), p->cdrom);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(lookup_widget(win_prefs, "music_dir")), prefs_get_music_dir(p));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "make_playlist")), p->make_playlist);
    gtk_entry_set_text(GTK_ENTRY(lookup_widget(win_prefs, "format_music")), p->format_music);
    gtk_entry_set_text(GTK_ENTRY(lookup_widget(win_prefs, "format_playlist")), p->format_playlist);
    gtk_entry_set_text(GTK_ENTRY(lookup_widget(win_prefs, "format_albumdir")), p->format_albumdir);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_wav")), p->rip_wav);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "do_fast_rip")), p->do_fast_rip);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_mp3")), p->rip_mp3);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_ogg")), p->rip_ogg);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_flac")), p->rip_flac);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_wavpack")), p->rip_wavpack);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "mp3_vbr")), p->mp3_vbr);
    gtk_range_set_value(GTK_RANGE(lookup_widget(win_prefs, "mp3bitrate")), p->mp3_bitrate);
    gtk_range_set_value(GTK_RANGE(lookup_widget(win_prefs, "oggquality")), p->ogg_quality);
    gtk_range_set_value(GTK_RANGE(lookup_widget(win_prefs, "flaccompression")), p->flac_compression);
    gtk_range_set_value(GTK_RANGE(lookup_widget(win_prefs, "wavpack_compression")), p->wavpack_compression);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "wavpack_hybrid")), p->wavpack_hybrid);
    gtk_range_set_value(GTK_RANGE(lookup_widget(win_prefs, "wavpack_bitrate_slider")), p->wavpack_bitrate);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_monkey")), p->rip_monkey);
    gtk_range_set_value(GTK_RANGE(lookup_widget(win_prefs, "monkey_compression_slider")), p->monkey_compression);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_aac")), p->rip_aac);
    gtk_range_set_value(GTK_RANGE(lookup_widget(win_prefs, "aac_quality_slider")), p->aac_quality);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_musepack")), p->rip_musepack);
    gtk_range_set_value(GTK_RANGE(lookup_widget(win_prefs, "musepack_bitrate_slider")), p->musepack_bitrate);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_opus")), p->rip_opus);
    gtk_range_set_value(GTK_RANGE(lookup_widget(win_prefs, "opusrate")), p->opus_bitrate);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "eject_on_done")), p->eject_on_done);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "do_cddb_updates")), p->do_cddb_updates);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "use_proxy")), p->use_proxy);
    gtk_entry_set_text(GTK_ENTRY(lookup_widget(win_prefs, "server_name")), p->server_name);
    snprintf(tempStr, sizeof(tempStr), "%d", p->port_number);
    gtk_entry_set_text(GTK_ENTRY(lookup_widget(win_prefs, "port_number")), tempStr);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "do_log")), p->do_log);
    gtk_entry_set_text(GTK_ENTRY(lookup_widget(win_prefs, "cddb_server_name")), p->cddb_server_name);
    snprintf(tempStr, sizeof(tempStr), "%d", p->cddb_port_number);
    gtk_entry_set_text(GTK_ENTRY(lookup_widget(win_prefs, "cddb_port_number")), tempStr);
    if (global_prefs->more_formats_expanded)
    {
        gtk_expander_set_expanded (GTK_EXPANDER(lookup_widget(win_prefs, "more_formats_expander")), TRUE);
    }
    if (global_prefs->proprietary_formats_expanded)
    {
        gtk_expander_set_expanded (GTK_EXPANDER(lookup_widget(win_prefs, "proprietary_formats_expander")), TRUE);
    }

    /* disable widgets if needed */
    if (!p->rip_mp3){
        disable_mp3_widgets();
    }
    if (!p->rip_ogg){
        disable_ogg_widgets();
    }
    if (!p->rip_flac){
        disable_flac_widgets();
    }
    if (!p->rip_wavpack){
        disable_wavpack_widgets();
    }
    else{
        enable_wavpack_widgets(); /* need this to potentially disable hybrid widgets */
    }
    if (!p->rip_monkey){
        disable_monkey_widgets();
    }
    if (!p->rip_aac){
        disable_aac_widgets();
    }
    if (!p->rip_musepack){
        disable_musepack_widgets();
    }
    if (!p->rip_opus){
        disable_opus_widgets();
    }
}

void get_prefs_from_widgets(struct prefs* p){
    gchar* tocopy;

    clear_prefs(p);

    p->cdrom                        = strdup(gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_prefs, "cdrom"))));

    tocopy = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(lookup_widget(win_prefs, "music_dir")));
    p->music_dir                    = strdup(tocopy);
    g_free(tocopy);

    p->make_playlist                = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "make_playlist")));
    p->format_music                 = strdup(gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_prefs, "format_music"))));
    p->format_playlist              = strdup(gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_prefs, "format_playlist"))));
    p->format_albumdir              = strdup(gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_prefs, "format_albumdir"))));
    p->rip_wav                      = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_wav")));
    p->do_fast_rip                  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "do_fast_rip")));
    p->rip_mp3                      = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_mp3")));
    p->rip_ogg                      = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_ogg")));
    p->rip_flac                     = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_flac")));
    p->rip_wavpack                  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_wavpack")));
    p->mp3_vbr                      = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "mp3_vbr")));
    p->mp3_bitrate                  = (int)gtk_range_get_value(GTK_RANGE(lookup_widget(win_prefs, "mp3bitrate")));
    p->ogg_quality                  = (int)gtk_range_get_value(GTK_RANGE(lookup_widget(win_prefs, "oggquality")));
    p->flac_compression             = (int)gtk_range_get_value(GTK_RANGE(lookup_widget(win_prefs, "flaccompression")));
    p->wavpack_compression          = (int)gtk_range_get_value(GTK_RANGE(lookup_widget(win_prefs, "wavpack_compression")));
    p->wavpack_hybrid               = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "wavpack_hybrid")));
    p->wavpack_bitrate              = (int)gtk_range_get_value(GTK_RANGE(lookup_widget(win_prefs, "wavpack_bitrate_slider")));
    p->rip_monkey                   = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_monkey")));
    p->monkey_compression           = (int)gtk_range_get_value(GTK_RANGE(lookup_widget(win_prefs, "monkey_compression_slider")));
    p->rip_aac                      = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_aac")));
    p->aac_quality                  = (int)gtk_range_get_value(GTK_RANGE(lookup_widget(win_prefs, "aac_quality_slider")));
    p->rip_musepack                 = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_musepack")));
    p->musepack_bitrate             = (int)gtk_range_get_value(GTK_RANGE(lookup_widget(win_prefs, "musepack_bitrate_slider")));
    p->rip_opus                     = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "rip_opus")));
    p->opus_bitrate                 = (int)gtk_range_get_value(GTK_RANGE(lookup_widget(win_prefs, "opusrate")));
    p->eject_on_done                = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "eject_on_done")));
    p->do_cddb_updates              = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "do_cddb_updates")));
    p->use_proxy                    = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "use_proxy")));
    p->server_name                  = strdup(gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_prefs, "server_name"))));
    p->port_number                  = atoi(gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_prefs, "port_number"))));
    p->cddb_server_name             = strdup(gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_prefs, "cddb_server_name"))));
    p->cddb_port_number             = atoi(gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_prefs, "cddb_port_number"))));
    p->do_log                       = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(win_prefs, "do_log")));
    p->more_formats_expanded        = gtk_expander_get_expanded (GTK_EXPANDER(lookup_widget(win_prefs, "more_formats_expander")));
    p->proprietary_formats_expanded = gtk_expander_get_expanded (GTK_EXPANDER(lookup_widget(win_prefs, "proprietary_formats_expander")));

    if (p->cdrom == NULL || p->music_dir == NULL || p->format_music == NULL || p->format_playlist == NULL || p->format_albumdir == NULL || p->server_name == NULL || p->cddb_server_name == NULL){
        fatalError("strdup() failed. Out of memory.");
    }
}

#define get_prefs_filename(file_name)                                            \
    {                                                                            \
        strncpy(file_name, getenv("HOME"), PATH_MAX);                            \
        strncat(file_name, "/.config/asunder/" CONFIG_FILENAME, PATH_MAX);       \
    }

#define get_prefs_directory(file_name)                                           \
    {                                                                            \
        strncpy(file_name, getenv("HOME"), PATH_MAX);                            \
        strncat(file_name, "/.config/asunder", PATH_MAX);                        \
    }

void save_prefs(struct prefs* p){
    char  path[PATH_MAX];
    FILE* file;

    get_prefs_directory(path);

    if (recursive_mkdir(path, S_IRWXU|S_IRWXG|S_IRWXO) != 0 && errno != EEXIST){
        fprintf(stderr, "cannot create preference directory: %s\n", strerror(errno));
        return;
    }

    get_prefs_filename(path);

    if ((file = fopen(path, "w")) != NULL){
        fprintf(file, "%s\n", p->cdrom);
        fprintf(file, "%s\n", p->music_dir);
        fprintf(file, "%d\n", p->make_playlist);
        fprintf(file, "%s\n", p->format_music);
        fprintf(file, "%s\n", p->format_playlist);
        fprintf(file, "%s\n", p->format_albumdir);
        fprintf(file, "%d\n", p->rip_wav);
        fprintf(file, "%d\n", p->rip_mp3);
        fprintf(file, "%d\n", p->rip_ogg);
        fprintf(file, "%d\n", p->rip_flac);
        fprintf(file, "%d\n", p->mp3_vbr);
        fprintf(file, "%d\n", p->mp3_bitrate);
        fprintf(file, "%d\n", p->ogg_quality);
        fprintf(file, "%d\n", p->flac_compression);
        fprintf(file, "%d\n", p->eject_on_done);
        fprintf(file, "%d\n", p->do_cddb_updates);
        fprintf(file, "%d\n", p->use_proxy);
        fprintf(file, "%s\n", p->server_name);
        fprintf(file, "%d\n", p->port_number);
        fprintf(file, "%d\n", p->rip_wavpack);
        fprintf(file, "%d\n", p->wavpack_compression);
        fprintf(file, "%d\n", p->wavpack_hybrid);
        fprintf(file, "%d\n", p->wavpack_bitrate);
        fprintf(file, "%d\n", p->do_log);
        fprintf(file, "%s\n", p->cddb_server_name);
        fprintf(file, "%d\n", p->cddb_port_number);
        fprintf(file, "%d\n", p->rip_monkey);
        fprintf(file, "%d\n", p->monkey_compression);
        fprintf(file, "%d\n", p->rip_aac);
        fprintf(file, "%d\n", p->aac_quality);
        fprintf(file, "%d\n", p->rip_musepack);
        fprintf(file, "%d\n", p->musepack_bitrate);
        fprintf(file, "%d\n", p->more_formats_expanded);
        fprintf(file, "%d\n", p->proprietary_formats_expanded);
        fprintf(file, "%d\n", p->rip_opus);
        fprintf(file, "%d\n", p->opus_bitrate);
        fprintf(file, "%d\n", p->do_fast_rip);

        fclose(file);
    }
    else{
        fprintf(stderr, "cannot open %s: %s\n", path, strerror(errno));
    }
}

#define is_valid_port_number(number) ((number) >= 0 && (number) <= 65535)

void load_prefs(struct prefs* p){
    char    file_name[PATH_MAX];
    int     fd;
    int     tmp_int;
    char*   tmp_str;

    get_prefs_filename(file_name)

    #define update_if_not_zero(old_val, new_val) (tmp_int = (new_val), (tmp_int) ? tmp_int : (old_val))
    #define update_if_not_null(old_val, new_val) (tmp_str = (new_val), (tmp_str != NULL) ? ((((old_val) != NULL) ? free(old_val), 0 : 0), tmp_str) : (old_val))

    if ((fd = open(file_name, O_RDONLY)) > -1){
        p->cdrom                        = update_if_not_null(p->cdrom, read_line(fd));
        p->music_dir                    = update_if_not_null(p->music_dir, read_line(fd));
        p->make_playlist                = read_line_num(fd);
        p->format_music                 = update_if_not_null(p->format_music, read_line(fd));
        p->format_playlist              = update_if_not_null(p->format_playlist, read_line(fd));
        p->format_albumdir              = update_if_not_null(p->format_albumdir, read_line(fd));
        p->rip_wav                      = read_line_num(fd);
        p->rip_mp3                      = read_line_num(fd);
        p->rip_ogg                      = read_line_num(fd);
        p->rip_flac                     = read_line_num(fd);
        p->mp3_vbr                      = read_line_num(fd);
        p->mp3_bitrate                  = update_if_not_zero(p->mp3_bitrate, read_line_num(fd));
        p->ogg_quality                  = read_line_num(fd);
        p->flac_compression             = read_line_num(fd);
        p->eject_on_done                = read_line_num(fd);
        p->do_cddb_updates              = read_line_num(fd);
        p->use_proxy                    = read_line_num(fd);
        p->server_name                  = update_if_not_null(p->server_name, read_line(fd));
        p->port_number                  = read_line_num(fd);

        if (p->port_number == 0 || !is_valid_port_number(p->port_number))
        {
            fprintf(stderr, "bad port number read from config file, using %d instead\n", DEFAULT_PROXY_PORT);
            p->port_number = DEFAULT_PROXY_PORT;
        }

        p->rip_wavpack                  = read_line_num(fd);
        p->wavpack_compression          = read_line_num(fd);
        p->wavpack_hybrid               = read_line_num(fd);
        p->wavpack_bitrate              = read_line_num(fd);
        p->do_log                       = read_line_num(fd);
        p->cddb_server_name             = update_if_not_null(p->cddb_server_name, read_line(fd));
        p->cddb_port_number             = read_line_num(fd);

        if (p->cddb_port_number == 0 || !is_valid_port_number(p->cddb_port_number))
        {
            fprintf(stderr, "bad port number read from config file, using %d instead\n", DEFAULT_CDDB_SERVER_PORT);
            p->cddb_port_number = DEFAULT_CDDB_SERVER_PORT;
        }

        p->rip_monkey                   = read_line_num(fd);
        p->monkey_compression           = read_line_num(fd);
        p->rip_aac                      = read_line_num(fd);
        p->aac_quality                  = read_line_num(fd);
        p->rip_musepack                 = read_line_num(fd);
        p->musepack_bitrate             = read_line_num(fd);
        p->more_formats_expanded        = read_line_num(fd);
        p->proprietary_formats_expanded = read_line_num(fd);
        p->rip_opus                     = read_line_num(fd);
        p->opus_bitrate                 = update_if_not_zero(p->opus_bitrate, read_line_num(fd));
        p->do_fast_rip                  = read_line_num(fd);

        close(fd);
    }
    else{
        fprintf(stderr, "cannot open %s: %s\n", file_name, strerror(errno));
    }
}

char* prefs_get_music_dir(struct prefs* p){
    struct stat s;
    char*       home;
    GtkWidget*  dialog;

    if (stat(p->music_dir, &s)){
        home = getenv("HOME");

        dialog = gtk_message_dialog_new(GTK_WINDOW(win_main),
                                        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                        "The music directory '%s' does not exist.\n\n"
                                        "The music directory will be reset to '%s'.",
                                        p->music_dir, home);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        free(p->music_dir);

        if ((p->music_dir = strdup(home)) == NULL){
            fatalError("strdup() failed. Out of memory.");
        }

        save_prefs(p);
    }
    return p->music_dir;
}

int prefs_are_valid(void){
    GtkWidget*  warningDialog;
    int         somethingWrong = 0;
    int         port_number;

    // playlistfile
    if (strchr(gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_prefs, "format_playlist"))), '/') != NULL){
        warningDialog = gtk_message_dialog_new(GTK_WINDOW(win_main), GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                               _("Invalid characters in the '%s' field"),
                                               _("Playlist file: "));
        gtk_dialog_run(GTK_DIALOG(warningDialog));
        gtk_widget_destroy(warningDialog);
        somethingWrong = 1;
    }

    // musicfile
    if (strchr(gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_prefs, "format_music"))), '/') != NULL){
        warningDialog = gtk_message_dialog_new(GTK_WINDOW(win_main), GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                               _("Invalid characters in the '%s' field"),
                                               _("Music file: "));
        gtk_dialog_run(GTK_DIALOG(warningDialog));
        gtk_widget_destroy(warningDialog);
        somethingWrong = 1;
    }
    if (!strlen(gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_prefs, "format_music"))))){
        warningDialog = gtk_message_dialog_new(GTK_WINDOW(win_main), GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                               _("'%s' cannot be empty"),
                                               _("Music file: "));
        gtk_dialog_run(GTK_DIALOG(warningDialog));
        gtk_widget_destroy(warningDialog);
        somethingWrong = 1;
    }

    // proxy port
    port_number = atoi(gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_prefs, "port_number"))));
    if (!is_valid_port_number(port_number)){
        warningDialog = gtk_message_dialog_new(GTK_WINDOW(win_main), GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                               _("Invalid proxy port number"));
        gtk_dialog_run(GTK_DIALOG(warningDialog));
        gtk_widget_destroy(warningDialog);
        somethingWrong = 1;
    }

    // cddb server port
    port_number = atoi(gtk_entry_get_text(GTK_ENTRY(lookup_widget(win_prefs, "cddb_port_number"))));
    if (!is_valid_port_number(port_number)){
        warningDialog = gtk_message_dialog_new(GTK_WINDOW(win_main), GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                               _("Invalid cddb server port number"));
        gtk_dialog_run(GTK_DIALOG(warningDialog));
        gtk_widget_destroy(warningDialog);
        somethingWrong = 1;
    }

    return !somethingWrong;
}
