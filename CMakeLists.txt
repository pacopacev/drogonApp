cmake_minimum_required(VERSION 3.8)
project(DrogonApp)

# Find PostgreSQL
find_package(PostgreSQL REQUIRED)
find_package(OpenSSL REQUIRED)

# MANUAL DROGON SETUP - vcpkg's DrogonConfig.cmake is broken!
set(DROGON_ROOT "H:/vcpkg/installed/x64-windows")
set(DROGON_INCLUDE_DIR "${DROGON_ROOT}/include")
set(DROGON_LIBRARY "${DROGON_ROOT}/lib/drogon.lib")

# Find Trantor (Drogon's underlying library)
set(TRANTOR_LIBRARY "${DROGON_ROOT}/lib/trantor.lib")

# Find JsonCpp
set(JSONCPP_LIBRARY "${DROGON_ROOT}/lib/jsoncpp.lib")

if(NOT EXISTS "${DROGON_LIBRARY}")
    message(FATAL_ERROR "Drogon library not found: ${DROGON_LIBRARY}")
endif()

if(NOT EXISTS "${TRANTOR_LIBRARY}")
    message(FATAL_ERROR "Trantor library not found: ${TRANTOR_LIBRARY}")
endif()

if(NOT EXISTS "${JSONCPP_LIBRARY}")
    message(FATAL_ERROR "JsonCpp library not found: ${JSONCPP_LIBRARY}")
endif()

message(STATUS "==========================================")
message(STATUS "MANUAL DROGON SETUP:")
message(STATUS "  Drogon library: ${DROGON_LIBRARY}")
message(STATUS "  Trantor library: ${TRANTOR_LIBRARY}")
message(STATUS "  JsonCpp library: ${JSONCPP_LIBRARY}")
message(STATUS "  PostgreSQL: ${PostgreSQL_LIBRARIES}")
message(STATUS "==========================================")

# Create executable
add_executable(${PROJECT_NAME} 
    main.cpp
    controllers/AuthController.cpp
    filters/AuthFilter.cpp
    models/User.cpp
    DatabaseConfig.cpp
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${DROGON_INCLUDE_DIR}
    ${PostgreSQL_INCLUDE_DIRS}
    .
    controllers
    filters
    models
)

# Link ALL required libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${DROGON_LIBRARY}
    ${TRANTOR_LIBRARY}
    ${JSONCPP_LIBRARY}
    ${PostgreSQL_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    ws2_32.lib
    crypt32.lib
    advapi32.lib
    user32.lib
    shell32.lib
)

# CRITICAL: Add PostgreSQL compile definition
target_compile_definitions(${PROJECT_NAME} PRIVATE 
    _CRT_SECURE_NO_WARNINGS
    USE_POSTGRESQL
)

# C++ standard
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Copy views directory to build output
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/views
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/views
    COMMENT "Copying views directory to build output"
)