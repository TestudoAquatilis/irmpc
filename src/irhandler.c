#include "irhandler.h"
#include "options.h"
#include "mpd.h"

#ifndef DEBUG_NO_LIRC
#include <lirc/lirc_client.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


static int    power_last_press = -1;
static time_t power_last_time;

static void system_handler (const char *command)
{
    if (strcmp (command, "poweroff") == 0) {
        time_t this_time = time (NULL);

        int power_press = 1;

        if (power_last_press >= 0) {
            double timediff = difftime (this_time, power_last_time);
            if (irmpc_options.debug) {
                printf ("INFO: timediff to last press: %fs\n", timediff);
            }
            if (timediff <= irmpc_options.lirc_key_timespan) {
                power_press += power_last_press;
            }
        }

        if (power_press >= irmpc_options.power_amount) {
            if (irmpc_options.power_command != NULL) {
                if (irmpc_options.debug) {
                    printf ("INFO: executing poweroff command: %s\n", irmpc_options.power_command);
                }

                /* TODO: exec command */
            } else {
                fprintf (stderr, "WARNING: no poweroff command specified\n");
            }

            power_press = 0;
        }

        power_last_press = power_press;
        power_last_time  = time (NULL);
    }
}

bool irmpc_irhandler ()
{
#ifndef DEBUG_NO_LIRC
    struct lirc_config *config;

    if (lirc_init (irmpc_options.progname, 1) == -1) {
        fprintf (stderr, "ERROR: failed to initialize lirc\n");
        return false;
    }

    if (lirc_readconfig (irmpc_options.lirc_config, &config, NULL) != 0) {
        fprintf (stderr, "ERROR: failed to load lirc config file\n");
        goto irmpc_irhandler_error_exit;
    }

    /* main loop */
    char *code;

    while (lirc_nextcode (&code) == 0) {
        if (code == NULL) continue;

        int ret;
        char *c = NULL;

        while (((ret = lirc_code2char (config, code, &c)) == 0) && (c != NULL)) {
#else
        char c [1024];
        while (true) {
            scanf("%s", c);
#endif
            if (irmpc_options.debug) {
                printf ("Got command: \"%s\"\n", c);
            }

            if (strlen (c) < 3) {
                fprintf (stderr, "WARNING: ignoring command \"%s\" - too short.\n", c);
                continue;
            }

            if ((c[0] == 'm') && (c[1] == ':')) {
                /* mpd command */
                irmpc_mpd_command (&(c[2]));
            } else if ((c[0] == 'v') && (c[1] == ':')) {
                /* volume command */
                irmpc_mpd_volume (&(c[2]));
            } else if ((c[0] == 's') && (c[1] == ':')) {
                /* system command */
                system_handler (&(c[2]));
            } else if ((c[0] == 'p') && (c[1] == ':')) {
                /* playlist command */
                int number = c[2] - '0';
                if ((number >= 0) && (number <= 9)) {
                    irmpc_mpd_playlist_num (number);
                }
            }

#ifdef DEBUG_NO_LIRC
        }
        return true;
#else
        }

        free (code);
        if (ret == -1) break;
    }
    lirc_freeconfig (config);

    lirc_deinit ();
    return true;

irmpc_irhandler_error_exit:
    lirc_deinit ();
    return false;
#endif
}
