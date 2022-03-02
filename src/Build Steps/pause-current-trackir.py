import NamedPipe

rsp = NamedPipe.ClientSendMessage("\\\\.\\pipe\\watchdog", "PAUSE")
print(rsp)
