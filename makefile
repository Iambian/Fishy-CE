# ----------------------------
# Set NAME to the program name
# Set ICON to the png icon file name
# Set DESCRIPTION to display within a compatible shell
# Set COMPRESSED to "YES" to create a compressed program
# ----------------------------

NAME        = FISHY
ICON        = icon.png
DESCRIPTION = "Fishy"
COMPRESSED  = YES
COMPRESSED_MODE = zx0
HAS_PRINTF  = NO
ARCHIVED    = NO

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

include $(shell cedev-config --makefile)
