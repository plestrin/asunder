#include <stdbool.h>

GtkWidget* create_main (void);
GtkWidget* create_prefs (void);
GtkWidget* create_ripping (void);
void disable_all_main_widgets(void);
void enable_all_main_widgets(void);
void show_completed_dialog(int numOk, int numFailed);
