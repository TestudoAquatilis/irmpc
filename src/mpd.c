#include "mpd.h"
#include "options.h"
#include "playlist.h"

#include <stdio.h>
#include <string.h>
#include <mpd/client.h>
#include <time.h>


/* update current playlist */
static void irmpc_mpd_playlist_update ();
/* load next/prev playlist */
static void irmpc_mpd_playlist_nextprev (int direction);


/* mpd connection */
static struct mpd_connection *connection = NULL;

/* check whether connection is working - try (re)connecting if not */
static bool irmpc_connection_check ()
{
    /* check state and clear existing errors */
    if (connection != NULL) {
        if (mpd_connection_get_error (connection) == MPD_ERROR_CLOSED) {
            irmpc_mpd_free ();
        } else if (mpd_connection_get_error (connection) != MPD_ERROR_SUCCESS) {
            mpd_connection_clear_error (connection);
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

/* handle simple mpd commands */
void irmpc_mpd_command (const char *command)
{
    bool success = false;
    int  tries   = 0;
    bool need_status = false;

    /* commands needing status */
    if (strcmp (command, "playpause") == 0) {
        need_status = true;
    } else if (strcmp (command, "delete") == 0) {
        need_status = true;
    } else if (strcmp (command, "nextalbum") == 0) {
        need_status = true;
    } else if (strcmp (command, "prevalbum") == 0) {
        need_status = true;
    }

    while ((!success) && (tries < irmpc_options.mpd_maxtries)) {
        tries++;

        if (! irmpc_connection_check ()) continue;

        struct mpd_status *status = NULL;
        /* get status if needed */

        if (need_status) {
            status = mpd_run_status (connection);

            if (status == NULL) {
                if (mpd_connection_get_error (connection) != MPD_ERROR_SUCCESS) {
                    fprintf (stderr, "ERROR obtaining mpd status: %s\n", mpd_connection_get_error_message (connection));
                } else {
                    fprintf (stderr, "ERROR obtaining mpd status:\n");
                }
                continue;
            }
        }

        if (strcmp (command, "playpause") == 0) {
            /* toggle play/pause */
            if (mpd_status_get_state (status) == MPD_STATE_PLAY) {
                /* pause */
                success = mpd_run_pause (connection, true);
            } else {
                /* play */
                success = mpd_run_play (connection);
            }
        } else if (strcmp (command, "next") == 0) {
            success = mpd_run_next (connection);
        } else if (strcmp (command, "prev") == 0) {
            success = mpd_run_previous (connection);
        } else if (strcmp (command, "stop") == 0) {
            success = mpd_run_stop (connection);
        } else if (strcmp (command, "delete") == 0) {
            if ((mpd_status_get_state (status) == MPD_STATE_PLAY) || (mpd_status_get_state (status) == MPD_STATE_PAUSE)) {
                int songpos = mpd_status_get_song_pos (status);

                if (irmpc_options.debug) {
                    printf ("deleting song at songpos: %d\n", songpos+1);
                }

                success = mpd_run_delete (connection, songpos);
            } else {
                success = true;
            }
        } else if (strcmp (command, "playlistupdate") == 0) {
            irmpc_mpd_playlist_update ();
            success = true;
        } else if ((strcmp (command, "nextplaylist") == 0)) {
            irmpc_mpd_playlist_nextprev (1);
            success = true;
        } else if ((strcmp (command, "prevplaylist") == 0)) {
            irmpc_mpd_playlist_nextprev (-1);
            success = true;
        } else if ((strcmp (command, "nextalbum") == 0) || (strcmp (command, "prevalbum") == 0)) {
            int queuelen = mpd_status_get_queue_length (status);
            int songpos  = mpd_status_get_song_pos (status);

            int searchdir = ((strcmp (command, "nextalbum") == 0) ? 1 : -1);
            if (searchdir < 0) songpos--;

            if ((queuelen > 0) && (songpos >= 0)) {
                struct mpd_song *current_song = mpd_run_get_queue_song_pos (connection, songpos);

                if (current_song != NULL) {
                    const char *current_album = mpd_song_get_tag (current_song, MPD_TAG_ALBUM, 0);

                    if (irmpc_options.debug) {
                        printf ("INFO: song pos: %d - current album: %s, queue length: %d, search direction: %d\n", songpos, current_album, queuelen, searchdir);
                    }

                    int next_songpos;
                    bool album_found = false;

                    for (next_songpos = songpos; (next_songpos >= 0) && (next_songpos < queuelen); next_songpos += searchdir) {
                        struct mpd_song *this_song = mpd_run_get_queue_song_pos (connection, next_songpos);
                        if (this_song == NULL) continue;
                        const char *this_album = mpd_song_get_tag (this_song, MPD_TAG_ALBUM, 0);

                        if ((this_album == NULL) || (current_album == NULL)) {
                            /* one with album tag and one without -> "found" */
                            album_found = true;
                        } else {
                            if (strcmp (this_album, current_album) != 0) {
                                album_found = true;
                            }
                        }

                        if (irmpc_options.debug) {
                            printf ("INFO: song pos: %d - album: %s - found: %d\n", next_songpos, this_album, album_found);
                        }

                        mpd_song_free (this_song);
                        if (album_found) break;
                    }

                    mpd_song_free (current_song);

                    if (searchdir < 0) {
                        if (!album_found) {
                            album_found  = true;
                            next_songpos = 0;
                        } else {
                            next_songpos++;
                        }
                    }

                    if (irmpc_options.debug) {
                        printf ("INFO: target found: %d, target song pos: %d\n", album_found, next_songpos);
                    }

                    if (album_found) {
                        success = mpd_run_play_pos (connection, next_songpos);
                    } else {
                        success = true;
                    }
                }
            } else {
                success = true;
            }
        } else {
            fprintf (stderr, "WARNING: ignoring command \"m:%s\" - unknown.\n", command);
            success = true;
        }

        if (status != NULL) {
            mpd_status_free (status);
        }
    }
}

/* name of currently loaded playlist */
static const char *playlist_current_name = NULL;

/* load given playlist */
static void irmpc_mpd_playlist (const struct playlist_info *playlist)
{
    bool success = false;
    int  tries   = 0;
    while ((!success) && (tries < irmpc_options.mpd_maxtries)) {
        tries++;

        if (! irmpc_connection_check ()) continue;

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

    if (success) {
        playlist_current_name = playlist->name;
    } else {
        playlist_current_name = NULL;
    }
}

/* load next/prev playlist */
static void irmpc_mpd_playlist_nextprev (int direction)
{
    const struct playlist_info *playlist = irmpc_playlist_nextprev (direction, playlist_current_name);

    if (irmpc_options.debug) {
        printf ("INFO: current playlist: %s\n", playlist_current_name);
        printf ("INFO: next    playlist: %s\n", playlist->name);
    }

    irmpc_mpd_playlist (playlist);
}

/* timestamp for last key press */
static int    playlist_update_last_press = -1;
static time_t playlist_update_last_time;

/* update current playlist */
static void irmpc_mpd_playlist_update ()
{
    if (playlist_current_name == NULL) return;

    time_t this_time = time (NULL);

    int playlist_update_press = 1;

    if (playlist_update_last_press >= 0) {
        double timediff = difftime (this_time, playlist_update_last_time);
        if (irmpc_options.debug) {
            printf ("INFO: timediff to last press: %fs\n", timediff);
        }
        if (timediff <= irmpc_options.lirc_key_timespan) {
            playlist_update_press += playlist_update_last_press;
        }
    }

    if (playlist_update_press >= irmpc_options.mpd_update_amount) {
        if (irmpc_options.debug) {
            printf ("INFO: updating playlist: %s\n", playlist_current_name);
        }

        bool success = false;
        int  tries   = 0;
        while ((!success) && (tries < irmpc_options.mpd_maxtries)) {
            tries++;

            if (! irmpc_connection_check ()) continue;

            success = mpd_run_save (connection, playlist_current_name);
            if (!success) {
                /* playlist might exist - try deleting */
                if (irmpc_options.debug) {
                    if (mpd_connection_get_error (connection) != MPD_ERROR_SUCCESS) {
                        fprintf (stderr, "ERROR: %s\n", mpd_connection_get_error_message (connection));
                    }
                    printf ("INFO: playlist %s might exist - trying to delete\n", playlist_current_name);
                }
                mpd_connection_clear_error (connection);
                success = mpd_run_rm (connection, playlist_current_name);
                if (!success) {
                    if (irmpc_options.debug) {
                        if (mpd_connection_get_error (connection) != MPD_ERROR_SUCCESS) {
                            fprintf (stderr, "ERROR: %s\n", mpd_connection_get_error_message (connection));
                        }
                    }
                    continue;
                }
                if (irmpc_options.debug) {
                    printf ("INFO: second try to save playlist %s\n", playlist_current_name);
                }
                success = mpd_run_save (connection, playlist_current_name);
                if (!success) continue;
            }
        }

        playlist_update_press = 0;
    }

    playlist_update_last_press = playlist_update_press;
    playlist_update_last_time  = time (NULL);
}

/* last number entered + timestamp for multi-digit numbers */
static int    playlist_num_last = -1;
static time_t playlist_num_last_time;

/* handle number key presses for playlist loading */
void irmpc_mpd_playlist_key (int key)
{
    time_t this_time = time (NULL);

    if ((key < 0) || (key > 9)) return;

    unsigned int number = key;

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

/* last volume setting for volume/mute */
static bool last_mute   = false;
static int  last_volume = 100;

/* volume/mute commands */
void irmpc_mpd_volume (const char *command)
{
    bool success = false;
    int  tries   = 0;
    while ((!success) && (tries < irmpc_options.mpd_maxtries)) {
        tries++;

        if (! irmpc_connection_check ()) continue;

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
            fprintf (stderr, "WARNING: ignoring command \"v:%s\" - unknown.\n", command);
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


/* free connection struct */
void irmpc_mpd_free () {
    if (connection != NULL) {
        mpd_connection_free (connection);
        connection = NULL;
    }
}

