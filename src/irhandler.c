#include "irhandler.h"
#include "options.h"
#include "mpd.h"

#ifndef DEBUG_NO_LIRC
#include <lirc/lirc_client.h>
#endif

#include <stdio.h>
#include <stdlib.h>

bool irmpc_irhandler ()
{
#ifndef DEBUG_NO_LIRC
    struct lirc_config *config;

    if (lirc_init (irmpc_options.progname, 1) == -1) {
        fprintf (stderr, "ERROR: failed to initialize lirc\n");
        return false;
    }

    if (lirc_readconfig (irmpc_options.lirc_config, &config, NULL) != 0) {
        fprintf (stderr, "ERROR: failed to load lirc config file\n");
        goto irmpc_irhandler_error_exit;
    }

    /* main loop */
    char *code;

    while (lirc_nextcode (&code) == 0) {
        if (code == NULL) continue;

        int ret;
        char *c = NULL;

        while (((ret = lirc_code2char (config, code, &c)) == 0) && (c != NULL)) {
#else
        char c [1024];
        while (true) {
            scanf("%s", c);
#endif
            if (irmpc_options.debug) {
                printf ("Got command: \"%s\"\n", c);
            }

            if ((c[0] == 'm') && (c[1] == ':')) {
                /* mpd command */
                irmpc_mpd_command (&(c[2]));
            } else if ((c[0] == 'v') && (c[1] == ':')) {
                /* volume command */
            } else if ((c[0] == 's') && (c[1] == ':')) {
                /* system command */
            } else if ((c[0] == 'p') && (c[1] == ':')) {
                /* playlist command */
            }

            /* TODO: mpd controlling */
#ifdef DEBUG_NO_LIRC
        }
        return true;
#else
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
#endif
}
