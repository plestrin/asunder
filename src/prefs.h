struct prefs
{
    char* cdrom;
    char* music_dir;
    int make_playlist;
    char* format_music;
    char* format_playlist;
    char* format_albumdir;
    int rip_wav;
    int rip_mp3;
    int rip_ogg;
    int rip_flac;
    int mp3_vbr;
    int mp3_bitrate;
    int ogg_quality;
    int flac_compression;
    int eject_on_done;
    int do_cddb_updates;
    int use_proxy;
    char* server_name;
    int port_number;
    int rip_wavpack;
    int wavpack_compression;
    int wavpack_hybrid;
    int wavpack_bitrate;
    int do_log;
    char* cddb_server_name;
    int cddb_port_number;
    int rip_monkey;
    int monkey_compression;
    int rip_aac;
    int aac_quality;
    int rip_musepack;
    int musepack_bitrate;
    int more_formats_expanded;
    int proprietary_formats_expanded;
    int rip_opus;
    int opus_bitrate;
    int do_fast_rip;
};

#define PREFS_DEFAULT_OGG_QUALITY 6
#define PREFS_DEFAULT_AAC_QUALITY 60

extern struct prefs* global_prefs;

void delete_prefs(struct prefs* p);

// returns a new prefs struct with all members set to nice default values
struct prefs* get_default_prefs(void);

// sets up all of the widgets in the preferences dialog to
// match the given prefs struct
void set_widgets_from_prefs(struct prefs* p);

void get_prefs_from_widgets(struct prefs* p);

// store the given prefs struct to the config file
void save_prefs(struct prefs* p);

// load the prefereces from the config file into the given prefs struct
void load_prefs(struct prefs* p);

// use this method when reading the "music_dir" field of a prefs struct
// it will make sure it always points to a nice directory
char* prefs_get_music_dir(struct prefs * p);

int prefs_are_valid(void);
