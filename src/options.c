#include "options.h"
#include "playlist.h"

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct _irmpc_options irmpc_options = {
    .config_file       = NULL,
    .mpd_hostname      = "localhost",
    .mpd_password      = NULL,
    .mpd_port          = 6600,
    .mpd_maxtries      = 2,
    .mpd_update_amount = 2,
    .volume_step       = 2,
    .lirc_config       = NULL,
    .lircd_tries       = 5,
    .lirc_key_timespan = 2,
    .power_command     = NULL,
    .power_amount      = 2,
    .progname          = "irmpc",
    .verbose           = false,
    .debug             = false
};

struct option_file_data {
    const gchar *group_name;
    const gchar *key_name;
    GOptionArg   arg;
    gpointer     arg_data;
};

static GOptionEntry option_entries [] = {
    {"config",       'c', 0, G_OPTION_ARG_FILENAME, &(irmpc_options.config_file),       "Configuration file",                                            "filename"},
    {"hostname",     'H', 0, G_OPTION_ARG_STRING,   &(irmpc_options.mpd_hostname),      "Hostname of host running mpd - default: localhost",             "host"},
    {"password",     'p', 0, G_OPTION_ARG_STRING,   &(irmpc_options.mpd_password),      "Password of mpd",                                               "password"},
    {"port",         'P', 0, G_OPTION_ARG_INT,      &(irmpc_options.mpd_port),          "Port of mpd - default: port=6600",                              "port"},
    {"maxtries",     'm', 0, G_OPTION_ARG_INT,      &(irmpc_options.mpd_maxtries),      "Maximum tries for sending mpd commands",                        "n"},
    {"updaterepeat", 'u', 0, G_OPTION_ARG_INT,      &(irmpc_options.mpd_update_amount), "Amount of times playlist update button needs to be pressed",    "n"},
    {"volumestep",   's', 0, G_OPTION_ARG_INT,      &(irmpc_options.volume_step),       "Step in percent for volume up/down",                            "step"},
    {"lircconfig",   'l', 0, G_OPTION_ARG_FILENAME, &(irmpc_options.lirc_config),       "Configuration file for lirc commands",                          "filename"},
    {"keytimespan",  't', 0, G_OPTION_ARG_INT,      &(irmpc_options.lirc_key_timespan), "Maximum time in seconds between keys of multiple key commands", "span"},
    {"powercmd",     'C', 0, G_OPTION_ARG_STRING,   &(irmpc_options.power_command),     "System command to execute when poweroff button is pressed",     "command"},
    {"powerrepeat",  'r', 0, G_OPTION_ARG_INT,      &(irmpc_options.power_amount),      "Amount of times power button needs to be pressed",              "amount"},
    {"verbose",      'v', 0, 0,                     &(irmpc_options.verbose),           "Set to verbose",                                                NULL},
    {"debug",        'd', 0, 0,                     &(irmpc_options.debug),             "Activate debug output",                                         NULL},
    {NULL}
};

static struct option_file_data cfg_file_entries [] = {
    {"mpd",    "hostname",     G_OPTION_ARG_STRING,   &(irmpc_options.mpd_hostname)},
    {"mpd",    "password",     G_OPTION_ARG_STRING,   &(irmpc_options.mpd_password)},
    {"mpd",    "port",         G_OPTION_ARG_INT,      &(irmpc_options.mpd_port)},
    {"mpd",    "maxtries",     G_OPTION_ARG_INT,      &(irmpc_options.mpd_maxtries)},
    {"mpd",    "updaterepeat", G_OPTION_ARG_INT,      &(irmpc_options.mpd_update_amount)},
    {"mpd",    "volumestep",   G_OPTION_ARG_INT,      &(irmpc_options.volume_step)},
    {"lirc",   "lircconfig",   G_OPTION_ARG_FILENAME, &(irmpc_options.lirc_config)},
    {"lirc",   "keytimespan",  G_OPTION_ARG_INT,      &(irmpc_options.lirc_key_timespan)},
    {"system", "powercmd",     G_OPTION_ARG_STRING,   &(irmpc_options.power_command)},
    {"system", "powerrepeat",  G_OPTION_ARG_INT,      &(irmpc_options.power_amount)},
    {NULL}
};

static bool irmpc_options_from_file ()
{
    GError   *error    = NULL;
    GKeyFile *key_file = g_key_file_new ();

    if (!g_key_file_load_from_file (key_file, irmpc_options.config_file, G_KEY_FILE_NONE, &error)) {
        fprintf (stderr, "Error parsing config file: %s\n", error->message);
        g_error_free (error);
        return false;
    }
    
    /* try finding options */
    for (struct option_file_data *entry = &(cfg_file_entries[0]); entry->group_name != NULL; entry++) {
        g_clear_error (&error);
        if (entry->arg == G_OPTION_ARG_INT) {
            gint   tempint;
            tempint = g_key_file_get_integer (key_file, entry->group_name, entry->key_name, &error);
            if (error == NULL) {
                int *targetint = (int *) entry->arg_data;
                *targetint = tempint;
            } else {
                if ((error->code != G_KEY_FILE_ERROR_GROUP_NOT_FOUND) && (error->code != G_KEY_FILE_ERROR_KEY_NOT_FOUND)) {
                    g_key_file_free (key_file);
                    fprintf (stderr, "Error parsing config file %s: %s\n", irmpc_options.config_file, error->message);
                    g_error_free (error);
                    return false;
                }
            }
        } else if ((entry->arg == G_OPTION_ARG_STRING) || (entry->arg == G_OPTION_ARG_FILENAME)) {
            gchar *tempstr;
            tempstr = g_key_file_get_string (key_file, entry->group_name, entry->key_name, &error);
            if (irmpc_options.debug) {
                printf ("INFO: parsed string %s for option %s\n", tempstr, entry->key_name);
            }
            if (tempstr != NULL) {
                gchar **targetstr = (gchar **) entry->arg_data;
                *targetstr = tempstr;
            }
        }
    }

    /* playlists */
    gchar **tempstrlist;
    gsize listlen;
    tempstrlist = g_key_file_get_keys (key_file, "playlists", &listlen, &error);
    if (tempstrlist != NULL) {
        for (int i = 0; i < listlen; i++) {
            g_clear_error (&error);
            char *number_str = tempstrlist[i];
            char *endptr;
            long int number = strtol (number_str, &endptr, 10);
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
    if (irmpc_options.mpd_port >= (1 << 16)) {
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
    if (irmpc_options.debug) {
        printf ("mpd-maxtries: %d\n", irmpc_options.mpd_maxtries);
    }
    if (irmpc_options.volume_step > 100) {
        fprintf (stderr, "ERROR: volume step needs to be in range 0 ... 100\n");
        return false;
    } else {
        if (irmpc_options.debug) {
            printf ("volume-step: %d\n", irmpc_options.volume_step);
        }
    }
    if (irmpc_options.lirc_config != NULL) {
        if (irmpc_options.debug) {
            printf ("lirc configuration: %s\n", irmpc_options.lirc_config);
        }
    }
    if (irmpc_options.debug) {
        printf ("lirc keytimespan: %d\n", irmpc_options.lirc_key_timespan);
    }
    if (irmpc_options.power_command != NULL) {
        if (irmpc_options.debug) {
            printf ("poweroff system command: %s\n", irmpc_options.power_command);
        }
    }
    if (irmpc_options.debug) {
        printf ("powerkey repetitions: %d\n", irmpc_options.power_amount);
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
        if (!irmpc_options_from_file ()) {
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

