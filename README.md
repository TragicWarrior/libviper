ABOUT
=====

libviper is the home of VDK (Viper Development Kit), a klass-based
widget toolkit for ncurses applications.  Built on top of ncursesw, it
provides a GTK-like framework with single-inheritance classes, virtual
dispatch, and constructor/destructor chaining for rapidly building
console programs.

As of 5.0.0 the project ships a single library, libvdk; the original
viper_* layer has been retired.  The repository name and pkg-config
module (libviper) are preserved for continuity.  See CHANGELOG for the
project history.

For API and architecture documentation see KLASSES.md.


FEATURES
========

Widgets

  Base                vk_object (RTTI, ctor/dtor chaining), vk_widget
  Layout              vk_frame, vk_window, vk_box, vk_grid, vk_table,
                      vk_deck (z-order with drop shadows), vk_container
  Primitives          vk_label, vk_marquee, vk_button, vk_input,
                      vk_spinbutton, vk_filler
  Content             vk_listbox, vk_selectbox (checkbox / radio),
                      vk_textbox, vk_dropdown, vk_menubar, vk_calendar,
                      vk_activity, vk_color
  Composites          vk_filedialog, vk_popup, vk_viewport
  Attachment          vk_scroller

Screen

  - vk_screen_t with virtual surfaces (multi-desktop), wallpaper
    callback, per-surface background, and screen overlay
  - Terminal migration (teleport) via vk_screen_teleport /
    vk_screen_evict_pty; widgets auto-repaint through the
    _on_recreate hook

I/O

  - vkmio keyboard / mouse layer driven by write(2) on a
    caller-supplied fd (no stdio dependency)
  - xterm 1003 any-event mouse tracking, GPM connection take-over,
    GPM wheel handling, mouse zone / double-click / drag utilities,
    queued event coalescing
  - Widgets are I/O-agnostic; default handlers overridable via
    vk_object_set_kmio()

Color

  - vdk_color_init() / vdk_color_pair() with 16x16 ANSI pair tier
    and extended 256-color pair registration

Style

  - Border styles SINGLE / DOUBLE / ASCII / NONE plus REVERSE and
    RELIEF_RAISED / RELIEF_SUNKEN modifiers
  - Configurable relief and shadow colors


REQUIREMENTS
============

CMake
ncursesw 5.4+

Optional: libgpm (for system-console mouse support)


BUILDING
========

cmake CMakeLists.txt
make
sudo make install

Installs libvdk.{a,so} to ${CMAKE_INSTALL_PREFIX}/lib, public headers
vdk.h and vkmio.h to .../include, and a pkg-config file (libviper.pc)
for downstream consumers.


DEMOS
=====

Two demo programs are built alongside the library under demo/:

  vk_demo            broad widget tour with multi-desktop surfaces
                     and dialog usage
  vk_viewport_demo   scrollable canvas via vk_viewport_t


BUGS
====

Submit bugs via GitHub.


Enjoy!
