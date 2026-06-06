ABOUT
=====

Built on top of ncurses, libviper is a convenience layer and a GTK-like
framework for rapidly creating console programs.  It takes care of setting up
the screen, initializing GPM for normal terminal use or Xterm.

The VK klass system provides a widget toolkit with single inheritance,
virtual dispatch, and constructor/destructor chaining.  Available widgets
include frames, windows, boxes, labels, marquees, listboxes, textboxes,
and scrollers.  Features include virtual surfaces, terminal migration
(teleport to another PTY), and automatic resize propagation.

See KLASSES.md for full API and architecture documentation.


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
