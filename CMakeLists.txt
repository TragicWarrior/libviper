cmake_minimum_required(VERSION 2.8)

project(libviper)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fPIC -s")
set(PROJECT_DESCRIPTION "A library for developing windowed apps on ncurses")

if (("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang") OR (("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU") AND (CMAKE_C_COMPILER_VERSION VERSION_GREATER 4.0)))
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra")
endif()

# the following is until we learn how to reorder the gcc arguments to correctly link on Ubuntu
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-as-needed")

add_definitions("-D_REENTRANT")
add_definitions("-D_GNU_SOURCE")

set(CURSES_NEED_NCURSES "TRUE")
find_package(Curses REQUIRED)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(GPM)

if (!GPM_FOUND)
add_definitions("-D_NO_GPM")
endif()


add_library(viper-static STATIC
    strings.c
    viper_private.c
    viper_callbacks.c
    viper_color.c
    viper_deck.c
    viper_events.c
    viper_keyboard.c
    viper_kmio.c
    viper_msgbox.c
    viper_screen.c
    viper_states.c
    viper_userptr.c
    viper_wcreate.c
    viper_wdecorate.c
    viper_wdestroy.c
    viper_wfind.c
    viper_wmove.c
    viper_wresize.c
    viper_wtraits.c
    w_create.c
    w_decorate.c
    w_fill.c
    w_metrics.c
    w_move.c
    w_shadow.c
    vk_object.c
    vk_widget.c
    vk_container.c
    vk_listbox.c
    )

add_library(viper.o OBJECT
    strings.c
    viper_private.c
    viper_callbacks.c
    viper_color.c
    viper_deck.c
    viper_events.c
    viper_keyboard.c
    viper_kmio.c
    viper_msgbox.c
    viper_screen.c
    viper_states.c
    viper_userptr.c
    viper_wcreate.c
    viper_wdecorate.c
    viper_wdestroy.c
    viper_wfind.c
    viper_wmove.c
    viper_wresize.c
    viper_wtraits.c
    w_create.c
    w_decorate.c
    w_fill.c
    w_metrics.c
    w_move.c
    w_shadow.c
    vk_object.c
    vk_widget.c
    vk_container.c
    vk_listbox.c
    )

add_library(viper-shared SHARED
    $<TARGET_OBJECTS:viper.o>
    )


if (GPM_FOUND)
target_link_libraries(viper-shared viper-static -lutil ${GPM_LIBRARIES} ${CURSES_LIBRARIES} )
else()
target_link_libraries(viper-shared viper-static -lutil ${CURSES_LIBRARIES} )
endif()

# CMake doesn't allow targets with the same name.  This renames them properly afterward.
SET_TARGET_PROPERTIES(viper-static PROPERTIES OUTPUT_NAME viper CLEAN_DIRECT_OUTPUT 1)
SET_TARGET_PROPERTIES(viper-shared PROPERTIES OUTPUT_NAME viper CLEAN_DIRECT_OUTPUT 1)

# set the subst vars that configure_file() will use
set(PROJECT_VERSION 1.0)

set(PKG_CONFIG_LIBS
    "-lviper"
)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pkg-config.pc.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
)

install (TARGETS viper-static viper-shared DESTINATION lib)
install (FILES viper.h DESTINATION include)

# Install pkgconfig files to libdata on BSD, otherwise lib
if(CMAKE_SYSTEM_NAME MATCHES "BSD")
    set(PKG_CONFIG_INSTALL_DIR "libdata/pkgconfig")
else()
    set(PKG_CONFIG_INSTALL_DIR "lib/pkgconfig")
endif()
install (FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.pc" DESTINATION ${PKG_CONFIG_INSTALL_DIR})
