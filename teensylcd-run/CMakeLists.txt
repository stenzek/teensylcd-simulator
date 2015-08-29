set(HEADER_FILES 
)

set(SOURCE_FILES
    teensylcd-run.c
)

find_package(SDL2 REQUIRED)

add_executable(teensylcd-run ${HEADER_FILES} ${SOURCE_FILES})
target_include_directories(teensylcd-run PRIVATE . ${SDL2_INCLUDE_DIR})
target_link_libraries(teensylcd-run libteensylcd ${SDL2_LIBRARY} ${SDL2MAIN_LIBRARY})
install(TARGETS teensylcd-run DESTINATION bin)

