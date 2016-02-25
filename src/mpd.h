#ifndef __mpd_hv_
#define __mpd_h__

void irmpc_mpd_command (const char *command);
void irmpc_mpd_playlist_key (int key);
void irmpc_mpd_volume (const char *command);

void irmpc_mpd_free ();

#endif
