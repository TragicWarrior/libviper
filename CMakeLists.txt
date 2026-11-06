cmake_minimum_required(VERSION 3.10.2)

project(libviper C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fPIC -DNCURSES_OPAQUE=0")
set(PROJECT_DESCRIPTION "A library for developing windowed apps on ncurses")

if (("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang") OR (("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU") AND (CMAKE_C_COMPILER_VERSION VERSION_GREATER 4.0)))
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra")
endif()

# the following is until we learn how to reorder the gcc arguments to correctly link on Ubuntu
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-as-needed")

add_definitions("-D_REENTRANT")
add_definitions("-D_GNU_SOURCE")
add_definitions("-DNCURSES_INTERNALS")

# needed for cchar_t definition and turn on "wide support"
add_definitions("-D_XOPEN_SOURCE_EXTENDED")

set(CURSES_NEED_NCURSES "TRUE")
set(CURSES_NEED_WIDE "TRUE")

find_package(Curses REQUIRED)

find_package(Curses)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# GPM (console mouse) needs no library: vkmio speaks the gpm daemon's
# socket protocol itself.  Configure with -D_NO_GPM in CMAKE_C_FLAGS to
# leave the support out.

set(PROJECT_VERSION 12.4.1)

# vdk.h tells programs which libviper they are building against
# (VDK_VERSION_*).  It has to say what is set here: stop if it does not.
file(STRINGS "${PROJECT_SOURCE_DIR}/vdk.h" _vdk_h_version
     REGEX "^#define VDK_VERSION +\"[0-9]+\\.[0-9]+\\.[0-9]+\"")
string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" _vdk_h_version "${_vdk_h_version}")
file(STRINGS "${PROJECT_SOURCE_DIR}/vdk.h" _vdk_h_parts
     REGEX "^#define VDK_VERSION_(MAJOR|MINOR|PATCH) +[0-9]+")
string(REGEX REPLACE "#define VDK_VERSION_MAJOR +([0-9]+);#define VDK_VERSION_MINOR +([0-9]+);#define VDK_VERSION_PATCH +([0-9]+)"
       "\\1.\\2.\\3" _vdk_h_parts "${_vdk_h_parts}")
if(NOT "${_vdk_h_version}" STREQUAL "${PROJECT_VERSION}"
   OR NOT "${_vdk_h_parts}" STREQUAL "${PROJECT_VERSION}")
    message(FATAL_ERROR
        "vdk.h says VDK_VERSION \"${_vdk_h_version}\" (${_vdk_h_parts}) but "
        "PROJECT_VERSION is ${PROJECT_VERSION}.  Change both together.")
endif()

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/vdk ${CMAKE_SOURCE_DIR}/vkmio)

set(VDK_SOURCES
    vdk/vdk_color.c
    vdk/vdk_cells.c
    vdk/vdk_private.c
    vdk/vk_object.c
    vdk/vk_screen.c
    vdk/vk_widget.c
    vdk/vk_container.c
    vdk/vk_listbox.c
    vdk/vk_selectbox.c
    vdk/vk_dropdown.c
    vdk/vk_frame.c
    vdk/vk_scroller.c
    vdk/vk_window.c
    vdk/vk_box.c
    vdk/vk_grid.c
    vdk/vk_table.c
    vdk/vk_color.c
    vdk/vk_label.c
    vdk/vk_button.c
    vdk/vk_textbox.c
    vdk/vk_marquee.c
    vdk/vk_deck.c
    vdk/vk_filler.c
    vdk/vk_activity.c
    vdk/vk_progress.c
    vdk/vk_meter.c
    vdk/vk_graph.c
    vdk/vk_histogram.c
    vdk/vk_input.c
    vdk/vk_filedialog.c
    vdk/vk_menubar.c
    vdk/vk_calendar.c
    vdk/vk_popup.c
    vdk/vk_viewport.c
    vdk/vk_spinbutton.c
    )

set(VKMIO_SOURCES
    vkmio/vk_kmio.c
    vkmio/vk_zone.c
    vkmio/vk_dblclick.c
    vkmio/vk_drag.c
    )

# --- libvdk (the VDK widget toolkit, the only library this project ships
#     now that the legacy viper_* layer has been retired) ---

add_library(vdk.o OBJECT ${VDK_SOURCES} ${VKMIO_SOURCES})

add_library(vdk-static STATIC $<TARGET_OBJECTS:vdk.o>)
add_library(vdk-shared SHARED $<TARGET_OBJECTS:vdk.o>)

target_link_libraries(vdk-shared ${CURSES_LIBRARIES})

SET_TARGET_PROPERTIES(vdk-static PROPERTIES OUTPUT_NAME vdk CLEAN_DIRECT_OUTPUT 1)
# the soname's number:  the major version
string(REGEX MATCH "^[0-9]+" VDK_SOVERSION "${PROJECT_VERSION}")

SET_TARGET_PROPERTIES(vdk-shared PROPERTIES OUTPUT_NAME vdk CLEAN_DIRECT_OUTPUT 1 VERSION ${PROJECT_VERSION} SOVERSION ${VDK_SOVERSION})

# what a program linking the static library needs as well
set(PKG_CONFIG_LIBS_PRIVATE "-lncursesw -ltinfo")

# @ONLY:  the ${...} in the template are pkg-config's own variables.  the
# prefix is found from where the .pc file sits, so the same file is right
# for /usr/local (make install) and /usr (the packages).
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pkg-config.pc.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
  @ONLY
)

# Every file is given a component, which decides the Debian package it
# goes in:  "runtime" is libvdk12, what a program needs to run, and "dev"
# is libvdk-dev, what is needed to build one.  A plain "make install"
# installs both.
#
# The shared library is installed twice:  its real file and the
# libvdk.so.12 soname link go with the runtime, the bare libvdk.so link a
# linker looks for goes with the development files.
install (TARGETS vdk-shared LIBRARY DESTINATION lib
    COMPONENT runtime NAMELINK_SKIP)
install (TARGETS vdk-shared LIBRARY DESTINATION lib
    COMPONENT dev NAMELINK_ONLY)
install (TARGETS vdk-static ARCHIVE DESTINATION lib COMPONENT dev)
install (FILES vdk.h vkmio.h DESTINATION include COMPONENT dev)

# Debian wants a copyright file (in its machine-readable format) and a
# compressed changelog in each package's own doc directory.  gzip -n
# leaves the time stamp out, so the same source gives the same package.
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    execute_process(
        COMMAND gzip -9 -n -c "${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG"
        OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/changelog.gz")

    install (FILES "${CMAKE_CURRENT_SOURCE_DIR}/packaging/copyright"
        "${CMAKE_CURRENT_BINARY_DIR}/changelog.gz"
        DESTINATION share/doc/libvdk${VDK_SOVERSION} COMPONENT runtime)
    install (FILES "${CMAKE_CURRENT_SOURCE_DIR}/packaging/copyright"
        "${CMAKE_CURRENT_BINARY_DIR}/changelog.gz"
        DESTINATION share/doc/libvdk-dev COMPONENT dev)
endif()

# Install pkgconfig files to libdata on BSD, otherwise lib
if(CMAKE_SYSTEM_NAME MATCHES "BSD")
    set(PKG_CONFIG_INSTALL_DIR "libdata/pkgconfig")
else()
    set(PKG_CONFIG_INSTALL_DIR "lib/pkgconfig")
endif()
install (FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.pc"
    DESTINATION ${PKG_CONFIG_INSTALL_DIR} COMPONENT dev)

# Refresh the dynamic linker's cache after installing, so a new
# libvdk.so.N is found straight away.  Without it a program linked
# against a just-installed major version fails with "cannot open shared
# object file" until someone runs ldconfig by hand.
#
# Skipped for a staged install (DESTDIR set: a packager installs into a
# scratch root, and the package's own scripts run ldconfig on the real
# system).  Never fatal: an install as a non-root user cannot write the
# cache, and that is reported, not treated as a failed install.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    install(CODE [[
        if(NOT "$ENV{DESTDIR}" STREQUAL "")
            message(STATUS "DESTDIR is set: not running ldconfig")
        else()
            find_program(VDK_LDCONFIG ldconfig PATHS /sbin /usr/sbin)
            if(NOT VDK_LDCONFIG)
                message(STATUS "ldconfig not found: linker cache not refreshed")
            else()
                execute_process(COMMAND "${VDK_LDCONFIG}"
                    RESULT_VARIABLE vdk_ldconfig_rc
                    OUTPUT_QUIET ERROR_QUIET)
                if(vdk_ldconfig_rc EQUAL 0)
                    message(STATUS "Ran ldconfig")
                else()
                    message(WARNING
                        "ldconfig could not refresh the linker cache "
                        "(not root?).  Run 'sudo ldconfig' before using "
                        "the newly installed library.")
                endif()
            endif()
        endif()
    ]])
endif()

#
# Debian packages:  "make package" builds two .deb files, no root needed.
#
#   libvdk12     the shared library (Section libs)
#   libvdk-dev   the headers, static library, the libvdk.so link and the
#                pkg-config file, libviper.pc (Section libdevel)
#
# The library's major version is in the runtime package's name, so a new
# major version installs beside the old one instead of replacing it.
# dpkg-shlibdeps works out what each package depends on, and the runtime
# package gets a shlibs file and an ldconfig trigger.
#
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "libvdk")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_CONTACT "Bryan Christ <bryan.christ@gmail.com>")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/TragicWarrior/libviper")
set(CPACK_STRIP_FILES ON)

# directories the packages create are 0755, whatever the builder's umask
set(CMAKE_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE)

set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_ALL runtime dev)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS_POLICY ">=")

# a package description:  the first line is the synopsis, a blank line
# separates paragraphs (CPack writes the " ." Debian wants there).  this
# works because CPACK_PACKAGE_DESCRIPTION_SUMMARY is left unset:  when it
# is set, CPack puts it in front of every package's description.
set(CPACK_DEBIAN_RUNTIME_PACKAGE_NAME "libvdk${VDK_SOVERSION}")
set(CPACK_DEBIAN_RUNTIME_PACKAGE_SECTION "libs")

# the linker cache is refreshed by a dpkg trigger, as Debian asks
set(CPACK_DEBIAN_RUNTIME_PACKAGE_CONTROL_EXTRA
    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/triggers")
set(CPACK_DEBIAN_RUNTIME_DESCRIPTION
"widget toolkit for ncurses programs (VDK)
libviper's VDK is a widget toolkit for console programs built on
ncursesw:  windows, frames, boxes and grids, labels, buttons, inputs,
list and select boxes, menus, dialogs, graphs and more, with mouse
support through xterm reporting or the console's gpm daemon.

This package holds the shared library, libvdk.")

set(CPACK_DEBIAN_DEV_PACKAGE_NAME "libvdk-dev")
set(CPACK_DEBIAN_DEV_PACKAGE_SECTION "libdevel")
set(CPACK_DEBIAN_DEV_PACKAGE_DEPENDS
    "libvdk${VDK_SOVERSION} (= ${PROJECT_VERSION}), libncurses-dev")
set(CPACK_DEBIAN_DEV_DESCRIPTION
"widget toolkit for ncurses programs (VDK) - development files
libviper's VDK is a widget toolkit for console programs built on
ncursesw.

This package holds the headers (vdk.h and vkmio.h), the static library
and the pkg-config file, libviper.pc.")

include(CPack)

add_subdirectory(demo)

# the tests need no terminal; run them with `ctest` (or `make test`)
enable_testing()
add_subdirectory(test)
