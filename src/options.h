#ifndef __options_h__
#define __options_h__

#include <stdbool.h>

struct _irmpc_options {
    char        *config_file;

    char        *mpd_hostname;
    char        *mpd_password;
    unsigned int mpd_port;
    unsigned int mpd_maxtries;

    char        *lirc_config;
    int          lirc_key_timespan;

    char        *progname;

    bool         verbose;
    bool         debug;
};

extern struct _irmpc_options irmpc_options;

bool irmpc_parse_options (int *argc, char ***argv);

#endif
