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

set(PROJECT_VERSION 8.0.1)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/vdk ${CMAKE_SOURCE_DIR}/vkmio)

set(VDK_SOURCES
    vdk/vdk_color.c
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
SET_TARGET_PROPERTIES(vdk-shared PROPERTIES OUTPUT_NAME vdk CLEAN_DIRECT_OUTPUT 1 VERSION ${PROJECT_VERSION} SOVERSION 8)

set(PKG_CONFIG_LIBS
    "-lvdk"
)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pkg-config.pc.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
)

install (TARGETS vdk-static vdk-shared DESTINATION lib)
install (FILES vdk.h vkmio.h DESTINATION include)

# Install pkgconfig files to libdata on BSD, otherwise lib
if(CMAKE_SYSTEM_NAME MATCHES "BSD")
    set(PKG_CONFIG_INSTALL_DIR "libdata/pkgconfig")
else()
    set(PKG_CONFIG_INSTALL_DIR "lib/pkgconfig")
endif()
install (FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.pc" DESTINATION ${PKG_CONFIG_INSTALL_DIR})

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

add_subdirectory(demo)
