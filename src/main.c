#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <sys/types.h>
#include <cddb/cddb.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <syslog.h>
#include <linux/cdrom.h>

#include "main.h"
#include "interface.h"
#include "support.h"
#include "prefs.h"
#include "callbacks.h"
#include "util.h"
#include "wrappers.h"
#include "threads.h"

static unsigned int gbl_current_discid = 0;

GList * gbl_disc_matches = NULL;
gboolean track_format[100];

GtkWidget* win_main = NULL;
GtkWidget* win_prefs = NULL;
GtkWidget* win_ripping = NULL;
GtkWidget* win_about = NULL;

GtkWidget* album_artist;
GtkWidget* album_title;
GtkWidget* album_genre;					// lnr
GtkWidget* album_year;

GtkWidget* tracklist;
GtkWidget* pick_disc;

int gbl_null_fd;

int main(int argc, char *argv[]){
    GtkCellRenderer *renderer;

#ifdef ENABLE_NLS
    /* initialize gettext */
    bindtextdomain("asunder", PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset("asunder", "UTF-8"); /* so that gettext() returns UTF-8 strings */
    textdomain("asunder");
#endif

    /* SET UP signal handler for children */
    struct sigaction signalHandler;
    sigset_t blockedSignals;

    bzero(&signalHandler, sizeof(signalHandler));
    signalHandler.sa_handler = sigchld;
    //~ signalHandler.sa_flags = SA_RESTART;
    sigemptyset(&blockedSignals);
    sigaddset(&blockedSignals, SIGCHLD);
    signalHandler.sa_mask = blockedSignals;

    sigaction(SIGCHLD, &signalHandler, NULL);
    /* END SET UP signal handler for children */

    gbl_null_fd = open("/dev/null", O_RDWR);

    //gtk_set_locale();
    g_thread_init(NULL);
    gdk_threads_init();
    gtk_init(&argc, &argv);

    // If moving this in relation to the signal handler setup above - make sure
    // to pay attention to the check in sigchld(), see email from Ariel Faigon
    global_prefs = get_default_prefs();
    load_prefs(global_prefs);

    openlog("asunder", 0, LOG_USER);

    win_main = create_main();
    album_artist = lookup_widget(win_main, "album_artist");
    album_title = lookup_widget(win_main, "album_title");
    album_genre	= lookup_widget(win_main, "album_genre");				// lnr
    album_year = lookup_widget(win_main, "album_year");
    tracklist = lookup_widget(win_main, "tracklist");
    pick_disc = lookup_widget(win_main, "pick_disc");

    // set up all the columns for the track listing widget
    renderer = gtk_cell_renderer_toggle_new();
    g_object_set(renderer, "activatable", TRUE, NULL);
    g_signal_connect(renderer, "toggled", (GCallback) on_rip_toggled, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tracklist), -1,
                    _("Rip"), renderer, "active", COL_RIPTRACK, NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tracklist), -1,
                    _("Track"), renderer, "text", COL_TRACKNUM, NULL);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "editable", TRUE, NULL);
    g_signal_connect(renderer, "edited", (GCallback) on_artist_edited, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tracklist), -1,
                    _("Artist"), renderer, "text", COL_TRACKARTIST, NULL);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "editable", TRUE, NULL);
    g_signal_connect(renderer, "edited", (GCallback) on_title_edited, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tracklist), -1,
                    _("Title"), renderer, "text", COL_TRACKTITLE, NULL);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "editable", TRUE, NULL);
    g_signal_connect(renderer, "edited", (GCallback) on_genre_edited, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tracklist), -1,
                    _("Genre"), renderer, "text", COL_GENRE, NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tracklist), -1,
                    _("Time"), renderer, "text", COL_TRACKTIME, NULL);

    // set up the columns for the album selecting dropdown box
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(pick_disc), renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(pick_disc), renderer,
                                                    "text", 0,
                                                    NULL);
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(pick_disc), renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(pick_disc), renderer,
                                                    "text", 1,
                                                    NULL);

    // disable the "rip" button
    // it will be enabled when check_disc() finds a disc in the drive
    gtk_widget_set_sensitive(lookup_widget(win_main, "rip_button"), FALSE);

    win_ripping = create_ripping();

    if (!program_exists("cdparanoia")){
        GtkWidget * dialog;

        dialog = gtk_message_dialog_new(GTK_WINDOW(win_main), GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                _("'cdparanoia' was not found in your path. Asunder requires cdparanoia to rip CDs."));
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        exit(-1);
    }

    gtk_widget_show(win_main);

    // set up recurring timeout to automatically re-scan the cdrom once every second
    g_timeout_add(500, idle, (void *)1);
    // add an idle event to scan the cdrom drive ASAP
    g_idle_add(idle, NULL);

    gdk_threads_enter();
    gtk_main();
    gdk_threads_leave();

    return EXIT_SUCCESS;
}

// scan the cdrom device for a disc
// returns True if a disc is present and
//   is different from the last time this was called
bool check_disc(char * cdrom)
{
    int fd;
    bool ret = false;
    int status;

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__)
    struct ioc_read_subchannel cdsc;
    struct cd_sub_channel_info data;
#endif

    // open the device
    fd = open(cdrom, O_RDONLY | O_NONBLOCK);
    if (fd < 0)
    {
        fprintf(stderr, "Error: Couldn't open %s\n", cdrom);
        return false;
    }

    /* this was the original (Eric's 0.1 and post 0.0.1) checking code,
    * but it never worked properly for me. Replaced 21 aug 2007. */
    //~ static bool newdisc = true;
    //~ // read the drive status info
    //~ if (ioctl(fd, CDROM_DRIVE_STATUS, CDSL_CURRENT) == CDS_DISC_OK)
    //~ {
        //~ if (newdisc)
        //~ {
            //~ newdisc = false;

            //~ status = ioctl(fd, CDROM_DISC_STATUS, CDSL_CURRENT);
            //~ if ((status == CDS_AUDIO) || (status == CDS_MIXED))
            //~ {
                //~ ret = true;
            //~ }printf("status %d vs %d\n", status, CDS_NO_INFO);
        //~ }
    //~ } else {
        //~ newdisc = true;
        //~ clear_widgets();
    //~ }

    static bool alreadyKnowGood = false; /* check when program just started */
    static bool alreadyCleared = true; /* no need to clear when program just started */

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__)
    bzero(&cdsc, sizeof(cdsc));
    cdsc.data = &data;
    cdsc.data_len = sizeof(data);
    cdsc.data_format = CD_CURRENT_POSITION;
    cdsc.address_format = CD_MSF_FORMAT;
    status = ioctl(fd, CDIOCREADSUBCHANNEL, (char*)&cdsc);
    if (status >= 0)
#elif defined(__linux__)
    status = ioctl(fd, CDROM_DISC_STATUS, CDSL_CURRENT);
    if (status == CDS_AUDIO || status == CDS_MIXED)
#endif
    {
        if (!alreadyKnowGood)
        {
            ret = true;
            alreadyKnowGood = true; /* don't return true again for this disc */
            alreadyCleared = false; /* clear when disc is removed */
        }
    }
    else
    {
        alreadyKnowGood = false; /* return true when good disc inserted */
        if (!alreadyCleared)
        {
            alreadyCleared = true;
            clear_widgets();
        }
    }

    close(fd);
    return ret;
}


void clear_widgets(void){
    gbl_current_discid = 0;

    // hide the widgets for multiple albums
    gtk_widget_hide(lookup_widget(win_main, "disc"));
    gtk_widget_hide(lookup_widget(win_main, "pick_disc"));

    // clear the textboxes
    gtk_entry_set_text(GTK_ENTRY(album_artist), "");
    gtk_entry_set_text(GTK_ENTRY(album_title), "");
    gtk_entry_set_text(GTK_ENTRY(album_genre), "");				// lnr
    gtk_entry_set_text(GTK_ENTRY(album_year), "");

    // clear the tracklist
    gtk_tree_view_set_model(GTK_TREE_VIEW(tracklist), NULL);

    // disable the "rip" button
    gtk_widget_set_sensitive(lookup_widget(win_main, "rip_button"), FALSE);
}


GtkTreeModel * create_model_from_disc(cddb_disc_t * disc)
{
    GtkListStore*   store;
    GtkTreeIter     iter;
    cddb_track_t*   track;
    int             seconds;
    char            time[6];
    char            year[5];
    char*           track_artist;
    char*           track_title;

    store = gtk_list_store_new(NUM_COLS,
                               G_TYPE_BOOLEAN,  /* rip? checkbox */
                               G_TYPE_UINT,     /* track number */
                               G_TYPE_STRING,   /* track artist */
                               G_TYPE_STRING,   /* track title */
                               G_TYPE_STRING,   /* track time */
                               G_TYPE_STRING,   /* genre */
                               G_TYPE_STRING    /* year */
                               );

    for (track = cddb_disc_get_track_first(disc); track != NULL; track = cddb_disc_get_track_next(disc)){
        seconds = cddb_track_get_length(track);
        snprintf(time, sizeof(time), "%02d:%02d", seconds/60, seconds%60);

        track_artist = (char*)cddb_track_get_artist(track);
        trim_whitespace(track_artist);

        track_title = (char*)cddb_track_get_title(track); //!! this returns const char*
        trim_whitespace(track_title);

        snprintf(year, sizeof(year), "%d", cddb_disc_get_year(disc));

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
            COL_RIPTRACK, track_format[cddb_track_get_number(track)],
            COL_TRACKNUM, cddb_track_get_number(track),
            COL_TRACKARTIST, track_artist,
            COL_TRACKTITLE, track_title,
            COL_TRACKTIME, time,
            COL_GENRE, cddb_disc_get_genre(disc),
            COL_YEAR, year,
            -1);
    }

    return GTK_TREE_MODEL(store);
}

static GThread * gbl_cddb_query_thread;
static int gbl_cddb_query_thread_running;
static cddb_conn_t * gbl_cddb_query_thread_conn;
static cddb_disc_t * gbl_cddb_query_thread_disc;
static int gbl_cddb_query_thread_num_matches;
static GList * gbl_matches = NULL;

gpointer cddb_query_thread_run(gpointer data)
{
    int i;

    gbl_cddb_query_thread_num_matches = cddb_query(gbl_cddb_query_thread_conn, gbl_cddb_query_thread_disc);
    if (gbl_cddb_query_thread_num_matches == -1)
        gbl_cddb_query_thread_num_matches = 0;

    debugLog("found %d CDDB matches\n", gbl_cddb_query_thread_num_matches)

    gbl_matches = NULL;

    // make a list of all the matches
    for (i = 0; i < gbl_cddb_query_thread_num_matches; i++)
    {
        cddb_disc_t * possible_match = cddb_disc_clone(gbl_cddb_query_thread_disc);
        if (cddb_read(gbl_cddb_query_thread_conn, possible_match) == 1)
        {
            gbl_matches = g_list_append(gbl_matches, possible_match);

            // move to next match
            if (i < gbl_cddb_query_thread_num_matches - 1)
            {
                if (!cddb_query_next(gbl_cddb_query_thread_conn, gbl_cddb_query_thread_disc))
                    fatalError("Query index out of bounds.");
            }
        }
        else
            printf("Failed to cddb_read()\n");
    }

    g_atomic_int_set(&gbl_cddb_query_thread_running, 0);

    return NULL;
}

GList * lookup_disc(cddb_disc_t * disc)
{
    // set up the connection to the cddb server
    gbl_cddb_query_thread_conn = cddb_new();
    if (gbl_cddb_query_thread_conn == NULL)
        fatalError("cddb_new() failed. Out of memory?");

    cddb_set_server_name(gbl_cddb_query_thread_conn, global_prefs->cddb_server_name);
    cddb_set_server_port(gbl_cddb_query_thread_conn, global_prefs->cddb_port_number);

    if (global_prefs->use_proxy)
    {
        cddb_set_http_proxy_server_name(gbl_cddb_query_thread_conn, global_prefs->server_name);
        cddb_set_http_proxy_server_port(gbl_cddb_query_thread_conn, global_prefs->port_number);
        cddb_http_proxy_enable(gbl_cddb_query_thread_conn);
    }

    // force HTTP when port 80 (for MusicBrainz). This code by Tim.
    if (global_prefs->cddb_port_number == 80)
        cddb_http_enable(gbl_cddb_query_thread_conn);

    // query cddb to find similar discs
    g_atomic_int_set(&gbl_cddb_query_thread_running, 1);
    gbl_cddb_query_thread_disc = disc;
    gbl_cddb_query_thread = g_thread_create(cddb_query_thread_run, NULL, TRUE, NULL);

    // show cddb update window
    gdk_threads_enter();
        disable_all_main_widgets();

        GtkWidget* statusLbl = lookup_widget(win_main, "statusLbl");
        gtk_label_set_text(GTK_LABEL(statusLbl), _("<b>Getting disc info from the internet...</b>"));
        gtk_label_set_use_markup(GTK_LABEL(statusLbl), TRUE);

        while (g_atomic_int_get(&gbl_cddb_query_thread_running) != 0)
        {
            while (gtk_events_pending())
                gtk_main_iteration();
            usleep(100000);
        }

        gtk_label_set_text(GTK_LABEL(statusLbl), "");

        enable_all_main_widgets();
    gdk_threads_leave();

    cddb_destroy(gbl_cddb_query_thread_conn);

    return gbl_matches;
}

// reads the TOC of a cdrom into a CDDB struct
// returns the filled out struct
// so we can send it over the internet to lookup the disc
cddb_disc_t * read_disc(char * cdrom)
{
    int fd;
    int status;
    int i;
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
    struct ioc_toc_header th;
    struct ioc_read_toc_single_entry te;
    struct ioc_read_subchannel cdsc;
    struct cd_sub_channel_info data;
#elif defined(__NetBSD__)
    struct ioc_toc_header th;
    struct ioc_read_toc_entry te;
    struct cd_toc_entry toc ;
    struct ioc_read_subchannel cdsc;
    struct cd_sub_channel_info data;
#elif defined(__linux__)
    struct cdrom_tochdr th;
    struct cdrom_tocentry te;
#endif

    cddb_disc_t * disc = NULL;
    cddb_track_t * track = NULL;

    char trackname[9];

    // open the device
    fd = open(cdrom, O_RDONLY | O_NONBLOCK);
    if (fd < 0)
    {
        fprintf(stderr, "Error: Couldn't open %s\n", cdrom);
        return NULL;
    }

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
    // read disc status info
    bzero(&cdsc,sizeof(cdsc));
    cdsc.data = &data;
    cdsc.data_len = sizeof(data);
    cdsc.data_format = CD_CURRENT_POSITION;
    cdsc.address_format = CD_MSF_FORMAT;
    status = ioctl(fd, CDIOCREADSUBCHANNEL, (char *)&cdsc);
    if (status >= 0)
    {
        // see if we can read the disc's table of contents (TOC).
        if (ioctl(fd, CDIOREADTOCHEADER, &th) == 0)
        {
            debugLog("starting track: %d, ending track: %d\n", th.starting_track, th.ending_track)

            disc = cddb_disc_new();
            if (disc == NULL)
                fatalError("cddb_disc_new() failed. Out of memory?");

            te.address_format = CD_LBA_FORMAT;
            for (i=th.starting_track; i<=th.ending_track; i++)
            {
                te.track = i;
                if (ioctl(fd, CDIOREADTOCENTRY, &te) == 0)
                {
                    if (te.entry.control & 0x04)
                    {
                        // track is a DATA track. make sure its "rip" box is not checked by default
                        track_format[i] = FALSE;
                    } else {
                        track_format[i] = TRUE;
                    }

                    track = cddb_track_new();
                    if (track == NULL)
                        fatalError("cddb_track_new() failed. Out of memory?");

                    cddb_track_set_frame_offset(track, ntohl(te.entry.addr.lba)+SECONDS_TO_FRAMES(2));
                    snprintf(trackname, 9, "Track %d", i);
                    cddb_track_set_title(track, trackname);
                    cddb_track_set_artist(track, "Unknown Artist");
                    cddb_disc_add_track(disc, track);
                }
            }
            te.track = 0xAA;
            if (ioctl(fd, CDIOREADTOCENTRY, &te) == 0)
            {
                cddb_disc_set_length(disc, (ntohl(te.entry.addr.lba)+SECONDS_TO_FRAMES(2))/SECONDS_TO_FRAMES(1));
            }
        }
    }
#elif defined(__NetBSD__)
    // read disc status info
    bzero(&cdsc,sizeof(cdsc));
    cdsc.data = &data;
    cdsc.data_len = sizeof(data);
    cdsc.data_format = CD_CURRENT_POSITION;
    cdsc.address_format = CD_MSF_FORMAT;
    status = ioctl(fd, CDIOCREADSUBCHANNEL, (char *)&cdsc);
    if (status >= 0)
    {
        // see if we can read the disc's table of contents (TOC).
        if (ioctl(fd, CDIOREADTOCHEADER, &th) == 0)
        {
            debugLog("starting track: %d, ending track: %d\n", th.starting_track, th.ending_track)

            disc = cddb_disc_new();
            if (disc == NULL)
                fatalError("cddb_disc_new() failed. Out of memory?");

            te.address_format = CD_LBA_FORMAT;
            te.data = &toc ;
            te.data_len=sizeof(struct cd_toc_entry) ;
            for (i=th.starting_track; i<=th.ending_track; i++)
            {
                te.starting_track = i;
                if ((ioctl(fd, CDIOREADTOCENTRIES, &te)) == 0)
                {
                    if (te.data->control & 0x04)
                    {
                        // track is a DATA track. make sure its "rip" box is not checked by default
                        track_format[i] = FALSE;
                    } else {
                        track_format[i] = TRUE;
                    }

                    track = cddb_track_new();
                    if (track == NULL)
                        fatalError("cddb_track_new() failed. Out of memory?");

                    cddb_track_set_frame_offset(track, te.data->addr.lba+SECONDS_TO_FRAMES(2));
                    snprintf(trackname, 9, "Track %d", i);
                    cddb_track_set_title(track, trackname);
                    cddb_track_set_artist(track, "Unknown Artist");
                    cddb_disc_add_track(disc, track);
                }
            }
            te.starting_track = 0xAA;
            if (ioctl(fd, CDIOREADTOCENTRIES, &te) == 0)
             {
                cddb_disc_set_length(disc, (te.data->addr.lba+SECONDS_TO_FRAMES(2))/SECONDS_TO_FRAMES(1));
            }
        }
    }
#elif defined(__linux__)
    // read disc status info
    status = ioctl(fd, CDROM_DISC_STATUS, CDSL_CURRENT);
    if ((status == CDS_AUDIO) || (status == CDS_MIXED))
    {
        // see if we can read the disc's table of contents (TOC).
        if (ioctl(fd, CDROMREADTOCHDR, &th) == 0)
        {
            debugLog("starting track: %d, ending track: %d\n", th.cdth_trk0, th.cdth_trk1)

            disc = cddb_disc_new();
            if (disc == NULL)
                fatalError("cddb_disc_new() failed. Out of memory?");

            te.cdte_format = CDROM_LBA;
            for (i=th.cdth_trk0; i<=th.cdth_trk1; i++)
            {
                te.cdte_track = i;
                if (ioctl(fd, CDROMREADTOCENTRY, &te) == 0)
                {
                    if (te.cdte_ctrl & CDROM_DATA_TRACK)
                    {
                        // track is a DATA track. make sure its "rip" box is not checked by default
                        track_format[i] = FALSE;
                    } else {
                        track_format[i] = TRUE;
                    }

                    track = cddb_track_new();
                    if (track == NULL)
                        fatalError("cddb_track_new() failed. Out of memory?");

                    cddb_track_set_frame_offset(track, te.cdte_addr.lba + SECONDS_TO_FRAMES(2));
                    snprintf(trackname, 9, "Track %d", i);
                    cddb_track_set_title(track, trackname);
                    cddb_track_set_artist(track, "Unknown Artist");
                    cddb_disc_add_track(disc, track);
                }
            }

            te.cdte_track = CDROM_LEADOUT;
            if (ioctl(fd, CDROMREADTOCENTRY, &te) == 0)
            {
                cddb_disc_set_length(disc, (te.cdte_addr.lba+SECONDS_TO_FRAMES(2))/SECONDS_TO_FRAMES(1));
            }
        }
    }
#endif
    close(fd);

    /* These two lines from Nicolas L�veill�
    * "let us have a discid for each read disc" */
    if (disc)
        cddb_disc_calc_discid(disc);

    return disc;
}


void update_tracklist(cddb_disc_t * disc)
{
    GtkTreeModel*   model;
    char*           disc_artist = (char*)cddb_disc_get_artist(disc);
    char*           disc_title  = (char*)cddb_disc_get_title(disc);
    char*           disc_genre  = (char*)cddb_disc_get_genre(disc);
    unsigned        disc_year   = cddb_disc_get_year(disc);
    cddb_track_t *  track;
    bool            singleartist;

    gbl_current_discid = cddb_disc_get_discid(disc);

    debugLog("update_tracklist() disk '%s' '%s' '%s'\n", disc_artist, disc_title, disc_genre)

    if (disc_artist != NULL){
        //trim_chars(disc_artist, BADCHARS);			// lnr	//Commented out by mrpl
        trim_whitespace(disc_artist);
        gtk_entry_set_text(GTK_ENTRY(album_artist), disc_artist);

        singleartist = true;
        for (track = cddb_disc_get_track_first(disc); track != NULL; track = cddb_disc_get_track_next(disc))
        {
            if (strcmp(disc_artist, cddb_track_get_artist(track)) != 0)
            {
                singleartist = false;
                break;
            }
        }
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(win_main, "single_artist")), singleartist);
    }
    if (disc_title != NULL){
        trim_whitespace(disc_title);
        gtk_entry_set_text(GTK_ENTRY(album_title), disc_title);
    }

    if (disc_genre != NULL){
        trim_whitespace(disc_genre);
        gtk_entry_set_text(GTK_ENTRY(album_genre), disc_genre);
    }
    else{
        gtk_entry_set_text(GTK_ENTRY(album_genre), "Unknown");
    }

    if (!disc_year){
        disc_year = 1900;
    }

    char disc_year_char[5];
    snprintf(disc_year_char, 5, "%d", disc_year);
    gtk_entry_set_text(GTK_ENTRY(album_year), disc_year_char);

    gtk_toggle_button_set_active(			    	// lnr
        GTK_TOGGLE_BUTTON( lookup_widget( win_main, "single_genre" )), true );

    model = create_model_from_disc(disc);
    gtk_tree_view_set_model(GTK_TREE_VIEW(tracklist), model);
    g_object_unref(model);
}


void refresh(char * cdrom, int force)
{
    cddb_disc_t * disc;
    //GList * curr;

    if (working)
    /* don't do nothing */
        return;

    if (check_disc(cdrom) || force)
    {
        disc = read_disc(cdrom);
        if (disc == NULL)
            return;

        if (gbl_current_discid != cddb_disc_get_discid(disc))
        {
            /* only trash the user's inputs when the disc is new */

            gtk_widget_set_sensitive(lookup_widget(win_main, "rip_button"), TRUE);

            // show the temporary info
            gtk_entry_set_text(GTK_ENTRY(album_artist), "Unknown Artist");
            gtk_entry_set_text(GTK_ENTRY(album_title), "Unknown Album");
            update_tracklist(disc);
        }

        // clear out the previous list of matches
        /* this causes a segfault in the following scenario:
        - disable auto cddb lookup
        - insert cd that has a record in cddb
        - click 'cddb lookup'
        - wait for lookup to finish successfully and eject the disk
        - reinsert the disk
        - click 'cddb lookup'
        - it crashes in the following loop (legrand-sw 14 sep 2008)
        for (curr = g_list_first(gbl_disc_matches); curr != NULL; curr = g_list_next(curr))
        {
            cddb_disc_destroy((cddb_disc_t *)curr->data);
        }
        g_list_free(gbl_disc_matches);
        */

        if (!global_prefs->do_cddb_updates && !force)
            return;

        gbl_disc_matches = lookup_disc(disc);
        cddb_disc_destroy(disc);

        if (gbl_disc_matches == NULL)
            return;

        if (g_list_length(gbl_disc_matches) > 1)
        {
            // fill in and show the album drop-down box
            GtkListStore * store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
            GtkTreeIter iter;
            GList * curr;
            cddb_disc_t * tempdisc;

            for (curr = g_list_first(gbl_disc_matches); curr != NULL; curr = g_list_next(curr))
            {
                tempdisc = (cddb_disc_t *)curr->data;
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(store, &iter,
                    0, cddb_disc_get_artist(tempdisc),
                    1, cddb_disc_get_title(tempdisc),
                    -1);
            }
            gtk_combo_box_set_model(GTK_COMBO_BOX(pick_disc), GTK_TREE_MODEL(store));
            gtk_combo_box_set_active(GTK_COMBO_BOX(pick_disc), 1);
            gtk_combo_box_set_active(GTK_COMBO_BOX(pick_disc), 0);

            gtk_widget_show(lookup_widget(win_main, "disc"));
            gtk_widget_show(lookup_widget(win_main, "pick_disc"));
        }

        update_tracklist((cddb_disc_t *)g_list_nth_data(gbl_disc_matches, 0));
    }
}
