LID-EVT
=======

A version of the `Laplock` (see below), extended for running a script or program as the reaction to closing the laptop lid. This feature is missing in Windows 7 through Windows 10.

Just like the original Laplock, this is window-less utility capable of accepting the command line parameters. User can either start LID-EVT for monitoring, or terminate the running process, or read help. Running `lid-evt` without action parameters is equivalent to `-lock` option, for compatibility.
LID-EVT will not run twice, and it will not run with conflicting parameters.

```
lid-evt [-run <script_to_run>] [-lock] [-log <file>] | -kill | -help

  -lock    Lock screen when lid is closed, or monitor turned off.
  -run     Run script when lid is closed. Needs full path and name to executable file.
           The script can be any executable Windows file. Starting directory is undefined.
  -log     Append log of power events into a file. Needs full path and name to file.
  -kill    Terminate previously launched LID-EVT. No effect if not present.
  -help    Show this text in separate console window.
```

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
