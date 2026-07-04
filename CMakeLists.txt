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

find_package(GPM)

if (!GPM_FOUND)
add_definitions("-D_NO_GPM")
endif()

set(PROJECT_VERSION 5.6.0)

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

if (GPM_FOUND)
target_link_libraries(vdk-shared ${CURSES_LIBRARIES} ${GPM_LIBRARIES})
else()
target_link_libraries(vdk-shared ${CURSES_LIBRARIES})
endif()

SET_TARGET_PROPERTIES(vdk-static PROPERTIES OUTPUT_NAME vdk CLEAN_DIRECT_OUTPUT 1)
SET_TARGET_PROPERTIES(vdk-shared PROPERTIES OUTPUT_NAME vdk CLEAN_DIRECT_OUTPUT 1 VERSION ${PROJECT_VERSION} SOVERSION 5)

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

add_subdirectory(demo)
