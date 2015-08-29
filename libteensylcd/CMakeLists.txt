set(HEADER_FILES 
    pcd8544.h
    teensylcd.h
    timer.h
)

set(SOURCE_FILES
    pcd8544.c
    teensylcd.c
    timer.c
)

add_library(libteensylcd ${HEADER_FILES} ${SOURCE_FILES})
target_include_directories(libteensylcd PRIVATE .)
target_include_directories(libteensylcd PUBLIC .)
target_link_libraries(libteensylcd simavr)
install(TARGETS libteensylcd DESTINATION lib)

