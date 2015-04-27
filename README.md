SICSOPHONE is a real-time, IP-based
system for point-to-point delivery of audio between computer
end-systems.

Sicsophone is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

The aim of the sicsophone is to provide a free and open telephony
platform.  The name sicsophone is of historical reasons. It wasn't
really developed at SICS. We just didnt find a better name than that
at the time of writing a paper. Maybe we should call it fiveophone?

The directory contains the following files and directories:

audio		Audio-interface specific modules
coding		Audio-encoding modules
configure	File generated from configure.in with autoconf
configure.in	Meta file for autoconf configuration
COPYING		GPL 2 license
docs		Documentation. There is a useful architecture description.
include		Include files
src		Source code
targets		Sub-directory containing binaries for several target archs.
README		This file

The code is more or less modularized. More in the parts where it
should be extensible. Less in the parts that are not expected to
change. The extensible parts are audio and coding. These two
sub-directories contain independent modules that implement different
audio interfaces (eg /dev/audio, MS DirectSound, file emulation, etc);
and coding algorithms (eg PCM, G711 ulaw and mlaw...).

The extensible modules follow a pseudo object-oriented methodology and
it should be enough to change in these directories when you wish to
add a new module.  You may also have to change in the user interfaces
(for obvious reasons).  The audio and coding directories contain
README's on how to add a new module. The architecture description in
the docs directory may also be useful.

Otherwise, the code follows two basic tracks illustrated by the two
example applications: sphone_main_send and sphone_main_rcv.  The first
(sphone_main_send) records audio samples, encodes them and
encapsulates them and sends them on an RTP/UDP/IP socket. The second
(sphone_main_rcv) receives RTP/UDP/IP packets, decodes them, arranges
the samples according to a playout algorithm (fifo or shbuf) and then
plays them.

There is currently no (1) signaling (2) GUI. Please help.

Other code weaknesses can be seen in the docs/todo.txt file.

To get going:
1. cd to target directory:
Example:
> cd targets/win32

2. Execute configure with correct parameters (check with configure --help
Example:
> ../../configure

3. Compile sources into executables:
> make

4. Run the executables:
> cd app
> sphone_main_rcv &
> sphone_main_send &

