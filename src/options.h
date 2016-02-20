#ifndef __options_h__
#define __options_h__

#include <stdbool.h>

struct _irmpc_options {
    char *config_file;

    char *mpd_hostname;
    char *mpd_password;
    int  mpd_port;

    char *lirc_config;

    bool verbose;
    bool debug;
};

extern struct _irmpc_options irmpc_options;

bool irmpc_parse_options (int *argc, char ***argv);

#endif
