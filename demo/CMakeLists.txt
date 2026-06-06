add_executable(vk_demo vk_demo.c)

target_include_directories(vk_demo PRIVATE ${PROJECT_SOURCE_DIR})

if(GPM_FOUND)
    target_link_libraries(vk_demo vdk-static -lutil ${GPM_LIBRARIES} ${CURSES_LIBRARIES})
else()
    target_link_libraries(vk_demo vdk-static -lutil ${CURSES_LIBRARIES})
endif()

add_executable(vk_viewport_demo vk_viewport_demo.c)

target_include_directories(vk_viewport_demo PRIVATE ${PROJECT_SOURCE_DIR})

if(GPM_FOUND)
    target_link_libraries(vk_viewport_demo vdk-static -lutil ${GPM_LIBRARIES} ${CURSES_LIBRARIES})
else()
    target_link_libraries(vk_viewport_demo vdk-static -lutil ${CURSES_LIBRARIES})
endif()
