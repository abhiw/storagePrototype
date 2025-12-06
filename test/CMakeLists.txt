# Test executable
set(TEST_CLASSES
        sample_test sample_test.cpp
        types_test types_test.cpp
        checksum_test checksum_test.cpp
        page_test page_test.cpp
        slot_directory_test slot_directory_test.cpp
        alignment_test alignment_test.cpp
        schema_test.cpp
        logger_test logger_test.cpp
        file_handle_test file_handle_test.cpp
        disk_manager_test disk_manager_test.cpp
        free_space_map_test free_space_map_test.cpp
        page_compaction_test page_compaction_test.cpp
)

set(SOURCES
        ../include/common/config.h
        ../include/common/types.h
        ../include/common/checksum.h
        ../src/common/checksum.cpp
        ../include/common/logger.h
        ../src/common/logger.cpp
        ../include/common/file_handle.h
        ../src/common/file_handle.cpp
        ../include/page/page.h
        ../src/page/page.cpp
        ../include/page/page_view.h
        ../src/page/page_view.cpp
        ../include/schema/alignment.h
        ../src/schema/alignment.cpp
        ../include/schema/schema.h
        ../src/schema/schema.cpp
        ../include/storage/disk_manager.h
        ../src/storage/disk_manager.cpp
        ../include/storage/free_space_map.h
        ../src/storage/free_space_map.cpp
        schema_test.cpp
)

add_executable(sample_test
        ${SOURCES}
        ${TEST_CLASSES}
)

# Link GoogleTest (gtest_main already includes gtest)
target_link_libraries(sample_test
        GTest::gtest_main
)

# Discover and register all individual GoogleTest cases
include(GoogleTest)
gtest_discover_tests(sample_test
    PROPERTIES
        ENVIRONMENT "STORAGE_ENGINE_LOG_DIR=${PROJECT_SOURCE_DIR}/logs"
)
#gtest_discover_tests(types_test)
