set(SOURCE_FILES
    main.c
)

add_executable(nonrecursive ${SOURCE_FILES})
target_link_libraries(nonrecursive io err atomic)
