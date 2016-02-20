#include "irhandler.h"
#include "options.h"

#include <lirc/lirc_client.h>
#include <stdio.h>
#include <stdlib.h>

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
    char *code;

    while (lirc_nextcode (&code) == 0) {
        if (code == NULL) continue;

        int ret;
        char *c = NULL;

        while (((ret = lirc_code2char (config, code, &c)) == 0) && (c != NULL)) {
            // TODO: mpd controlling
            printf ("Got command: \"%s\"\n", c);
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
}
