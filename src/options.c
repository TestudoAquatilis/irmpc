#include "options.h"
#include "playlist.h"

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct _irmpc_options irmpc_options = {
    .config_file  = NULL,
    .mpd_hostname = NULL,
    .mpd_password = NULL,
    .mpd_port     = 6600,
    .lirc_config  = NULL,
    .verbose      = false,
    .debug        = false
};

static GOptionEntry option_entries[] = {
    {"config",     'c', 0, G_OPTION_ARG_FILENAME, &(irmpc_options.config_file),  "Configuration file",                   "filename"},
    {"hostname",   'H', 0, G_OPTION_ARG_STRING,   &(irmpc_options.mpd_hostname), "Hostname of host running mpd",         "host"},
    {"password",   'p', 0, G_OPTION_ARG_STRING,   &(irmpc_options.mpd_password), "Password of mpd",                      "password"},
    {"port",       'P', 0, G_OPTION_ARG_INT,      &(irmpc_options.mpd_password), "Port of mpd - default: port=6600",     "port"},
    {"lircconfig", 'l', 0, G_OPTION_ARG_FILENAME, &(irmpc_options.lirc_config),  "Configuration file for lirc commands", "filename"},
    {"verbose",    'v', 0, 0,                     &(irmpc_options.verbose),      "Set to verbose",                       NULL},
    {"debug",      'd', 0, 0,                     &(irmpc_options.debug),        "Activate debug output",                NULL},
    {NULL}
};

static bool options_from_file ()
{
    GError   *error    = NULL;
    GKeyFile *key_file = g_key_file_new ();

    if (!g_key_file_load_from_file (key_file, irmpc_options.config_file, G_KEY_FILE_NONE, &error)) {
        fprintf (stderr, "Error parsing config file: %s\n", error->message);
        g_error_free (error);
        return false;
    }
    
    /* try finding options */
    gchar *tempstr;
    gint   tempint;

    tempstr = g_key_file_get_string (key_file, "mpd", "hostname", &error);
    if (tempstr != NULL) {irmpc_options.mpd_hostname = tempstr;}

    tempstr = g_key_file_get_string (key_file, "mpd", "password", &error);
    if (tempstr != NULL) {irmpc_options.mpd_password = tempstr;}

    tempint = g_key_file_get_integer (key_file, "mpd", "port", &error);
    if (error == NULL) {
        irmpc_options.mpd_port = tempint;
    } else {
        if (error->code != G_KEY_FILE_ERROR_KEY_NOT_FOUND) {
            g_key_file_free (key_file);
            fprintf (stderr, "Error parsing config file %s: %s\n", irmpc_options.config_file, error->message);
            g_error_free (error);
            return false;
        }
    }

    tempstr = g_key_file_get_string (key_file, "lirc", "config-file", &error);
    if (tempstr != NULL) {irmpc_options.lirc_config = tempstr;}


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

                irmpc_playlist_add (number, entryname, entryrand);
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
static bool options_check ()
{
    if (irmpc_options.config_file != NULL) {
        if (irmpc_options.debug) {
            printf ("config-file: %s\n", irmpc_options.config_file);
        }
    }
    if (irmpc_options.mpd_hostname == NULL) {
        fprintf (stderr, "ERROR: no hostname specified\n");
        return false;
    } else {
        if (irmpc_options.debug) {
            printf ("hostname: %s\n", irmpc_options.mpd_hostname);
        }
    }
    if ((irmpc_options.mpd_port < 0) || (irmpc_options.mpd_port >= (1 << 16))) {
        fprintf (stderr, "ERROR: port needs to be in range 0 ... %d\n", (1 << 16));
        return false;
    } else {
        if (irmpc_options.debug) {
            printf ("port: %d\n", irmpc_options.mpd_port);
        }
    }
    if (irmpc_options.mpd_password != NULL) {
        if (irmpc_options.debug) {
            printf ("password: %s\n", irmpc_options.mpd_password);
        }
    }
    if (irmpc_options.lirc_config == NULL) {
        fprintf (stderr, "ERROR: no lirc configuration file specified\n");
        return false;
    } else {
        if (irmpc_options.debug) {
            printf ("lirc configuration: %s\n", irmpc_options.lirc_config);
        }
    }

    if (irmpc_options.debug) {
        irmpc_playlist_print_debug ();
    }

    return true;
}


bool irmpc_parse_options (int *argc, char ***argv)
{
    /* option parsing */
    GError         *error          = NULL;
    GOptionContext *option_context = g_option_context_new ("send mpc commands via remote control");

    g_option_context_add_main_entries (option_context, option_entries, NULL);

    if (!g_option_context_parse (option_context, argc, argv, &error)) {
        fprintf (stderr, "Error parsing options: %s\n", error->message);
        goto irmpc_options_exit_error;
    }

    if (irmpc_options.config_file != NULL) {
        if (!options_from_file ()) {
            goto irmpc_options_exit_error;
        }
    }

    if (!options_check ()) {
        goto irmpc_options_exit_error;
    }

    g_option_context_free (option_context);

    if (error != NULL) {
        g_error_free (error);
    }

    return true;

irmpc_options_exit_error:
    g_option_context_free (option_context);
    if (error != NULL) {
        g_error_free (error);
    }
    return false;
}

