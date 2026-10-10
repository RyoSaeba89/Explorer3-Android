# Changes to the copy of main.cpp for Android only (the Cardputer project is
# never modified). tools/build.sh checks that each one still applies.

# "Connect the screen": continuing is only offered once mission control is
# connected (user's request); input.cpp also ignores the button until then.
s/^\( *\)hint(tr(ADDR_CONTINUE), tx, 104, C_YELLOW);/\1if (mirror::clientCount() > 0) hint(tr(ADDR_CONTINUE), tx, 104, C_YELLOW);/
