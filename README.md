# Schism Tracker

This is laamaa's fork of Schism Tracker. It adds support for controlling the application/playback with a Novation Launchpad controller and to see playback information with an OLED display on a Raspberry Pi. The aim is to make playing modules without a display possible. Development done in Linux with a Launchpad Mini MK2 and SH1106 based OLED connected via Raspberry PI's I2C pins, no idea if this works under any other platforms.

How to set up Launchpad & Schism:
* Connect the Launchpad via USB
* Open Schism Tracker and select the LP as a Duplex mode device in MIDI config (Shift+F1)
* Quit Schism and reopen, after the restart you should see some lights come up on the controller

* *Scene F* button brings up "Load module" page, select the desired file with grid buttons and load with *Scene H*. Pressing *Scene F* again returns to the previous page.
* *Scene H* starts/stops playback
* The next order can be queued by pressing the corresponding button on the order grid.
* Loop sequences can be created by pressing two orders in the grid simultaneously
* Top row buttons show channel activity and can be used to mute channels. Unmute all by pressing *Scene A*

OLED display support requires the [ArduiPi_OLED library with C headers](https://github.com/destroyedlolo/ArduiPi_OLED) and a compatible I2C display. The autoconf script should recognize if the headers are installed and enable the support if so.

-----

Schism Tracker is a free and open-source reimplementation of [Impulse
Tracker](https://github.com/schismtracker/schismtracker/wiki/Impulse-Tracker),
a program used to create high quality music without the requirements of
specialized, expensive equipment, and with a unique "finger feel" that is
difficult to replicate in part. The player is based on a highly modified
version of the [Modplug](https://openmpt.org/legacy_software) engine, with a
number of bugfixes and changes to [improve IT
playback](https://github.com/schismtracker/schismtracker/wiki/Player-abuse-tests).

Where Impulse Tracker was limited to i386-based systems running MS-DOS, Schism
Tracker runs on almost any platform that [SDL](http://www.libsdl.org/)
supports, and has been successfully built for Linux, Mac OS X, Windows,
FreeBSD, AmigaOS, BeOS, and even [the
Wii](http://www.wiibrew.org/wiki/Schism_Tracker). Schism will most likely build
on *any* architecture supported by GCC4 (e.g. alpha, m68k, arm, etc.) but it
will probably not be as well-optimized on many systems.

See [the wiki](https://github.com/schismtracker/schismtracker/wiki) for more
information.

![screenshot](http://schismtracker.org/screenie.png)

## Download

The latest stable builds for Windows, macOS, and Linux are available from [the
releases page](https://github.com/schismtracker/schismtracker/releases). Builds
can also be installed from some distro repositories on Linux, but these
versions may not have the latest bug fixes and enhancements. Older builds for
other platforms can be found on
[the wiki](https://github.com/schismtracker/schismtracker/wiki). Installing via
Homebrew on macOS is no longer recommended, as the formula for Schism Tracker
is not supported or maintained by anyone directly involved in the project.

## Compilation

See the
[docs/](https://github.com/schismtracker/schismtracker/tree/master/docs) folder
for platform-specific instructions.
