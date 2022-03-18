"""
This script toggles the sending of mouse input for my active tracker client.
I use previous releases of this app to control my mouse while developing.
I also code by voice using dragon/natlink/dragonfly/caster.

Internally, my active client should still grab tracking data frames as normal.
No api calls to the NPTrackIR dll are made from my active client by the
running of this script. See source code relating to "watchdog" to see how
messages are handled.
"""

import NamedPipe

# TODO: remove dependency on personal NamedPipe module
# convert named pipe c++ code into native python
# should make this more portable and transparent for future developement.
rsp = NamedPipe.ClientSendMessage("\\\\.\\pipe\\watchdog", "PAUSE")
print(rsp)
