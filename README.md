# irmpc

irmpc is an MPD client using commands from infra-red controls via LIRC

# Requirements

You need
- libmpdclient
- lirc-client library
- glib version 2

# Build

In src directory run:

> make

For debugging without remote control you can change the Makefile (commented lines) to
use some primitive commandline like mode where you can enter the command strings otherwise
received via lirc.

# Usage

You can start irmpc as a daemon (using the systemd file) or directly.
Options can be specified via a config file (example in cfg) or as command line arguments.
Playlists must be specified in the config file (see example).
Lirc command assignment is done via a lircrc file (see example in cfg).

# Configuration

You might want to adapt the config file to match your mpd config
(especially hostname, password, playlists) and the lircrc file to match
your remote control key names.

