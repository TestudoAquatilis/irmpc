#include "mpd.h"
#include "options.h"
#include "playlist.h"

#include <stdio.h>
#include <string.h>
#include <mpd/client.h>
#include <time.h>


static struct mpd_connection *connection = NULL;

static bool connection_check ()
{
    /* check state */
    if (connection != NULL) {
        if (mpd_connection_get_error (connection) == MPD_ERROR_CLOSED) {
            irmpc_mpd_free ();
        } else {
            return true;
        }
    }

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
    bool success = false;
    int  tries   = 0;
    while ((!success) && (tries < irmpc_options.mpd_maxtries)) {
        tries++;

        if (! connection_check ()) continue;

        if (strcmp (command, "playpause") == 0) {
            /* toggle play/pause */
            struct mpd_status *status = mpd_run_status (connection);

            if (status == NULL) {
                if (mpd_connection_get_error (connection) != MPD_ERROR_SUCCESS) {
                    fprintf (stderr, "ERROR obtaining mpd status: %s\n", mpd_connection_get_error_message (connection));
                } else {
                    fprintf (stderr, "ERROR obtaining mpd status:\n");
                }
                continue;
            }

            if (mpd_status_get_state(status) == MPD_STATE_PLAY) {
                /* pause */
                success = mpd_run_pause (connection, true);
            } else {
                /* play */
                success = mpd_run_play (connection);
            }

            mpd_status_free (status);
        } else if (strcmp (command, "next") == 0) {
            success = mpd_run_next (connection);
        } else if (strcmp (command, "prev") == 0) {
            success = mpd_run_previous (connection);
        } else if (strcmp (command, "stop") == 0) {
            success = mpd_run_stop (connection);
        } else if (strcmp (command, "delete") == 0) {
            /* TODO */
        } else {
            break;
        }
    }
}

void irmpc_mpd_playlist (const struct playlist_info *playlist)
{
    bool success = false;
    int  tries   = 0;
    while ((!success) && (tries < irmpc_options.mpd_maxtries)) {
        tries++;

        if (! connection_check ()) continue;

        success = mpd_run_stop (connection);
        if (!success) continue;

        success = mpd_run_clear (connection);
        if (!success) continue;

        success = mpd_run_load (connection, playlist->name);
        if (!success) continue;

        success = mpd_run_random (connection, playlist->random);
        if (!success) continue;

        success = mpd_run_play (connection);
        if (!success) continue;
    }
}

static int    playlist_num_last = -1;
static time_t playlist_num_last_time;

void irmpc_mpd_playlist_num (int number)
{
    time_t this_time = time (NULL);

    if (playlist_num_last > 0) {
        double timediff = difftime (this_time, playlist_num_last_time);
        if (irmpc_options.debug) {
            printf ("INFO: timediff to last key: %fs\n", timediff);
        }
        if (timediff <= irmpc_options.lirc_key_timespan) {
            number = playlist_num_last * 10 + number;
        }
    }

    const struct playlist_info * playlist = irmpc_playlist_get (number);
    if (irmpc_options.debug) {
        printf ("number chosen: %d\n", number);
    }

    if (playlist != NULL) {
        if (irmpc_options.debug) {
            printf ("playlist: %s - random: %d\n", playlist->name, playlist->random);
        }

        irmpc_mpd_playlist (playlist);
    }

    playlist_num_last = number;
    playlist_num_last_time = time (NULL);
}

static bool last_mute   = false;
static int  last_volume = 100;

void irmpc_mpd_volume (const char *command)
{
    bool success = false;
    int  tries   = 0;
    while ((!success) && (tries < irmpc_options.mpd_maxtries)) {
        tries++;

        if (! connection_check ()) continue;

        struct mpd_status *status = mpd_run_status (connection);

        if (status == NULL) {
            if (mpd_connection_get_error (connection) != MPD_ERROR_SUCCESS) {
                fprintf (stderr, "ERROR obtaining mpd status: %s\n", mpd_connection_get_error_message (connection));
            } else {
                fprintf (stderr, "ERROR obtaining mpd status:\n");
            }
            continue;
        }

        int  current_volume = mpd_status_get_volume (status);
        bool current_mute;

        mpd_status_free (status);

        if (strcmp (command, "up") == 0) {
            current_mute = false;
            if (last_mute) {
                current_volume = last_volume;
            } else {
                current_volume += irmpc_options.volume_step;
            }
        } else if (strcmp (command, "down") == 0) {
            if (last_mute) {
                current_mute   = true;
                current_volume = last_volume;
            } else {
                current_mute    = false;
                current_volume -= irmpc_options.volume_step;
            }
        } else if (strcmp (command, "mute") == 0) {
            if (last_mute) {
                current_mute   = false;
                current_volume = last_volume;
            } else {
                current_mute = true;
            }
        } else {
            break;
        }

        if (current_volume < 0)   current_volume = 0;
        if (current_volume > 100) current_volume = 100;

        if (irmpc_options.debug) {
            printf ("INFO: setting volume from %d (mute: %d) to %d (mute: %d)\n", last_volume, last_mute, current_volume, current_mute);
        }

        if (current_mute) {
            success = mpd_run_set_volume (connection, 0);
            if (!success) continue;
        } else {
            success = mpd_run_set_volume (connection, current_volume);
            if (!success) continue;
        }

        last_mute   = current_mute;
        last_volume = current_volume;
    }
}


void irmpc_mpd_free () {
    if (connection != NULL) {
        mpd_connection_free (connection);
        connection = NULL;
    }
}

