# headless tests: each is a program that exits 0 when its checks pass
add_executable(test_cells test_cells.c)

target_include_directories(test_cells PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(test_cells vdk-static -lutil ${CURSES_LIBRARIES})

add_test(NAME test_cells COMMAND test_cells)

add_executable(test_destroy test_destroy.c)

target_include_directories(test_destroy PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(test_destroy vdk-static -lutil ${CURSES_LIBRARIES})

add_test(NAME test_destroy COMMAND test_destroy)
