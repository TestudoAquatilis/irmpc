#ifndef __options_h__
#define __options_h__

#include <stdbool.h>

struct _irmpc_options {
    const char  *config_file;

    const char  *mpd_hostname;
    const char  *mpd_password;
    unsigned int mpd_port;
    unsigned int mpd_maxtries;
    unsigned int mpd_update_amount;

    unsigned int volume_step;

    const char  *lirc_config;
    unsigned int lircd_tries;
    unsigned int lirc_key_timespan;

    const char  *power_command;
    unsigned int power_amount;

    const char  *progname;

    bool         verbose;
    bool         debug;
};

extern struct _irmpc_options irmpc_options;

bool irmpc_parse_options (int *argc, char ***argv);

#endif
