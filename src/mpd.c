#include "mpd.h"
#include "options.h"

#include <stdio.h>
#include <string.h>
#include <mpd/client.h>


static struct mpd_connection *connection = NULL;

static bool connection_init ()
{
    if (connection != NULL) return true;

    if (irmpc_options.debug) {
        printf ("INFO: trying to connect to %s:%d\n", irmpc_options.mpd_hostname, irmpc_options.mpd_port);
    }
    
    /* connect */
    connection = mpd_connection_new (irmpc_options.mpd_hostname, irmpc_options.mpd_port, 5000);
    if (connection == NULL) return false;

    if (mpd_connection_get_error (connection) != MPD_ERROR_SUCCESS) {
        fprintf (stderr, "ERROR: mpd connection failed: %s\n", mpd_connection_get_error_message (connection));
        mpd_connection_free (connection);
        connection = NULL;
        return false;
    }

    /* password */
    if (irmpc_options.mpd_password != NULL) {
        if (irmpc_options.debug) {
            printf ("INFO: sendung password: %s\n", irmpc_options.mpd_password);
        }
        if (! mpd_run_password (connection, irmpc_options.mpd_password)) {
            fprintf (stderr, "ERROR: password failed\n");
            return false;
        }
    }

    if (irmpc_options.debug) {
        printf ("INFO: connection to mpd established successfully\n");
    }

    return true;
}

void irmpc_mpd_command (const char *command)
{
    if (! connection_init ()) return;

    if (strcmp (command, "toggle") == 0) {
        /* toggle play/pause */
        struct mpd_status *status = mpd_run_status (connection);

        if (status == NULL) {
            if (mpd_connection_get_error (connection) != MPD_ERROR_SUCCESS) {
                fprintf (stderr, "ERROR obtaining mpd status: %s\n", mpd_connection_get_error_message (connection));
            } else {
                fprintf (stderr, "ERROR obtaining mpd status:\n");
            }
            return;
        }

        if (mpd_status_get_state(status) == MPD_STATE_PLAY) {
            /* pause */
            mpd_run_pause (connection, true);
        } else {
            /* play */
            mpd_run_play (connection);
        }

        mpd_status_free (status);
    }

}

void irmpc_mpd_free () {
    if (connection != NULL) {
        mpd_connection_free (connection);
        connection = NULL;
    }
}

