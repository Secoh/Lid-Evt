LID-EVT
=======

A version of the `Laplock` (see below), extended for running a script or program as the reaction to closing the laptop lid. This feature is missing in Windows 7 through Windows 10.

TODO: This is command-line utility. Describe parameters. It can be run from command line or in Windows startup scripts. (SK! Provide example of startup setup.) Quotes are accepted in the file name. Running `lid-evt` without action parameters is equivalent to `-lock` option, for compatibility.

lid-evt [-run <script_to_run>] [-lock] [-report <script_to_run>] [-log <file>] | -kill | -help

`Lid-Evt` is derivative work based on the `Laplock` code and thus retains the same license, GPL 3.0. See the note of the original author below (technical edit).<br>
Honorable mention: `Lidlock` at https://github.com/linusyang92/lidlock

laplock
=======

Author: Etienne Dechamps (a.k.a e-t172) <etienne@edechamps.fr>

https://github.com/dechamps/laplock

This extremely small (102 lines, 21KB binary) C++ program allows you to automatically lock your Windows computer on two events:
 - The computer is a laptop and its lid is closed;
 - The computer screen is turned off. Note that this probably won't work with the power button of your monitor since the system is not notified when this happens; however, it works when the screen is turned off automatically after the delay specified in the Windows power management options.

laplock silently runs in the background on Windows Vista, 7, and probably later versions; however, it doesn't work on Windows XP or earlier. It keeps running until you log off or kill it. It is recommended to add laplock to your Startup folder; then you can happily forget about it.<br>
Note that laplock listens intelligently for events; meaning, it doesn't consume any CPU *at all* while waiting.
