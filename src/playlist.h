#ifndef __playlist_h__
#define __playlist_h__

#include <stdbool.h>

void irmpc_playlist_add  (unsigned int number, const char *name, bool random);
void irmpc_playlist_free ();
void irmpc_playlist_print_debug ();

#endif
