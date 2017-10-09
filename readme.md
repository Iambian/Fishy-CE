Fishy CE
========

A game about being a small fish in a big pond.
You must eat the smaller fish to grow larger while not getting eaten by larger fish.
Grow to be the largest fish ever.

Warning
-------

* This program is only for the TI-84 Plus CE.
* This will not work on the TI-84 Plus C Silver Edition.
* This will not work on any monochrome calculator.

Installing and Running
----------------------

Use your favorite computer to calculator link software (TiLP, TIConnect, etc)
to send FISHY.8xp to your calculator.

To run without a shell, follow these steps:
1. Turn on the calculator and make sure you're on the home screen.
2. Then push the following keys in order:
   [CLEAR] [2nd] [0] [DOWN] [DOWN] [DOWN] [DOWN] [DOWN] [DOWN] [ENTER]
3. Push [PRGM] then move the cursor down until FISHY is selected.
   When you have done that, push [ENTER]
4. Your homescreen should look something like:

   `Asm(prgmFISHY)`

   Then push [ENTER] to start the game.

Controls
--------
On the title screen:

| Keys   |  Function         |
|-------:|:------------------|
|[Mode]  | Quit game         |
|[2nd]   | Start game        |
|[Del]   | Show in-game help |

In the game:
   
|   Keys                        |  Function                |
|------------------------------:|:-------------------------|
|[Left], [Right], [Down], [Up]  | Move the fish            |
|[Mode]                         | Go back to title screen  |


Troubleshooting
---------------
1. * Q: When I run the game, the calculator gives me `ERROR:ARCHIVED`
   * A: Follow the grey text prompt to unachive the variable.

2. * Q: When I run the game, the calculator is complaining
        about needing this "Libload" or something.
   * A: You need the libload library. Follow the link provided in the
        error message to find the needed libraries.
		
3. * Q: When I run the game, the calculator is complaining about
        library version or something like that.
   * A: You need the library specified in the error message.
		Follow the link provided in the
        error message to find the needed libraries.

License
-------
I use the BSD license for this project. See `LICENSE` for more details.

Version History
---------------
* 0.1 - Initial release to Cemetech
* 0.2 - I don't know what I did wrong but the provided executable didn't
        want to work right. It does if you recompile with the included
		source so I took this as an excuse to upload what I had to GitHub.
		
________________________________________________________________________________

Building the Game from Source
-----------------------------

* If you haven't already, download and install the TI-CE development
  toolchain found here: https://github.com/CE-Programming/toolchain/releases
  
* Get the source for Fishy here:
  https://github.com/Iambian/Fishy-CE
  
* Compile the graphics. Navigate to `src/gfx`, then type `convpng` in the
  Explorer window's address bar or run it from that folder in a console window.
  
* Build the project. Navigate to the project's root folder (where makefile is)
  and type `make` in the Explorer window's address bar, or type that in a
  console window open to that folder.
  
* If the build is successful, the result should be in the `bin` folder.
