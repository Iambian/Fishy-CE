Independent C Project #6 -- FISHY - Developer's ReadME
-----------------------------------------------------------------------------
_______________________________________
 ..:: Warning ::..

This program is only for the TI-84 Plus CE.
This will not work on the TI-84 Plus C Silver Edition.
This will not work on any monochrome calculator.

_______________________________________
 ..:: About ::..

This short readme details how to rebuild FISHY for yourself since it
really isn't as simple as typing "make". Yet.

_______________________________________
 ..:: How to Build ::..
 
First of all, you'll want to download the CE Programming toolchain.
https://github.com/CE-Programming/toolchain/releases/tag/v7.3
 
Once you have all that set up, if you've modified any of the graphics
you'll need to manually run convpng from the src/gfx folder. On my
Win7 system, I would open an explorer window up to that folder,
Shift-RightClick then select "Open command window here".
Then simply type "convpng" (no quotes) and push ENTER.

To actually build the project once you've gotten the graphics
squared away open a command window in the project's root directory
and run the makefile. On my Win7 system, that is as simple as
typing "make" (no quotes), then pushing ENTER.

The results you're probably looking for is in the bin folder.
