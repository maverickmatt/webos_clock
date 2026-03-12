#include <pebble.h>

#define SETTINGS_DATEFMT_KEY 1
#define SETTINGS_LARGEFONT_KEY 2

static void prv_save_settings();
static void prv_load_settings();