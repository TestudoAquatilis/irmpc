#ifndef __playlist_h__
#define __playlist_h__

#include <stdbool.h>

struct playlist_info {
    const char *name;
    bool        random;
};

void irmpc_playlist_add  (unsigned int number, const char *name, bool random);
const struct playlist_info * irmpc_playlist_get  (unsigned int number);

void irmpc_playlist_free ();
void irmpc_playlist_print_debug ();

#endif
