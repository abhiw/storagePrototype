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
        page_update_test page_update_test.cpp
        page_manager_test page_manager_test.cpp
        field_value_test field_value_test.cpp
        tuple_header_test tuple_header_test.cpp
        tuple_serializer_test tuple_serializer_test.cpp
        tuple_builder_test tuple_builder_test.cpp
        tuple_accessor_test tuple_accessor_test.cpp
        tuple_integration_test tuple_integration_test.cpp
        crud_integration_test crud_integration_test.cpp
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
        ../include/storage/page_manager.h
        ../src/storage/page_manager.cpp
        ../include/tuple/field_value.h
        ../src/tuple/field_value.cpp
        ../include/tuple/tuple_header.h
        ../src/tuple/tuple_header.cpp
        ../include/tuple/tuple_serializer.h
        ../src/tuple/tuple_serializer.cpp
        ../include/tuple/tuple_builder.h
        ../src/tuple/tuple_builder.cpp
        ../include/tuple/tuple_accessor.h
        ../src/tuple/tuple_accessor.cpp
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
