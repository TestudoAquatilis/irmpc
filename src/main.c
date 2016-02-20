#include "options.h"
#include "playlist.h"
#include "irhandler.h"

int main (int argc, char **argv)
{
    if (!irmpc_parse_options (&argc, &argv)) {
        goto exit_error;
    }

    // main loop ...
    irmpc_irhandler ();

    // TODO: signal catching

    irmpc_playlist_free ();

    return 0;

exit_error:
    irmpc_playlist_free ();
    return 1;
}
