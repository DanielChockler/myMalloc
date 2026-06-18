set(ALLOCATOR_SOURCES
    Allocator/Allocator.cpp
    Allocator/Allocator.h
)

add_executable(myMalloc main.cpp ${ALLOCATOR_SOURCES})
