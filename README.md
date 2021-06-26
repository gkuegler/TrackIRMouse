# TrackIRMouse

This program controls the mouse using pitch and yaw data from the Natural Point TrackIR5.
Rotational coordinates are mapped to the Windows virtual desktop in an absolute fashion, so the mouse always moves to where the user is looking.

[Python Version](https://github.com/georgekuegler/TrackIR5PyMouse)

# How It Works:
The user picks comfortable pitch and yaw boundaries for each active monitor.
The program poles Windows for the virtual desktop coordinates of each monitor and does some translation (see the section on the virtual desktop if you're interested).
The rotational boundaries are mapped to each monitor's window in the virtual desktop.
Every loop iteration, a simple linear interpolation between monitor edges determines the mouse position.

![monitor diagram](https://github.com/georgekuegler/TrackIRMouse/blob/master/docs/Windows%20Desktop%20Diagram-Model.png)

Here's an example of my settings file to show you what's comfortable for me.
```
[default_padding]
left = 3
right = 3
top = 0
bottom = 0

[display0]
left = -43.00
right = 50.00
top = 19.00
bottom = -19.00

[display1]
left = 62.00
right = 180.00
top = 19.00
bottom = -19.00
```

# Notes On Similar Software:

FreePIE is an excellent tool that I started with. It is the first piece of software I used to get going quickly.
Unfortunately, it does not support the Win32 call, 'SendInput'. This is necessary for any pen enabled applications and some UWP applications to register drawing. These applications use the Windows messaging system to get updates about mouse movement within the application window. I believe the common FreePIE function for moving the mouse, SetCursorPosition, does not cause the appropriate Windows messages is to be sent to the target program. Thus I can't markups PDFs with some programs.

# The Virtual Screen
...TODO explain Microsoft's quirky virtual desktop and the need to translate coordinates...
Link: (https://docs.microsoft.com/en-us/windows/win32/gdi/the-virtual-screen)
