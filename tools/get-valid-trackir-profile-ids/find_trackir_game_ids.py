"""
Finding valid game ID numbers registered with the Natural Point
TrackIR 5 program.
"""
from trackir_lib import TrackIRDLL
import tkinter

with open("trackir_game_ids.txt", "a") as f:

    # NaturalPoint TrackIR needs a window to register with.
    # To speed up checking game id's, we unregister the window handle to
    # clear the game title so we can immediately try another.
    app = tkinter.Tk()
    app.title("TrackIR Mouse Controller")
    app.update_idletasks()
    app.update()
    tkinter.Label(app, text="Running Mouse Controller. Close to stop").grid(
        column=0, row=0)

    tracker = TrackIRDLL(app.wm_frame())

    # deregister current window handle before we start
    tracker.NP_UnregisterWindowHandle()

    for i in range(0, 65535):
        tracker.NP_RegisterWindowHandle(tracker.hWnd)
        result = tracker.NP_RegisterProgramProfileID(i)
        print("%d: %d" % (i, result))

        if result == 0:  # game client exists
            f.write("%d\n" % i)
        elif result == 7:
            print("error, couldn't register, already in use")
            exit()

        tracker.NP_UnregisterWindowHandle()
