/*
Asunder

Copyright(C) 2005 Eric Lathrop <eric@ericlathrop.com>
Copyright(C) 2007 Andrew Smith <http://littlesvr.ca/misc/contactandrew.php>

Any code in this file may be redistributed or modified under the terms of
the GNU General Public Licence as published by the Free Software
Foundation; version 2 of the licence.

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "main.h"
#include "prefs.h"
#include "util.h"
#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "completion.h"

#define GLADE_HOOKUP_OBJECT(component, widget, name)                                  \
	g_object_set_data_full(G_OBJECT(component), name, g_object_ref(G_OBJECT(widget)), \
						   (GDestroyNotify)g_object_unref)

#define GLADE_HOOKUP_OBJECT_NO_REF(component, widget, name) \
	g_object_set_data(G_OBJECT(component), name, widget)

GtkWidget *create_main(void)
{
	GtkWidget *main_win;
	GtkWidget *vbox1;
	GtkWidget *toolbar1;
	GtkToolItem *lookup;
	GtkToolItem *preferences;
	GtkWidget *table2;
	GtkWidget *album_artist;
	GtkWidget *album_title;
	GtkWidget *pick_disc;
	GtkWidget *disc;
	GtkWidget *artist_label;
	GtkWidget *title_label;
	GtkWidget *single_artist;
	GtkWidget *scrolledwindow1;
	GtkWidget *tracklist;
	GtkWidget *rip_button;
	GtkWidget *alignment3;
	GtkWidget *hbox4;
	GtkWidget *image1;
	GtkWidget *label8;
	GtkWidget *hbox5;
	GtkWidget *fillerBox;
	GtkWidget *statusLbl;
	GtkWidget *album_genre; // lnr
	GtkWidget *genre_label; // lnr
	GtkWidget *single_genre; // lnr
	GtkWidget *icon;

	main_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(main_win), "Asunder");

	gtk_window_set_default_size(GTK_WINDOW(main_win), 600, 450);

	vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show(vbox1);
	gtk_container_add(GTK_CONTAINER(main_win), vbox1);

	toolbar1 = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar1), GTK_TOOLBAR_BOTH_HORIZ);
	gtk_widget_show(toolbar1);
	gtk_box_pack_start(GTK_BOX(vbox1), toolbar1, FALSE, FALSE, 0);

	icon = gtk_image_new_from_icon_name("view-refresh",
										gtk_toolbar_get_icon_size(GTK_TOOLBAR(toolbar1)));
	gtk_widget_show(icon);
	lookup = gtk_tool_button_new(icon, _("CDDB Lookup"));
	gtk_tool_item_set_is_important(lookup, TRUE);
	gtk_widget_show(GTK_WIDGET(lookup));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar1), lookup, 0);

	icon = gtk_image_new_from_icon_name("gtk-preferences",
										gtk_toolbar_get_icon_size(GTK_TOOLBAR(toolbar1)));
	gtk_widget_show(icon);
	preferences = gtk_tool_button_new(icon, "Preferences");
	gtk_tool_item_set_is_important(preferences, TRUE);
	gtk_widget_show(GTK_WIDGET(preferences));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar1), preferences, 0);

	table2 = gtk_table_new(3, 3, FALSE);
	gtk_widget_show(table2);
	gtk_box_pack_start(GTK_BOX(vbox1), table2, FALSE, TRUE, 3);

	album_artist = gtk_entry_new();
	create_completion(album_artist, "album_artist");
	gtk_widget_show(album_artist);
	gtk_table_attach(GTK_TABLE(table2), album_artist, 1, 2, 1, 2,
					 (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), (GtkAttachOptions)(0), 0, 0);

	album_title = gtk_entry_new();
	gtk_widget_show(album_title);
	gtk_table_attach(GTK_TABLE(table2), album_title, 1, 2, 2, 3,
					 (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), (GtkAttachOptions)(0), 0, 0);

	pick_disc = gtk_combo_box_new();
	gtk_table_attach(GTK_TABLE(table2), pick_disc, 1, 2, 0, 1, (GtkAttachOptions)(GTK_FILL),
					 (GtkAttachOptions)(GTK_FILL), 0, 0);

	album_genre = gtk_entry_new();
	create_completion(album_genre, "album_genre");
	gtk_widget_show(album_genre);
	gtk_table_attach(GTK_TABLE(table2), album_genre, 1, 2, 3, 4,
					 (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), (GtkAttachOptions)(0), 0, 0);

	disc = gtk_label_new(_("Disc:"));
	gtk_table_attach(GTK_TABLE(table2), disc, 0, 1, 0, 1, (GtkAttachOptions)(GTK_FILL),
					 (GtkAttachOptions)(0), 3, 0);
	gtk_misc_set_alignment(GTK_MISC(disc), 0, 0.49);

	artist_label = gtk_label_new(_("Album Artist:"));
	gtk_misc_set_alignment(GTK_MISC(artist_label), 0, 0);
	gtk_widget_show(artist_label);
	gtk_table_attach(GTK_TABLE(table2), artist_label, 0, 1, 1, 2, (GtkAttachOptions)(GTK_FILL),
					 (GtkAttachOptions)(0), 3, 0);

	title_label = gtk_label_new(_("Album Title:"));
	gtk_misc_set_alignment(GTK_MISC(title_label), 0, 0);
	gtk_widget_show(title_label);
	gtk_table_attach(GTK_TABLE(table2), title_label, 0, 1, 2, 3, (GtkAttachOptions)(GTK_FILL),
					 (GtkAttachOptions)(0), 3, 0);

	single_artist = gtk_check_button_new_with_mnemonic(_("Single Artist"));
	gtk_widget_show(single_artist);
	gtk_table_attach(GTK_TABLE(table2), single_artist, 2, 3, 1, 2, (GtkAttachOptions)(GTK_FILL),
					 (GtkAttachOptions)(0), 3, 0);

	genre_label = gtk_label_new(_("Genre / Year:")); // lnr
	gtk_misc_set_alignment(GTK_MISC(genre_label), 0, 0);
	gtk_widget_show(genre_label);
	gtk_table_attach(GTK_TABLE(table2), genre_label, 0, 1, 3, 4, (GtkAttachOptions)(GTK_FILL),
					 (GtkAttachOptions)(0), 3, 0);

	single_genre = gtk_check_button_new_with_mnemonic(_("Single Genre")); // lnr
	//~ gtk_widget_show( single_genre );
	//~ gtk_table_attach( GTK_TABLE( table2 ), single_genre, 2, 3, 3, 4,
	//~ (GtkAttachOptions) ( GTK_FILL ),
	//~ (GtkAttachOptions) (0), 3, 0);

	GtkWidget *album_year = gtk_entry_new();
	gtk_widget_show(album_year);
	gtk_table_attach(GTK_TABLE(table2), album_year, 2, 3, 3, 4, (GtkAttachOptions)(GTK_FILL),
					 (GtkAttachOptions)(0), 3, 0);

	scrolledwindow1 = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolledwindow1);
	gtk_box_pack_start(GTK_BOX(vbox1), scrolledwindow1, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow1), GTK_POLICY_AUTOMATIC,
								   GTK_POLICY_AUTOMATIC);

	tracklist = gtk_tree_view_new();
	gtk_widget_show(tracklist);
	gtk_container_add(GTK_CONTAINER(scrolledwindow1), tracklist);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tracklist), TRUE);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tracklist), FALSE);

	hbox5 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox5, FALSE, TRUE, 5);
	gtk_widget_show(hbox5);

	statusLbl = gtk_label_new("");
	gtk_label_set_use_markup(GTK_LABEL(statusLbl), TRUE);
	gtk_misc_set_alignment(GTK_MISC(statusLbl), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox5), statusLbl, TRUE, TRUE, 0);
	gtk_widget_show(statusLbl);

	fillerBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(hbox5), fillerBox, TRUE, TRUE, 0);
	gtk_widget_show(hbox5);

	rip_button = gtk_button_new();
	gtk_widget_show(rip_button);
	gtk_box_pack_start(GTK_BOX(hbox5), rip_button, FALSE, FALSE, 5);

	alignment3 = gtk_alignment_new(0.5, 0.5, 0, 0);
	gtk_widget_show(alignment3);
	gtk_container_add(GTK_CONTAINER(rip_button), alignment3);

	hbox4 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_widget_show(hbox4);
	gtk_container_add(GTK_CONTAINER(alignment3), hbox4);

	image1 = gtk_image_new_from_icon_name("media-optical", GTK_ICON_SIZE_BUTTON);
	gtk_widget_show(image1);
	gtk_box_pack_start(GTK_BOX(hbox4), image1, FALSE, FALSE, 0);

	label8 = gtk_label_new_with_mnemonic(_("Rip"));
	gtk_widget_show(label8);
	gtk_box_pack_start(GTK_BOX(hbox4), label8, FALSE, FALSE, 0);

	g_signal_connect((gpointer)main_win, "delete_event", G_CALLBACK(on_window_close), NULL);

	g_signal_connect((gpointer)tracklist, "button-press-event",
					 G_CALLBACK(on_tracklist_mouse_click), NULL);

	g_signal_connect((gpointer)lookup, "clicked", G_CALLBACK(on_lookup_clicked), NULL);
	g_signal_connect((gpointer)preferences, "clicked", G_CALLBACK(on_preferences_clicked), NULL);
	g_signal_connect((gpointer)album_artist, "focus_out_event",
					 G_CALLBACK(on_album_artist_focus_out_event), NULL);
	g_signal_connect((gpointer)album_title, "focus_out_event",
					 G_CALLBACK(on_album_title_focus_out_event), NULL);
	g_signal_connect((gpointer)pick_disc, "changed", G_CALLBACK(on_pick_disc_changed), NULL);
	g_signal_connect((gpointer)single_artist, "toggled", G_CALLBACK(on_single_artist_toggled),
					 NULL);
	g_signal_connect((gpointer)rip_button, "clicked", G_CALLBACK(on_rip_button_clicked), NULL);
	g_signal_connect((gpointer)album_genre, "focus_out_event", // lnr
					 G_CALLBACK(on_album_genre_focus_out_event), NULL);
	g_signal_connect((gpointer)single_genre, "toggled", G_CALLBACK(on_single_genre_toggled), NULL);
	g_signal_connect((gpointer)album_year, "focus_out_event", G_CALLBACK(on_year_focus_out_event),
					 NULL);

	/* KEYBOARD accelerators */
	GtkAccelGroup *accelGroup;
	guint accelKey;
	GdkModifierType accelModifier;
	GClosure *closure = NULL;

	accelGroup = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(main_win), accelGroup);

	gtk_accelerator_parse("<Control>W", &accelKey, &accelModifier);
	closure = g_cclosure_new(G_CALLBACK(on_window_close), NULL, NULL);
	gtk_accel_group_connect(accelGroup, accelKey, accelModifier, GTK_ACCEL_VISIBLE, closure);

	gtk_accelerator_parse("<Control>Q", &accelKey, &accelModifier);
	closure = g_cclosure_new(G_CALLBACK(on_window_close), NULL, NULL);
	gtk_accel_group_connect(accelGroup, accelKey, accelModifier, GTK_ACCEL_VISIBLE, closure);

	gtk_accelerator_parse("F2", &accelKey, &accelModifier);
	closure = g_cclosure_new(G_CALLBACK(on_press_f2), NULL, NULL);
	gtk_accel_group_connect(accelGroup, accelKey, accelModifier, GTK_ACCEL_VISIBLE, closure);
	/* END KEYBOARD accelerators */

	/* Store pointers to all widgets, for use by lookup_widget(). */
	GLADE_HOOKUP_OBJECT_NO_REF(main_win, main_win, "main");
	GLADE_HOOKUP_OBJECT(main_win, toolbar1, "toolbar1");
	GLADE_HOOKUP_OBJECT(main_win, lookup, "lookup");
	GLADE_HOOKUP_OBJECT(main_win, preferences, "preferences");
	GLADE_HOOKUP_OBJECT(main_win, table2, "table2");
	GLADE_HOOKUP_OBJECT(main_win, album_artist, "album_artist");
	GLADE_HOOKUP_OBJECT(main_win, album_title, "album_title");
	GLADE_HOOKUP_OBJECT(main_win, pick_disc, "pick_disc");
	GLADE_HOOKUP_OBJECT(main_win, disc, "disc");
	GLADE_HOOKUP_OBJECT(main_win, artist_label, "artist_label");
	GLADE_HOOKUP_OBJECT(main_win, title_label, "title_label");
	GLADE_HOOKUP_OBJECT(main_win, single_artist, "single_artist");
	GLADE_HOOKUP_OBJECT(main_win, scrolledwindow1, "scrolledwindow1");
	GLADE_HOOKUP_OBJECT(main_win, tracklist, "tracklist");
	GLADE_HOOKUP_OBJECT(main_win, rip_button, "rip_button");
	GLADE_HOOKUP_OBJECT(main_win, statusLbl, "statusLbl");
	GLADE_HOOKUP_OBJECT(main_win, album_genre, "album_genre"); // lnr
	GLADE_HOOKUP_OBJECT(main_win, genre_label, "genre_label"); // lnr
	GLADE_HOOKUP_OBJECT(main_win, single_genre, "single_genre"); // lnr
	GLADE_HOOKUP_OBJECT(main_win, album_year, "album_year");

	return main_win;
}

GtkWidget *create_prefs(void)
{
	GtkWidget *label;
	GtkWidget *prefs;
	GtkWidget *notebook1;
	GtkWidget *vbox;
	GtkWidget *music_dir;
	GtkWidget *make_playlist;
	GtkWidget *hbox12;
	GtkWidget *cdrom;
	GtkWidget *frame2;
	GtkWidget *table1;
	GtkWidget *format_music;
	GtkWidget *format_albumdir;
	GtkWidget *format_playlist;
	GtkWidget *rip_wav;
	GtkWidget *dialog_action_area1;
	GtkWidget *cancelbutton1;
	GtkWidget *okbutton1;
	GtkWidget *eject_on_done;
	GtkWidget *hboxFill;

	prefs = gtk_dialog_new();
	gtk_window_set_transient_for(GTK_WINDOW(prefs), GTK_WINDOW(win_main));
	gtk_window_set_title(GTK_WINDOW(prefs), _("Preferences"));
	gtk_window_set_modal(GTK_WINDOW(prefs), TRUE);
	gtk_window_set_type_hint(GTK_WINDOW(prefs), GDK_WINDOW_TYPE_HINT_DIALOG);

	vbox = gtk_dialog_get_content_area(GTK_DIALOG(prefs));
	gtk_widget_show(vbox);

	notebook1 = gtk_notebook_new();
	gtk_widget_show(notebook1);
	gtk_box_pack_start(GTK_BOX(vbox), notebook1, TRUE, TRUE, 0);

	/* GENERAL tab */
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(notebook1), vbox);

	label = gtk_label_new(_("Destination folder"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);

	music_dir =
		gtk_file_chooser_button_new(_("Destination folder"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
	gtk_widget_show(music_dir);
	gtk_box_pack_start(GTK_BOX(vbox), music_dir, FALSE, FALSE, 0);

	make_playlist = gtk_check_button_new_with_mnemonic(_("Create M3U playlist"));
	gtk_widget_show(make_playlist);
	gtk_box_pack_start(GTK_BOX(vbox), make_playlist, FALSE, FALSE, 0);

	hbox12 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show(hbox12);
	gtk_box_pack_start(GTK_BOX(vbox), hbox12, FALSE, FALSE, 0);

	label = gtk_label_new(_("CD-ROM device: "));
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox12), label, FALSE, FALSE, 0);

	cdrom = gtk_entry_new();
	gtk_widget_show(cdrom);
	gtk_box_pack_start(GTK_BOX(hbox12), cdrom, TRUE, TRUE, 0);
	gtk_widget_set_tooltip_text(cdrom, _("Default: /dev/cdrom\n"
										 "Other example: /dev/hdc\n"
										 "Other example: /dev/sr0"));

	eject_on_done = gtk_check_button_new_with_mnemonic(_("Eject disc when finished"));
	gtk_widget_show(eject_on_done);
	gtk_box_pack_start(GTK_BOX(vbox), eject_on_done, FALSE, FALSE, 5);

	hboxFill = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show(hboxFill);
	gtk_box_pack_start(GTK_BOX(vbox), hboxFill, TRUE, TRUE, 0);

	label = gtk_label_new(_("General"));
	gtk_widget_show(label);
	gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook1),
							   gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook1), 0), label);
	/* END GENERAL tab */

	/* FILENAMES tab */
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(notebook1), vbox);

	frame2 = gtk_frame_new(NULL);
	gtk_widget_show(frame2);
	gtk_box_pack_start(GTK_BOX(vbox), frame2, FALSE, FALSE, 0);

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(frame2), vbox);

	label = gtk_label_new(_(
		"%A - Artist\n%L - Album\n%N - Track number (2-digit)\n%Y - Year (4-digit or \"0\")\n%T - Song title\n%G - Genre"));
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);

	// problem is that the same albumdir is used (threads.c) for all formats
	//~ label = gtk_label_new (_("%F - Format (e.g. FLAC)"));
	//~ gtk_widget_show (label);
	//~ gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
	//~ gtk_misc_set_alignment (GTK_MISC (label), 0, 0);

	table1 = gtk_grid_new();
	gtk_grid_set_column_spacing(GTK_GRID(table1), 5);
	gtk_widget_show(table1);
	gtk_box_pack_start(GTK_BOX(vbox), table1, TRUE, TRUE, 0);

	label = gtk_label_new(_("Album directory:"));
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_widget_show(label);
	gtk_grid_attach(GTK_GRID(table1), label, 0, 0, 1, 1);

	label = gtk_label_new(_("Playlist file:"));
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_widget_show(label);
	gtk_grid_attach(GTK_GRID(table1), label, 0, 1, 1, 1);

	label = gtk_label_new(_("Music file:"));
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_widget_show(label);
	gtk_grid_attach(GTK_GRID(table1), label, 0, 2, 1, 1);

	format_albumdir = gtk_entry_new();
	gtk_widget_set_hexpand(format_albumdir, TRUE);
	gtk_widget_show(format_albumdir);
	gtk_grid_attach(GTK_GRID(table1), format_albumdir, 1, 0, 1, 1);
	gtk_widget_set_tooltip_text(
		format_albumdir, _("This is relative to the destination folder (from the General tab).\n"
						   "Can be blank.\n"
						   "Default: %A - %L\n"
						   "Other example: %A/%L"));

	format_playlist = gtk_entry_new();
	gtk_widget_set_hexpand(format_playlist, TRUE);
	gtk_widget_show(format_playlist);
	gtk_grid_attach(GTK_GRID(table1), format_playlist, 1, 1, 1, 1);
	gtk_widget_set_tooltip_text(format_playlist, _("This will be stored in the album directory.\n"
												   "Can be blank.\n"
												   "Default: %A - %L"));

	format_music = gtk_entry_new();
	gtk_widget_set_hexpand(format_music, TRUE);
	gtk_widget_show(format_music);
	gtk_grid_attach(GTK_GRID(table1), format_music, 1, 2, 1, 1);
	gtk_widget_set_tooltip_text(format_music, _("This will be stored in the album directory.\n"
												"Cannot be blank.\n"
												"Default: %A - %T\n"
												"Other example: %N - %T"));

	label = gtk_label_new(_("Filename formats"));
	gtk_widget_show(label);
	gtk_frame_set_label_widget(GTK_FRAME(frame2), label);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);

	label = gtk_label_new(_("Filenames"));
	gtk_widget_show(label);
	gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook1),
							   gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook1), 1), label);
	/* END FILENAMES tab */

	/* ENCODE tab */
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(notebook1), vbox);

	/* WAV */
	rip_wav = gtk_check_button_new_with_mnemonic(_("WAV (uncompressed)"));
	gtk_widget_show(rip_wav);
	gtk_box_pack_start(GTK_BOX(vbox), rip_wav, FALSE, FALSE, 0);
	/* END WAV */

	/* MP3 */
	GtkWidget *mp3_frame;
	GtkWidget *mp3_box;
	GtkWidget *mp3_vbr;
	GtkWidget *mp3_bitrate_box;
	GtkWidget *mp3_bitrate_label;
	GtkWidget *mp3_bitrate;
	GtkWidget *rip_mp3;

	mp3_frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(mp3_frame), GTK_SHADOW_IN);
	gtk_widget_show(mp3_frame);
	gtk_box_pack_start(GTK_BOX(vbox), mp3_frame, FALSE, FALSE, 0);

	mp3_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	GLADE_HOOKUP_OBJECT(prefs, mp3_box, "mp3_box");
	gtk_widget_set_margin_start(mp3_box, 10);
	gtk_widget_set_margin_end(mp3_box, 10);
	gtk_widget_show(mp3_box);
	gtk_container_add(GTK_CONTAINER(mp3_frame), mp3_box);

	mp3_vbr = gtk_check_button_new_with_mnemonic(_("Variable bit rate (VBR)"));
	GLADE_HOOKUP_OBJECT(prefs, mp3_vbr, "mp3_vbr");
	gtk_widget_show(mp3_vbr);
	gtk_box_pack_start(GTK_BOX(mp3_box), mp3_vbr, FALSE, FALSE, 0);
	g_signal_connect((gpointer)mp3_vbr, "toggled", G_CALLBACK(on_vbr_toggled), NULL);
	gtk_widget_set_tooltip_text(mp3_vbr, _("Better quality for the same size."));

	mp3_bitrate_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_tooltip_text(
		mp3_bitrate_box,
		_("Higher bitrate is better quality but also bigger file. Most people use 192Kbps."));
	gtk_widget_show(mp3_bitrate_box);
	gtk_box_pack_start(GTK_BOX(mp3_box), mp3_bitrate_box, TRUE, TRUE, 0);

	mp3_bitrate_label = gtk_label_new(_("Bitrate"));
	gtk_widget_show(mp3_bitrate_label);
	gtk_box_pack_start(GTK_BOX(mp3_bitrate_box), mp3_bitrate_label, FALSE, FALSE, 0);

	mp3_bitrate = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, gtk_adjustment_new(0, 0, 14, 1, 1, 1));
	GLADE_HOOKUP_OBJECT(prefs, mp3_bitrate, "mp3_bitrate");
	gtk_widget_show(mp3_bitrate);
	gtk_box_pack_start(GTK_BOX(mp3_bitrate_box), mp3_bitrate, TRUE, TRUE, 5);
	gtk_scale_set_draw_value(GTK_SCALE(mp3_bitrate), FALSE);
	gtk_scale_set_digits(GTK_SCALE(mp3_bitrate), 0);
	g_signal_connect((gpointer)mp3_bitrate, "value_changed",
					 G_CALLBACK(on_mp3bitrate_value_changed), NULL);

	char kbps_text[10];
	snprintf(kbps_text, 10, _("%dKbps"), 32);
	label = gtk_label_new(kbps_text);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(mp3_bitrate_box), label, FALSE, FALSE, 0);
	GLADE_HOOKUP_OBJECT(prefs, label, "bitrate_lbl_2");

	rip_mp3 = gtk_check_button_new_with_mnemonic(_("MP3 (lossy compression)"));
	GLADE_HOOKUP_OBJECT(prefs, rip_mp3, "rip_mp3");
	gtk_widget_show(rip_mp3);
	gtk_frame_set_label_widget(GTK_FRAME(mp3_frame), rip_mp3);
	g_signal_connect((gpointer)rip_mp3, "toggled", G_CALLBACK(on_rip_mp3_toggled), NULL);
	/* END MP3 */

	/* OGG */
	GtkWidget *ogg_frame;
	GtkWidget *ogg_quality_box;
	GtkWidget *ogg_quality_label;
	GtkWidget *ogg_quality;
	GtkWidget *rip_ogg;

	ogg_frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(ogg_frame), GTK_SHADOW_IN);
	gtk_widget_show(ogg_frame);
	gtk_box_pack_start(GTK_BOX(vbox), ogg_frame, FALSE, FALSE, 0);

	ogg_quality_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GLADE_HOOKUP_OBJECT(prefs, ogg_quality_box, "ogg_box");
	gtk_widget_set_margin_start(ogg_quality_box, 10);
	gtk_widget_set_margin_end(ogg_quality_box, 10);
	gtk_widget_set_tooltip_text(
		ogg_quality_box,
		_("Higher quality means bigger file. Default is " STR(PREFS_DEFAULT_OGG_QUALITY) "."));
	gtk_widget_show(ogg_quality_box);
	gtk_container_add(GTK_CONTAINER(ogg_frame), ogg_quality_box);

	ogg_quality_label = gtk_label_new(_("Quality"));
	gtk_widget_show(ogg_quality_label);
	gtk_box_pack_start(GTK_BOX(ogg_quality_box), ogg_quality_label, FALSE, FALSE, 0);

	ogg_quality = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL,
								gtk_adjustment_new(PREFS_DEFAULT_OGG_QUALITY, 0, 11, 1, 1, 1));
	GLADE_HOOKUP_OBJECT(prefs, ogg_quality, "ogg_quality");
	gtk_widget_show(ogg_quality);
	gtk_box_pack_start(GTK_BOX(ogg_quality_box), ogg_quality, TRUE, TRUE, 5);
	gtk_scale_set_value_pos(GTK_SCALE(ogg_quality), GTK_POS_RIGHT);
	gtk_scale_set_digits(GTK_SCALE(ogg_quality), 0);

	rip_ogg = gtk_check_button_new_with_mnemonic(_("OGG Vorbis (lossy compression)"));
	GLADE_HOOKUP_OBJECT(prefs, rip_ogg, "rip_ogg");
	gtk_widget_show(rip_ogg);
	gtk_frame_set_label_widget(GTK_FRAME(ogg_frame), rip_ogg);
	g_signal_connect((gpointer)rip_ogg, "toggled", G_CALLBACK(on_rip_ogg_toggled), NULL);
	/* END OGG */

	/* FLAC */
	GtkWidget *flac_frame;
	GtkWidget *flac_compression_box;
	GtkWidget *flac_compression_label;
	GtkWidget *flac_compression;
	GtkWidget *rip_flac;

	flac_frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(flac_frame), GTK_SHADOW_IN);
	gtk_widget_show(flac_frame);
	gtk_box_pack_start(GTK_BOX(vbox), flac_frame, FALSE, FALSE, 0);

	flac_compression_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GLADE_HOOKUP_OBJECT(prefs, flac_compression_box, "flac_box");
	gtk_widget_set_margin_start(flac_compression_box, 10);
	gtk_widget_set_margin_end(flac_compression_box, 10);
	gtk_widget_set_tooltip_text(
		flac_compression_box,
		_("This does not affect the quality. Higher number means smaller file."));
	gtk_widget_show(flac_compression_box);
	gtk_container_add(GTK_CONTAINER(flac_frame), flac_compression_box);

	flac_compression_label = gtk_label_new(_("Compression level"));
	gtk_widget_show(flac_compression_label);
	gtk_box_pack_start(GTK_BOX(flac_compression_box), flac_compression_label, FALSE, FALSE, 0);

	flac_compression =
		gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, gtk_adjustment_new(0, 0, 9, 1, 1, 1));
	GLADE_HOOKUP_OBJECT(prefs, flac_compression, "flac_compression");
	gtk_widget_show(flac_compression);
	gtk_box_pack_start(GTK_BOX(flac_compression_box), flac_compression, TRUE, TRUE, 5);
	gtk_scale_set_value_pos(GTK_SCALE(flac_compression), GTK_POS_RIGHT);
	gtk_scale_set_digits(GTK_SCALE(flac_compression), 0);

	rip_flac = gtk_check_button_new_with_mnemonic(_("FLAC (lossless compression)"));
	GLADE_HOOKUP_OBJECT(prefs, rip_flac, "rip_flac");
	gtk_widget_show(rip_flac);
	gtk_frame_set_label_widget(GTK_FRAME(flac_frame), rip_flac);
	g_signal_connect((gpointer)rip_flac, "toggled", G_CALLBACK(on_rip_flac_toggled), NULL);
	/* END FLAC */

	GtkWidget *expander;
	GtkWidget *hiddenbox;

	expander = gtk_expander_new(_("More formats"));
	gtk_widget_show(expander);
	gtk_box_pack_start(GTK_BOX(vbox), expander, FALSE, FALSE, 0);
	GLADE_HOOKUP_OBJECT(prefs, expander, "more_formats_expander");

	hiddenbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show(hiddenbox);
	gtk_container_add(GTK_CONTAINER(expander), hiddenbox);

	/* OPUS */
	GtkWidget *opus_frame;
	GtkWidget *opus_bitrate_box;
	GtkWidget *opus_bitrate_label;
	GtkWidget *opus_bitrate;
	GtkWidget *rip_opus;

	opus_frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(opus_frame), GTK_SHADOW_IN);
	gtk_widget_show(opus_frame);
	gtk_box_pack_start(GTK_BOX(hiddenbox), opus_frame, FALSE, FALSE, 0);

	opus_bitrate_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GLADE_HOOKUP_OBJECT(prefs, opus_bitrate_box, "opus_box");
	gtk_widget_set_margin_start(opus_bitrate_box, 10);
	gtk_widget_set_margin_end(opus_bitrate_box, 10);
	gtk_widget_set_tooltip_text(
		opus_bitrate_box,
		_("Higher bitrate is better quality but also bigger file. Most people use 160Kbps."));
	gtk_widget_show(opus_bitrate_box);
	gtk_container_add(GTK_CONTAINER(opus_frame), opus_bitrate_box);

	opus_bitrate_label = gtk_label_new(_("Bitrate"));
	gtk_widget_show(opus_bitrate_label);
	gtk_box_pack_start(GTK_BOX(opus_bitrate_box), opus_bitrate_label, FALSE, FALSE, 0);

	opus_bitrate = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, gtk_adjustment_new(0, 0, 13, 1, 1, 1));
	GLADE_HOOKUP_OBJECT(prefs, opus_bitrate, "opus_bitrate");
	gtk_widget_show(opus_bitrate);
	gtk_box_pack_start(GTK_BOX(opus_bitrate_box), opus_bitrate, TRUE, TRUE, 5);
	gtk_scale_set_digits(GTK_SCALE(opus_bitrate), 0);
	gtk_scale_set_value_pos(GTK_SCALE(opus_bitrate), GTK_POS_RIGHT);
	g_signal_connect((gpointer)opus_bitrate, "format-value", G_CALLBACK(format_opus_bitrate), NULL);

	rip_opus = gtk_check_button_new_with_mnemonic(_("OPUS (lossy compression)"));
	GLADE_HOOKUP_OBJECT(prefs, rip_opus, "rip_opus");
	gtk_widget_show(rip_opus);
	gtk_frame_set_label_widget(GTK_FRAME(opus_frame), rip_opus);
	g_signal_connect((gpointer)rip_opus, "toggled", G_CALLBACK(on_rip_opus_toggled), NULL);
	/* END OPUS */

	/* WAVPACK */
	GtkWidget *wavpack_frame;
	GtkWidget *wavpack_box;
	GtkWidget *wavpack_compression_box;
	GtkWidget *wavpack_compression_label;
	GtkWidget *wavpack_compression;
	GtkWidget *wavpack_hybrid;
	GtkWidget *wavpack_hybrid_frame;
	GtkWidget *wavpack_bitrate_box;
	GtkWidget *wavpack_bitrate_label;
	GtkWidget *wavpack_bitrate;
	GtkWidget *rip_wavpack;

	wavpack_frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(wavpack_frame), GTK_SHADOW_IN);
	gtk_widget_show(wavpack_frame);
	gtk_box_pack_start(GTK_BOX(hiddenbox), wavpack_frame, FALSE, FALSE, 0);

	wavpack_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	GLADE_HOOKUP_OBJECT(prefs, wavpack_box, "wavpack_box");
	gtk_widget_set_margin_start(wavpack_box, 10);
	gtk_widget_set_margin_end(wavpack_box, 10);
	gtk_widget_set_margin_bottom(wavpack_box, 5);
	gtk_widget_show(wavpack_box);
	gtk_container_add(GTK_CONTAINER(wavpack_frame), wavpack_box);

	wavpack_compression_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_tooltip_text(
		wavpack_compression_box,
		_("This does not affect the quality. Higher number means smaller file. Default is 1 (recommended)."));
	gtk_widget_show(wavpack_compression_box);
	gtk_box_pack_start(GTK_BOX(wavpack_box), wavpack_compression_box, FALSE, FALSE, 0);

	wavpack_compression_label = gtk_label_new(_("Compression level"));
	gtk_widget_show(wavpack_compression_label);
	gtk_box_pack_start(GTK_BOX(wavpack_compression_box), wavpack_compression_label, FALSE, FALSE,
					   0);

	wavpack_compression =
		gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, gtk_adjustment_new(1, 0, 4, 1, 1, 1));
	GLADE_HOOKUP_OBJECT(prefs, wavpack_compression, "wavpack_compression");
	gtk_widget_show(wavpack_compression);
	gtk_box_pack_start(GTK_BOX(wavpack_compression_box), wavpack_compression, TRUE, TRUE, 5);
	gtk_scale_set_digits(GTK_SCALE(wavpack_compression), 0);
	gtk_scale_set_value_pos(GTK_SCALE(wavpack_compression), GTK_POS_RIGHT);

	wavpack_hybrid_frame = gtk_frame_new(NULL);
	gtk_widget_show(wavpack_hybrid_frame);
	gtk_box_pack_start(GTK_BOX(wavpack_box), wavpack_hybrid_frame, FALSE, FALSE, 0);

	wavpack_hybrid = gtk_check_button_new_with_mnemonic(_("Hybrid compression"));
	GLADE_HOOKUP_OBJECT(prefs, wavpack_hybrid, "wavpack_hybrid");
	gtk_widget_set_tooltip_text(
		wavpack_hybrid,
		_("The format is lossy but a correction file is created for restoring the lossless original."));
	gtk_widget_show(wavpack_hybrid);
	gtk_frame_set_label_widget(GTK_FRAME(wavpack_hybrid_frame), wavpack_hybrid);
	g_signal_connect((gpointer)wavpack_hybrid, "toggled", G_CALLBACK(on_hybrid_toggled), NULL);

	wavpack_bitrate_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GLADE_HOOKUP_OBJECT(prefs, wavpack_bitrate_box, "wavpack_hybrid_box");
	gtk_widget_set_margin_start(wavpack_bitrate_box, 10);
	gtk_widget_set_margin_end(wavpack_bitrate_box, 10);
	gtk_widget_show(wavpack_bitrate_box);
	gtk_container_add(GTK_CONTAINER(wavpack_hybrid_frame), wavpack_bitrate_box);

	wavpack_bitrate_label = gtk_label_new(_("Bitrate"));
	gtk_widget_show(wavpack_bitrate_label);
	gtk_box_pack_start(GTK_BOX(wavpack_bitrate_box), wavpack_bitrate_label, FALSE, FALSE, 2);

	wavpack_bitrate =
		gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, gtk_adjustment_new(0, 0, 6, 1, 1, 1));
	GLADE_HOOKUP_OBJECT(prefs, wavpack_bitrate, "wavpack_bitrate");
	gtk_widget_show(wavpack_bitrate);
	gtk_box_pack_start(GTK_BOX(wavpack_bitrate_box), wavpack_bitrate, TRUE, TRUE, 5);
	gtk_scale_set_digits(GTK_SCALE(wavpack_bitrate), 0);
	gtk_scale_set_value_pos(GTK_SCALE(wavpack_bitrate), GTK_POS_RIGHT);
	g_signal_connect((gpointer)wavpack_bitrate, "format-value", G_CALLBACK(format_wavpack_bitrate),
					 NULL);

	rip_wavpack = gtk_check_button_new_with_mnemonic("WavPack");
	GLADE_HOOKUP_OBJECT(prefs, rip_wavpack, "rip_wavpack");
	gtk_widget_show(rip_wavpack);
	gtk_frame_set_label_widget(GTK_FRAME(wavpack_frame), rip_wavpack);
	g_signal_connect((gpointer)rip_wavpack, "toggled", G_CALLBACK(on_rip_wavpack_toggled), NULL);
	/* END WAVPACK */

	/* MUSEPACK */
	GtkWidget *musepack_frame;
	GtkWidget *musepack_bitrate_box;
	GtkWidget *musepack_bitrate_label;
	GtkWidget *musepack_bitrate;
	GtkWidget *rip_musepack;

	musepack_frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(musepack_frame), GTK_SHADOW_IN);
	gtk_widget_show(musepack_frame);
	gtk_box_pack_start(GTK_BOX(hiddenbox), musepack_frame, FALSE, FALSE, 0);

	musepack_bitrate_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GLADE_HOOKUP_OBJECT(prefs, musepack_bitrate_box, "musepack_box");
	gtk_widget_set_margin_start(musepack_bitrate_box, 10);
	gtk_widget_set_margin_end(musepack_bitrate_box, 10);
	gtk_widget_set_tooltip_text(musepack_bitrate_box,
								_("Higher bitrate is better quality but also bigger file."));
	gtk_widget_show(musepack_bitrate_box);
	gtk_container_add(GTK_CONTAINER(musepack_frame), musepack_bitrate_box);

	musepack_bitrate_label = gtk_label_new(_("Bitrate"));
	gtk_widget_show(musepack_bitrate_label);
	gtk_box_pack_start(GTK_BOX(musepack_bitrate_box), musepack_bitrate_label, FALSE, FALSE, 0);

	musepack_bitrate =
		gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, gtk_adjustment_new(0, 0, 4, 1, 1, 1));
	GLADE_HOOKUP_OBJECT(prefs, musepack_bitrate, "musepack_bitrate");
	gtk_widget_show(musepack_bitrate);
	gtk_box_pack_start(GTK_BOX(musepack_bitrate_box), musepack_bitrate, TRUE, TRUE, 5);
	gtk_scale_set_digits(GTK_SCALE(musepack_bitrate), 0);
	gtk_scale_set_value_pos(GTK_SCALE(musepack_bitrate), GTK_POS_RIGHT);
	g_signal_connect((gpointer)musepack_bitrate, "format-value",
					 G_CALLBACK(format_musepack_bitrate), NULL);

	rip_musepack = gtk_check_button_new_with_mnemonic(_("Musepack (lossy compression)"));
	GLADE_HOOKUP_OBJECT(prefs, rip_musepack, "rip_musepack");
	gtk_widget_show(rip_musepack);
	gtk_frame_set_label_widget(GTK_FRAME(musepack_frame), rip_musepack);
	g_signal_connect((gpointer)rip_musepack, "toggled", G_CALLBACK(on_rip_musepack_toggled), NULL);
	/* END MUSEPACK */

	/* MONKEY */
	GtkWidget *monkey_frame;
	GtkWidget *monkey_compression_box;
	GtkWidget *monkey_compression_label;
	GtkWidget *monkey_compression;
	GtkWidget *rip_monkey;

	monkey_frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(monkey_frame), GTK_SHADOW_IN);
	gtk_widget_show(monkey_frame);
	gtk_box_pack_start(GTK_BOX(hiddenbox), monkey_frame, FALSE, FALSE, 0);

	monkey_compression_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GLADE_HOOKUP_OBJECT(prefs, monkey_compression_box, "monkey_box");
	gtk_widget_set_margin_start(monkey_compression_box, 10);
	gtk_widget_set_margin_end(monkey_compression_box, 10);
	gtk_widget_set_tooltip_text(
		monkey_compression_box,
		_("This does not affect the quality. Higher number means smaller file."));
	gtk_widget_show(monkey_compression_box);
	gtk_container_add(GTK_CONTAINER(monkey_frame), monkey_compression_box);

	monkey_compression_label = gtk_label_new(_("Compression level"));
	gtk_widget_show(monkey_compression_label);
	gtk_box_pack_start(GTK_BOX(monkey_compression_box), monkey_compression_label, FALSE, FALSE, 0);

	monkey_compression =
		gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, gtk_adjustment_new(0, 0, 5, 1, 1, 1));
	GLADE_HOOKUP_OBJECT(prefs, monkey_compression, "monkey_compression");
	gtk_widget_show(monkey_compression);
	gtk_box_pack_start(GTK_BOX(monkey_compression_box), monkey_compression, TRUE, TRUE, 5);
	gtk_scale_set_value_pos(GTK_SCALE(monkey_compression), GTK_POS_RIGHT);
	gtk_scale_set_digits(GTK_SCALE(monkey_compression), 0);

	rip_monkey = gtk_check_button_new_with_mnemonic(_("Monkey's Audio (lossless compression)"));
	GLADE_HOOKUP_OBJECT(prefs, rip_monkey, "rip_monkey");
	gtk_widget_show(rip_monkey);
	gtk_frame_set_label_widget(GTK_FRAME(monkey_frame), rip_monkey);
	g_signal_connect((gpointer)rip_monkey, "toggled", G_CALLBACK(on_rip_monkey_toggled), NULL);
	/* END MONKEY */

	expander = gtk_expander_new(_("Proprietary encoders"));
	gtk_widget_show(expander);
	gtk_box_pack_start(GTK_BOX(vbox), expander, FALSE, FALSE, 0);
	GLADE_HOOKUP_OBJECT(prefs, expander, "proprietary_formats_expander");

	hiddenbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show(hiddenbox);
	gtk_container_add(GTK_CONTAINER(expander), hiddenbox);

	/* AAC */
	GtkWidget *aac_frame;
	GtkWidget *aac_quality_box;
	GtkWidget *aac_quality_label;
	GtkWidget *aac_quality;
	GtkWidget *rip_aac;

	aac_frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(aac_frame), GTK_SHADOW_IN);
	gtk_widget_show(aac_frame);
	gtk_box_pack_start(GTK_BOX(hiddenbox), aac_frame, FALSE, FALSE, 0);

	aac_quality_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GLADE_HOOKUP_OBJECT(prefs, aac_quality_box, "aac_box");
	gtk_widget_set_margin_start(aac_quality_box, 10);
	gtk_widget_set_margin_end(aac_quality_box, 10);
	gtk_widget_set_tooltip_text(
		aac_quality_box,
		_("Higher quality means bigger file. Default is " STR(PREFS_DEFAULT_AAC_QUALITY) "."));
	gtk_widget_show(aac_quality_box);
	gtk_container_add(GTK_CONTAINER(aac_frame), aac_quality_box);

	aac_quality_label = gtk_label_new(_("Quality"));
	gtk_widget_show(aac_quality_label);
	gtk_box_pack_start(GTK_BOX(aac_quality_box), aac_quality_label, FALSE, FALSE, 0);

	aac_quality = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL,
								gtk_adjustment_new(PREFS_DEFAULT_AAC_QUALITY, 1, 100, 1, 1, 1));
	GLADE_HOOKUP_OBJECT(prefs, aac_quality, "aac_quality");
	gtk_widget_show(aac_quality);
	gtk_box_pack_start(GTK_BOX(aac_quality_box), aac_quality, TRUE, TRUE, 5);
	gtk_scale_set_value_pos(GTK_SCALE(aac_quality), GTK_POS_RIGHT);
	gtk_scale_set_digits(GTK_SCALE(aac_quality), 0);

	rip_aac = gtk_check_button_new_with_mnemonic(_("AAC (lossy compression, Nero encoder)"));
	GLADE_HOOKUP_OBJECT(prefs, rip_aac, "rip_aac");
	gtk_widget_show(rip_aac);
	gtk_frame_set_label_widget(GTK_FRAME(aac_frame), rip_aac);
	g_signal_connect((gpointer)rip_aac, "toggled", G_CALLBACK(on_rip_aac_toggled), NULL);
	/* END AAC */

	label = gtk_label_new(_("Encode"));
	gtk_widget_show(label);
	gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook1),
							   gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook1), 2), label);
	/* END ENCODE tab */

	/* ADVANCED tab */
	GtkWidget *do_cddb_updates;
	GtkWidget *frame;
	GtkWidget *hbox;
	GtkWidget *cddbServerName;
	GtkWidget *cddbPortNum;
	GtkWidget *useProxy;
	GtkWidget *serverName;
	GtkWidget *portNum;
	GtkWidget *frameVbox;
	GtkWidget *do_log;
	GtkWidget *do_fast_rip;

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(notebook1), vbox);

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_frame_set_label(GTK_FRAME(frame), "CDDB");
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);

	frameVbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show(frameVbox);
	gtk_container_add(GTK_CONTAINER(frame), frameVbox);

	do_cddb_updates =
		gtk_check_button_new_with_mnemonic(_("Get disc info from the internet automatically"));
	gtk_widget_show(do_cddb_updates);
	gtk_box_pack_start(GTK_BOX(frameVbox), do_cddb_updates, FALSE, FALSE, 0);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(frameVbox), hbox, FALSE, FALSE, 1);

	label = gtk_label_new(_("Server: "));
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

	cddbServerName = gtk_entry_new();
	gtk_widget_show(cddbServerName);
	gtk_box_pack_start(GTK_BOX(hbox), cddbServerName, TRUE, TRUE, 5);
	GLADE_HOOKUP_OBJECT(prefs, cddbServerName, "cddb_server_name");
	gtk_widget_set_tooltip_text(
		cddbServerName, _("The CDDB server to get disc info from (default is gnudb.gnudb.org)"));

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(frameVbox), hbox, FALSE, FALSE, 1);

	label = gtk_label_new(_("Port: "));
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

	cddbPortNum = gtk_entry_new();
	gtk_widget_show(cddbPortNum);
	gtk_box_pack_start(GTK_BOX(hbox), cddbPortNum, TRUE, TRUE, 5);
	GLADE_HOOKUP_OBJECT(prefs, cddbPortNum, "cddb_port_number");
	gtk_widget_set_tooltip_text(cddbPortNum, _("The CDDB server port (default is 8880)"));

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);

	useProxy =
		gtk_check_button_new_with_mnemonic(_("Use an HTTP proxy to connect to the internet"));
	gtk_widget_show(useProxy);
	gtk_frame_set_label_widget(GTK_FRAME(frame), useProxy);
	GLADE_HOOKUP_OBJECT(prefs, useProxy, "use_proxy");

	frameVbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show(frameVbox);
	gtk_container_add(GTK_CONTAINER(frame), frameVbox);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(frameVbox), hbox, FALSE, FALSE, 1);

	label = gtk_label_new(_("Server: "));
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

	serverName = gtk_entry_new();
	gtk_widget_show(serverName);
	gtk_box_pack_start(GTK_BOX(hbox), serverName, TRUE, TRUE, 5);
	GLADE_HOOKUP_OBJECT(prefs, serverName, "server_name");

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(frameVbox), hbox, FALSE, FALSE, 1);

	label = gtk_label_new(_("Port: "));
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

	portNum = gtk_entry_new();
	gtk_widget_show(portNum);
	gtk_box_pack_start(GTK_BOX(hbox), portNum, TRUE, TRUE, 5);
	GLADE_HOOKUP_OBJECT(prefs, portNum, "port_number");

	do_log = gtk_check_button_new_with_label(_("Enable log"));
	gtk_widget_show(do_log);
	gtk_box_pack_start(GTK_BOX(vbox), do_log, FALSE, FALSE, 0);
	GLADE_HOOKUP_OBJECT(prefs, do_log, "do_log");

	do_fast_rip = gtk_check_button_new_with_label(_("Faster ripping (no error correction)"));
	gtk_widget_show(do_fast_rip);
	gtk_box_pack_start(GTK_BOX(vbox), do_fast_rip, FALSE, FALSE, 0);
	GLADE_HOOKUP_OBJECT(prefs, do_fast_rip, "do_fast_rip");

	hboxFill = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show(hboxFill);
	gtk_box_pack_start(GTK_BOX(vbox), hboxFill, TRUE, TRUE, 0);

	label = gtk_label_new(_("Advanced"));
	gtk_widget_show(label);
	gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook1),
							   gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook1), 3), label);
	/* END ADVANCED tab */

	dialog_action_area1 = gtk_dialog_get_action_area(GTK_DIALOG(prefs));
	gtk_widget_show(dialog_action_area1);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(dialog_action_area1), GTK_BUTTONBOX_END);

	cancelbutton1 = gtk_button_new_from_stock("gtk-cancel");
	gtk_widget_show(cancelbutton1);
	gtk_dialog_add_action_widget(GTK_DIALOG(prefs), cancelbutton1, GTK_RESPONSE_CANCEL);
	gtk_widget_set_can_default(cancelbutton1, TRUE);

	okbutton1 = gtk_button_new_from_stock("gtk-ok");
	gtk_widget_show(okbutton1);
	gtk_dialog_add_action_widget(GTK_DIALOG(prefs), okbutton1, GTK_RESPONSE_OK);
	gtk_widget_set_can_default(okbutton1, TRUE);

	g_signal_connect((gpointer)prefs, "response", G_CALLBACK(on_prefs_response), NULL);
	g_signal_connect((gpointer)prefs, "realize", G_CALLBACK(on_prefs_show), NULL);

	/* Store pointers to all widgets, for use by lookup_widget(). */
	GLADE_HOOKUP_OBJECT_NO_REF(prefs, prefs, "prefs");
	GLADE_HOOKUP_OBJECT(prefs, notebook1, "notebook1");
	GLADE_HOOKUP_OBJECT(prefs, music_dir, "music_dir");
	GLADE_HOOKUP_OBJECT(prefs, make_playlist, "make_playlist");
	GLADE_HOOKUP_OBJECT(prefs, cdrom, "cdrom");
	GLADE_HOOKUP_OBJECT(prefs, eject_on_done, "eject_on_done");
	GLADE_HOOKUP_OBJECT(prefs, format_music, "format_music");
	GLADE_HOOKUP_OBJECT(prefs, format_albumdir, "format_albumdir");
	GLADE_HOOKUP_OBJECT(prefs, format_playlist, "format_playlist");
	GLADE_HOOKUP_OBJECT(prefs, rip_wav, "rip_wav");
	GLADE_HOOKUP_OBJECT(prefs, do_cddb_updates, "do_cddb_updates");
	GLADE_HOOKUP_OBJECT_NO_REF(prefs, dialog_action_area1, "dialog_action_area1");
	GLADE_HOOKUP_OBJECT(prefs, cancelbutton1, "cancelbutton1");
	GLADE_HOOKUP_OBJECT(prefs, okbutton1, "okbutton1");

	return prefs;
}

GtkWidget *create_ripping(void)
{
	GtkWidget *ripping;
	GtkWidget *dialog_vbox2;
	GtkWidget *grid;
	GtkWidget *progress_total;
	GtkWidget *progress_rip;
	GtkWidget *progress_encode;
	GtkWidget *label_total;
	GtkWidget *label_rip;
	GtkWidget *label_encode;
	GtkWidget *dialog_action_area2;
	GtkWidget *cancel;

	ripping = gtk_dialog_new();
	gtk_window_set_transient_for(GTK_WINDOW(ripping), GTK_WINDOW(win_main));
	gtk_window_set_title(GTK_WINDOW(ripping), _("Ripping"));
	gtk_window_set_modal(GTK_WINDOW(ripping), TRUE);
	gtk_window_set_type_hint(GTK_WINDOW(ripping), GDK_WINDOW_TYPE_HINT_DIALOG);

	dialog_vbox2 = gtk_dialog_get_content_area(GTK_DIALOG(ripping));
	gtk_widget_show(dialog_vbox2);

	grid = gtk_grid_new();
	gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
	gtk_widget_set_margin_start(grid, 10);
	gtk_widget_set_margin_end(grid, 10);
	gtk_widget_set_margin_top(grid, 5);
	gtk_widget_set_margin_bottom(grid, 10);
	gtk_widget_show(grid);
	gtk_box_pack_start(GTK_BOX(dialog_vbox2), grid, TRUE, TRUE, 0);

	progress_total = gtk_progress_bar_new();
	gtk_widget_show(progress_total);
	gtk_grid_attach(GTK_GRID(grid), progress_total, 1, 0, 1, 1);

	progress_rip = gtk_progress_bar_new();
	gtk_widget_show(progress_rip);
	gtk_grid_attach(GTK_GRID(grid), progress_rip, 1, 1, 1, 1);

	progress_encode = gtk_progress_bar_new();
	gtk_widget_show(progress_encode);
	gtk_grid_attach(GTK_GRID(grid), progress_encode, 1, 2, 1, 1);

	label_total = gtk_label_new(_("Total progress"));
	gtk_widget_set_halign(label_total, GTK_ALIGN_START);
	gtk_widget_show(label_total);
	gtk_grid_attach(GTK_GRID(grid), label_total, 0, 0, 1, 1);

	label_rip = gtk_label_new(_("Ripping"));
	gtk_widget_set_halign(label_rip, GTK_ALIGN_START);
	gtk_widget_show(label_rip);
	gtk_grid_attach(GTK_GRID(grid), label_rip, 0, 1, 1, 1);

	label_encode = gtk_label_new(_("Encoding"));
	gtk_widget_set_halign(label_encode, GTK_ALIGN_START);
	gtk_widget_show(label_encode);
	gtk_grid_attach(GTK_GRID(grid), label_encode, 0, 2, 1, 1);

	dialog_action_area2 = gtk_dialog_get_action_area(GTK_DIALOG(ripping));
	gtk_widget_show(dialog_action_area2);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(dialog_action_area2), GTK_BUTTONBOX_END);

	cancel = gtk_button_new_from_stock("gtk-cancel");
	gtk_widget_show(cancel);
	gtk_dialog_add_action_widget(GTK_DIALOG(ripping), cancel, GTK_RESPONSE_CANCEL);
	gtk_widget_set_can_default(cancel, TRUE);

	g_signal_connect((gpointer)cancel, "clicked", G_CALLBACK(on_cancel_clicked), NULL);

	/* Store pointers to all widgets, for use by lookup_widget(). */
	GLADE_HOOKUP_OBJECT_NO_REF(ripping, ripping, "ripping");
	GLADE_HOOKUP_OBJECT(ripping, progress_total, "progress_total");
	GLADE_HOOKUP_OBJECT(ripping, progress_rip, "progress_rip");
	GLADE_HOOKUP_OBJECT(ripping, progress_encode, "progress_encode");
	GLADE_HOOKUP_OBJECT(ripping, cancel, "cancel");

	return ripping;
}

void disable_all_main_widgets(void)
{
	gtk_widget_set_sensitive(lookup_widget(win_main, "lookup"), FALSE);
	gtk_widget_set_sensitive(lookup_widget(win_main, "preferences"), FALSE);
	gtk_widget_set_sensitive(lookup_widget(win_main, "disc"), FALSE);
	gtk_widget_set_sensitive(lookup_widget(win_main, "album_artist"), FALSE);
	gtk_widget_set_sensitive(lookup_widget(win_main, "artist_label"), FALSE);
	gtk_widget_set_sensitive(lookup_widget(win_main, "title_label"), FALSE);
	gtk_widget_set_sensitive(lookup_widget(win_main, "album_title"), FALSE);
	gtk_widget_set_sensitive(lookup_widget(win_main, "single_artist"), FALSE);
	gtk_widget_set_sensitive(tracklist, FALSE);
	gtk_widget_set_sensitive(lookup_widget(win_main, "rip_button"), FALSE);
	gtk_widget_set_sensitive(lookup_widget(win_main, "album_genre"),
							 FALSE); // lnr
	gtk_widget_set_sensitive(lookup_widget(win_main, "genre_label"),
							 FALSE); // lnr
	gtk_widget_set_sensitive(lookup_widget(win_main, "single_genre"),
							 FALSE); // lnr
	gtk_widget_set_sensitive(lookup_widget(win_main, "album_year"), FALSE);
}

void enable_all_main_widgets(void)
{
	gtk_widget_set_sensitive(lookup_widget(win_main, "lookup"), TRUE);
	gtk_widget_set_sensitive(lookup_widget(win_main, "preferences"), TRUE);
	gtk_widget_set_sensitive(lookup_widget(win_main, "disc"), TRUE);
	gtk_widget_set_sensitive(lookup_widget(win_main, "album_artist"), TRUE);
	gtk_widget_set_sensitive(lookup_widget(win_main, "artist_label"), TRUE);
	gtk_widget_set_sensitive(lookup_widget(win_main, "title_label"), TRUE);
	gtk_widget_set_sensitive(lookup_widget(win_main, "album_title"), TRUE);
	gtk_widget_set_sensitive(lookup_widget(win_main, "single_artist"), TRUE);
	gtk_widget_set_sensitive(tracklist, TRUE);
	gtk_widget_set_sensitive(lookup_widget(win_main, "rip_button"), TRUE);
	gtk_widget_set_sensitive(lookup_widget(win_main, "album_genre"),
							 TRUE); // lnr
	gtk_widget_set_sensitive(lookup_widget(win_main, "genre_label"),
							 TRUE); // lnr
	gtk_widget_set_sensitive(lookup_widget(win_main, "single_genre"),
							 TRUE); // lnr
	gtk_widget_set_sensitive(lookup_widget(win_main, "album_year"), TRUE);
}

void show_completed_dialog(int numOk, int numFailed)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new(
		GTK_WINDOW(win_main), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
		ngettext("%d file created successfully", "%d files created successfully", numOk), numOk);

	if (numFailed > 0) {
		dialog = gtk_message_dialog_new(GTK_WINDOW(win_main), GTK_DIALOG_DESTROY_WITH_PARENT,
										GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE,
										ngettext("There was an error creating %d file",
												 "There was an error creating %d files", numFailed),
										numFailed);
	}

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}
