#include "irhandler.h"
#include "options.h"

#include <lirc/lirc_client.h>
#include <stdio.h>

bool irmpc_irhandler ()
{
    struct lirc_config *config;

    if (lirc_init (irmpc_options.progname, 1) == -1) {
        fprintf (stderr, "ERROR: failed to initialize lirc\n");
        return false;
    }

    if (lirc_readconfig (irmpc_options.lirc_config, &config, NULL) != 0) {
        fprintf (stderr, "ERROR: failed to load lirc config file\n");
        goto irmpc_irhandler_error_exit;
    }

    // main loop
    // TODO: lirc reading
    // TODO: mpd controlling

    lirc_deinit ();
    return true;

irmpc_irhandler_error_exit:
    lirc_deinit ();
    return false;
}
