cmake_minimum_required(VERSION 2.8)

project(libviper)

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

find_package(GPM)

if (!GPM_FOUND)
add_definitions("-D_NO_GPM")
endif()

set(PROJECT_VERSION 4.0.0)
set(VDK_VERSION 1.0.0)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/vdk ${CMAKE_SOURCE_DIR}/vkmio)

set(VDK_SOURCES
    vdk/vdk_color.c
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
    vdk/vk_label.c
    vdk/vk_button.c
    vdk/vk_textbox.c
    vdk/vk_marquee.c
    vdk/vk_deck.c
    vdk/vk_filler.c
    vdk/vk_activity.c
    vdk/vk_input.c
    vdk/vk_filedialog.c
    vdk/vk_menubar.c
    vdk/vk_calendar.c
    vdk/vk_popup.c
    )

set(VKMIO_SOURCES
    vkmio/vk_kmio.c
    vkmio/vk_zone.c
    vkmio/vk_dblclick.c
    vkmio/vk_drag.c
    )

set(VIPER_SOURCES
    strings.c
    viper_private.c
    viper_callbacks.c
    viper_color.c
    viper_deck.c
    viper_events.c
    viper_screen.c
    viper_states.c
    viper_wdestroy.c
    viper_wmove.c
    w_create.c
    w_decorate.c
    w_fill.c
    w_metrics.c
    w_move.c
    w_shadow.c
    )

# --- libvdk (standalone VDK widget toolkit) ---

add_library(vdk.o OBJECT ${VDK_SOURCES} ${VKMIO_SOURCES})

add_library(vdk-static STATIC $<TARGET_OBJECTS:vdk.o>)
add_library(vdk-shared SHARED $<TARGET_OBJECTS:vdk.o>)

if (GPM_FOUND)
target_link_libraries(vdk-shared ${CURSES_LIBRARIES} ${GPM_LIBRARIES})
else()
target_link_libraries(vdk-shared ${CURSES_LIBRARIES})
endif()

SET_TARGET_PROPERTIES(vdk-static PROPERTIES OUTPUT_NAME vdk CLEAN_DIRECT_OUTPUT 1)
SET_TARGET_PROPERTIES(vdk-shared PROPERTIES OUTPUT_NAME vdk CLEAN_DIRECT_OUTPUT 1 VERSION ${VDK_VERSION} SOVERSION 1)

# --- libviper (legacy + VDK) ---

add_library(viper-static STATIC ${VIPER_SOURCES} ${VDK_SOURCES} ${VKMIO_SOURCES})

add_library(viper.o OBJECT ${VIPER_SOURCES} ${VDK_SOURCES} ${VKMIO_SOURCES})

add_library(viper-shared SHARED $<TARGET_OBJECTS:viper.o>)

if (GPM_FOUND)
target_link_libraries(viper-shared viper-static -lutil ${GPM_LIBRARIES} ${CURSES_LIBRARIES} )
else()
target_link_libraries(viper-shared viper-static -lutil ${CURSES_LIBRARIES} )
endif()

# CMake doesn't allow targets with the same name.  This renames them properly afterward.
SET_TARGET_PROPERTIES(viper-static PROPERTIES OUTPUT_NAME viper CLEAN_DIRECT_OUTPUT 1)
SET_TARGET_PROPERTIES(viper-shared PROPERTIES OUTPUT_NAME viper CLEAN_DIRECT_OUTPUT 1)

set(PKG_CONFIG_LIBS
    "-lviper"
)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pkg-config.pc.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
)

install (TARGETS vdk-static vdk-shared viper-static viper-shared DESTINATION lib)
install (FILES viper.h vdk.h vkmio.h DESTINATION include)

# Install pkgconfig files to libdata on BSD, otherwise lib
if(CMAKE_SYSTEM_NAME MATCHES "BSD")
    set(PKG_CONFIG_INSTALL_DIR "libdata/pkgconfig")
else()
    set(PKG_CONFIG_INSTALL_DIR "lib/pkgconfig")
endif()
install (FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.pc" DESTINATION ${PKG_CONFIG_INSTALL_DIR})

add_subdirectory(demo)
