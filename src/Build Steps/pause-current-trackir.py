"""
This script toggles the sending of mouse input for my active tracker client.
I use previous releases of this app to control my mouse while developing.
I also code by voice using dragon/natlink/dragonfly/caster.

Internally, my active client should still grab tracking data frames as normal.
No api calls to the NPTrackIR dll are made from my active client by the
running of this script. See source code relating to "watchdog" to see how
messages are handled.
"""


import win32file, win32pipe, win32api
import pywintypes
import logging
import threading
import sys
from time import sleep


class PipeServerClient:
    """
    Base client for synchronous communications with the named pipe server.

    Args:
        name (str): the name path of the pipe server
        encoding (str): one of python's built-in string encoding types
            can also be 'True' if overriding 'encode' and 'decode' methods
        clear_pipe_before_send (bool): clears the pipe before sending data if true
        log_level (str): logging log_level of client
        t (float): timeout for waiting for a response
    """

    def __init__(
        self,
        name,
        encoding="utf-8",  # can be set to true or false
        clear_pipe_before_send=True,
        log_level=logging.DEBUG,
        timeout_s=10,
        message_buffer_size=512,
    ):
        self.name = "\\\\.\\pipe\\" + name
        self.clear_pipe_before_send = clear_pipe_before_send
        self.encoding = encoding
        self.timeout_s = timeout_s
        self.message_buffer_size = message_buffer_size

        # create formatters and console output handler for the logger
        self.logger = logging.getLogger("py-" + name)
        formatter = logging.Formatter("[%(name)s][%(levelname)s]: %(message)s")
        console = logging.StreamHandler(sys.stdout)
        console.setFormatter(formatter)
        self.logger.addHandler(console)
        self.logger.setLevel(logging.DEBUG)
        # self.logger.disabled = True

    def encode(self, message):
        """formats the message into a byte string to send to the server"""
        return message.encode(encoding=self.encoding, errors="strict")

    def decode(self, message):
        """decodes the byte string received from the server"""
        return message.decode(encoding=self.encoding, errors="strict")

    def send(self, message, callback=None):
        assert isinstance(message, str) or isinstance(message, bytes)
        assert callback == None or callable(callback)
        # TODO: need one more log_level of threading

        # if the messages already a byte string, don't encode
        # allow user to send bytes directly
        data = self.encode(message) if not isinstance(message, bytes) else message

        # I have to return control back to Dragon NaturallySpeaking immediately
        threading.Thread(target=self._send, args=(data, callback)).start()
        return  # return immediatly

    def _send(self, data, callback=None):
        # TODO: use a futures object here?

        t = threading.Thread(target=self._write_to_pipe, args=(data,))
        t.start()
        t.join(timeout=self.timeout_s)

        # thread should have closed when message was received
        # if t.is_alive():
        #     self.logger.critical("No return message received in time.")
        #     return

        # decode the message or use the raw bytes received
        # allow user to receive response in bytese
        # response = (
        #     self.decode(self._server_response_in_bytes)
        #     if self.encoding
        #     else self._server_response_in_bytes
        # )
        # print(response)
        return  # nothing

    def _write_to_pipe(self, data, callback=None):
        """Writes data to a named pipe and blocks for return message."""
        if not isinstance(data, bytes):
            self.logger.critical("data was not bytes")
            return

        if len(data) > self.message_buffer_size:
            self.logger.critical("data too big for buffer")
            return

        while True:
            try:
                pipe = win32file.CreateFile(
                    self.name,
                    win32file.GENERIC_READ | win32file.GENERIC_WRITE,
                    0,
                    None,  # security attributes, need to match pipe
                    win32file.OPEN_EXISTING,
                    win32file.FILE_FLAG_SEQUENTIAL_SCAN,
                    None,
                )

                self.logger.info(f"send message: {data}")
                # write to the pipe
                result = win32file.WriteFile(pipe, data)

                # I can't find out what error code 122 means
                # but everything works fine when it's thrown.
                if result[0] != 0 and (err := win32api.GetLastError()) != 122:
                    self.logger.critical(
                        f"Failed writing to the pipe.\nGet Last Error: {err}"
                    )

                # I don't have any named pipe servers designed to return anything near 1024 bytes
                rc, bytes_received = win32file.ReadFile(pipe, 1024)
                self.logger.info(f"bytes_received: {bytes_received}")

                # TODO: do the call back here? print result?

            # waiting on python 3.10 match statement!
            except pywintypes.error as e:
                error_code = e.args[0]
                if error_code == 2 or error_code == 3:
                    self.logger.critical("pipe doesn't exist")
                elif error_code == 109:
                    self.logger.critical("broken pipe, bye bye")
                elif error_code == 231:
                    self.logger.info("pipe instance is/are busy")
                    sleep(0.2)
                    continue
                else:
                    self.logger.critical(f"unrecoverable error: {e}")
                self._server_response_in_bytes = None

            break


if __name__ == "__main__":
    client = PipeServerClient(name="watchdog", message_buffer_size=512)
    client.send("PAUSE")
