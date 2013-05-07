---------------------------------
| Overview of MultiCam solution |
---------------------------------


DeviceEnumerator:
----------------

A standalone executable (DeviceEnumerator.exe) for enumerating
DirectShow VideoInput devices installed on the current machine.  

The code is based closely on the code and article at http://www.codeproject.com/Articles/6445/Enumerate-Installed-Devices-Using-Setup-API, written by A. Riazi.

To run, execute DeviceEnumerator.exe from a command line with no
commandline arguments.

Gma.UserActivityMonitor:
-----------------------

A library written by George Mamaladze for processing global mouse and
keyboard hooks using C#. See http://www.codeproject.com/Articles/7294/Processing-Global-Mouse-and-Keyboard-Hooks-in-C

MulticamExe:
-----------

The MultiCam executable.  Communicates with the MultiCam DLL running
in a video chat process (usually Skype) -- see the MultiCam technical
report for details.

MultiCamFilter:
--------------

The MultiCam DLL.  A video chat executable (usually Skype) will load
this DLL when MultiCam is selected as a camera input.  See the MultiCam technical
report for details.


MultiCamHelpers:
---------------

Some helper code for the MultiCam executable.

PlayCap:
-------

A standalone executable (PlayCap.exe) for testing video devices such
as webcams and MultiCam. This is an altered version of the DirectShow
PlayCap demo.  To use, run PlayCap.exe from the command line with a
single argument specifying the name of the video device has captured
video should be displayed. The name of the device is the so-called
friendly name, as output by the above DeviceEnumerator.exe
executable. For example:

PlayCap.exe "Microsoft LifeCam VX-3000"
PlayCap.exe MultiCam

With multiple arguments, this executable can also be used to connect
multiple DirectShow filters in a chain.  However, this functionality
is not needed for testing MultiCam.

SkypeControl:
------------

A library for using interprocess communication to communicate with a
local instance of Skype.  The library is an altered version of code
written by Gabriel Szabo, and described in his article "Controlling
Skype with C#"
(http://www.codeproject.com/Articles/13081/Controlling-Skype-with-C).

vcam:
----

Creates a virtual camera DLL.  This is a fake camera that will be
recognized as a video device by DirectShow. The camera outputs random
colors at each pixel. The friendly name of the camera is "Virtual Cam
(use before/after MultiCam)" -- this can be used, for example, with
the PlayCap executable described above.  The code is an altered
version of code originally downloaded from
http://tmhare.mvps.org/downloads.htm.

The main purpose of this "vcam" virtual camera is to enable testing of
virtual cameras with video chat programs such as Skype, without having
to worry about any actual hardware. You can, for example, select vcam
as the video input for Skype.

vcam-lib:
--------

Library code containing various helper functions for working with
DirectShow virtual cameras, Skype, key hooks, and logging.

MultiCamSetup:
-------------

Code for creating a Windows installer for MultiCam.

