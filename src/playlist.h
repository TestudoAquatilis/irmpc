#ifndef __playlist_h__
#define __playlist_h__

#include <stdbool.h>

void playlist_add  (unsigned int number, const char *name, bool random);
void playlist_free ();
void playlist_print_debug ();

#endif
