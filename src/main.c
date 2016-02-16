#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>


#include "playlist.h"

/* ================================================================================================================================= */
/* || configuration */
/* ================================================================================================================================= */
struct irmpc_options {
    char *config_file;

    char *mpd_hostname;
    char *mpd_password;
    int  mpd_port;

    char *lirc_config;

    bool verbose;
    bool debug;
};

static struct irmpc_options options = {
    .config_file  = NULL,
    .mpd_hostname = NULL,
    .mpd_password = NULL,
    .mpd_port     = 6600,
    .lirc_config  = NULL,
    .verbose      = false,
    .debug        = false
};

static GOptionEntry option_entries[] = {
    {"config",     'c', 0, G_OPTION_ARG_FILENAME, &(options.config_file),  "Configuration file",                   "filename"},
    {"hostname",   'H', 0, G_OPTION_ARG_STRING,   &(options.mpd_hostname), "Hostname of host running mpd",         "host"},
    {"password",   'p', 0, G_OPTION_ARG_STRING,   &(options.mpd_password), "Password of mpd",                      "password"},
    {"port",       'P', 0, G_OPTION_ARG_INT,      &(options.mpd_password), "Port of mpd - default: port=6600",     "port"},
    {"lircconfig", 'l', 0, G_OPTION_ARG_FILENAME, &(options.lirc_config),  "Configuration file for lirc commands", "filename"},
    {"verbose",    'v', 0, 0,                     &(options.verbose),      "Set to verbose",                       NULL},
    {"debug",      'd', 0, 0,                     &(options.debug),        "Activate debug output",                NULL},
    {NULL}
};

bool options_from_file ()
{
    GError   *error    = NULL;
    GKeyFile *key_file = g_key_file_new ();

    if (!g_key_file_load_from_file (key_file, options.config_file, G_KEY_FILE_NONE, &error)) {
        fprintf (stderr, "Error parsing config file: %s\n", error->message);
        g_error_free (error);
        return false;
    }
    
    /* try finding options */
    gchar *tempstr;
    gint   tempint;

    tempstr = g_key_file_get_string (key_file, "mpd", "hostname", &error);
    if (tempstr != NULL) {options.mpd_hostname = tempstr;}

    tempstr = g_key_file_get_string (key_file, "mpd", "password", &error);
    if (tempstr != NULL) {options.mpd_password = tempstr;}

    tempint = g_key_file_get_integer (key_file, "mpd", "port", &error);
    if (error == NULL) {
        options.mpd_port = tempint;
    } else {
        if (error->code != G_KEY_FILE_ERROR_KEY_NOT_FOUND) {
            g_key_file_free (key_file);
            fprintf (stderr, "Error parsing config file %s: %s\n", options.config_file, error->message);
            g_error_free (error);
            return false;
        }
    }

    tempstr = g_key_file_get_string (key_file, "lirc", "config-file", &error);
    if (tempstr != NULL) {options.lirc_config = tempstr;}


    /* playlists */
    gchar **tempstrlist;
    gsize listlen;
    tempstrlist = g_key_file_get_keys (key_file, "playlists", &listlen, &error);
    if (tempstrlist != NULL) {
        for (int i = 0; i < listlen; i++) {
            char *number_str = tempstrlist[i];
            char *endptr;
            long int number = strtol(number_str, &endptr, 10);
            if ((*number_str == '\0') || (*endptr != '\0') || (number < 0)) continue;

            gsize entrylen;
            gchar **entrylist = g_key_file_get_string_list (key_file, "playlists", number_str, &entrylen, &error);

            if (entrylist == NULL) continue;

            if (entrylen > 0) {
                char *entryname = entrylist[0];
                bool entryrand  = false;
                if (entrylen > 1) {
                    if (strcmp (entrylist[1], "r") == 0) {
                        entryrand = true;
                    }
                }

                playlist_add (number, entryname, entryrand);
            }

            g_strfreev (entrylist);
        }

        g_strfreev (tempstrlist);
    }

    /* free */
    g_key_file_free (key_file);
    if (error != NULL) {
        g_error_free (error);
    }

    return true;
}

/* option checking */
bool options_check ()
{
    if (options.config_file != NULL) {
        if (options.debug) {
            printf ("config-file: %s\n", options.config_file);
        }
    }
    if (options.mpd_hostname == NULL) {
        fprintf (stderr, "ERROR: no hostname specified\n");
        return false;
    } else {
        if (options.debug) {
            printf ("hostname: %s\n", options.mpd_hostname);
        }
    }
    if ((options.mpd_port < 0) || (options.mpd_port >= (1 << 16))) {
        fprintf (stderr, "ERROR: port needs to be in range 0 ... %d\n", (1 << 16));
        return false;
    } else {
        if (options.debug) {
            printf ("port: %d\n", options.mpd_port);
        }
    }
    if (options.mpd_password != NULL) {
        if (options.debug) {
            printf ("password: %s\n", options.mpd_password);
        }
    }
    if (options.lirc_config == NULL) {
        fprintf (stderr, "ERROR: no lirc configuration file specified\n");
        return false;
    } else {
        if (options.debug) {
            printf ("lirc configuration: %s\n", options.lirc_config);
        }
    }

    if (options.debug) {
        playlist_print_debug ();
    }

    return true;
}

/* ================================================================================================================================= */
/* || main */
/* ================================================================================================================================= */
int main (int argc, char **argv)
{
    /* option parsing */
    GError         *error          = NULL;
    GOptionContext *option_context = g_option_context_new ("send mpc commands via remote control");

    g_option_context_add_main_entries (option_context, option_entries, NULL);

    if (!g_option_context_parse (option_context, &argc, &argv, &error)) {
        fprintf (stderr, "Error parsing options: %s\n", error->message);
        goto exit_error;
    }

    if (options.config_file != NULL) {
        if (!options_from_file ()) {
            goto exit_error;
        }
    }

    if (!options_check ()) {
        goto exit_error;
    }

    playlist_free ();

    return 0;

exit_error:
    g_option_context_free (option_context);
    if (error != NULL) {
        g_error_free (error);
    }
    playlist_free ();
    return 1;
}
