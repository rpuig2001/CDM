# Unit tests
find_package(GTest CONFIG REQUIRED)

ADD_EXECUTABLE(unit_tests
    test_main.cpp
)
TARGET_LINK_LIBRARIES(unit_tests PRIVATE
    ${PROJECT_NAME}_static
    GTest::gtest
    GTest::gtest_main 
    GTest::gmock 
    GTest::gmock_main
)
TARGET_INCLUDE_DIRECTORIES(unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/external/include
)
INCLUDE(GoogleTest)
gtest_discover_tests(unit_tests
    PROPERTIES
    LABELS "unit"
)

# # Integration tests
# ADD_EXECUTABLE(integration_tests
#     test_main.cpp
# )
# TARGET_LINK_LIBRARIES(integration_tests PRIVATE
#     ${PROJECT_NAME}_static
#     GTest::gmock_main
#     GTest::gtest_main
#     GTest::gtest
# )
# TARGET_INCLUDE_DIRECTORIES(integration_tests PRIVATE
#     ${CMAKE_SOURCE_DIR}/src
#     ${CMAKE_BINARY_DIR}
#     ${CMAKE_SOURCE_DIR}/external/include
# )
# gtest_discover_tests(integration_tests
#     PROPERTIES
#     LABELS "integration"
# )
