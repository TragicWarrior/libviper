ABOUT
=====

Built on top of ncurses, libviper is a convenience layer and a GTK-like
framework for rapidly creating console programs.  It takes care of setting up
the screen, initializing GPM for normal terminal use or Xterm.  The screen
itself can be painted with custom wallpaper (line art of course).  On top of
that libviper managds two "decks" of windows per screen.  These decks are
analogous to a deck of cards.  The decks can be rotated and the window on top
has default focus.  The managed deck can be manipulated by the mouse and is
where normal windows reside.  On top of that is another deck, the unmanaged
deck.  In terms of Z-order, the last window on the unmanaged deck resides
just above the topmost window on the managed deck.  The unmanaged deck is
useful for creating menus and user dialogs.  It's also well suited for
installing hotkeys since user input propogates to the unmanaged deck first. 


BUILDING
========

Building libviper is typically pretty straight forward.

REQUIREMENTS

ncursesw 5.4+

OPTIONAL

libgpm


run the following make commands:                                  

cmake CMakeList.txt
make
sudo make install

BUGS
====

Submit bugs via GitHub


Enjoy!
