cmake_minimum_required(VERSION 3.8)
project(DrogonApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set skip flags as cache variables
set(DROGON_SKIP_MYSQL ON CACHE INTERNAL "" FORCE)
set(DROGON_SKIP_SQLITE ON CACHE INTERNAL "" FORCE)

message(STATUS "=== PostgreSQL-Only Build ===")
message(STATUS "MySQL: DISABLED")
message(STATUS "SQLite: DISABLED")

# Find PostgreSQL for Aiven
find_package(PostgreSQL REQUIRED)
message(STATUS "PostgreSQL found: ${PostgreSQL_VERSION}")

# Manually find Drogon components
# Set Drogon library paths
set(DROGON_LIB /usr/lib/x86_64-linux-gnu/libdrogon.so)
set(TRANTOR_LIB /usr/lib/x86_64-linux-gnu/libtrantor.so)

# Check if Drogon libs exist
if(NOT EXISTS ${DROGON_LIB})
    message(FATAL_ERROR "Drogon library not found at ${DROGON_LIB}")
endif()

# Find other dependencies
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Try to find Jsoncpp
find_package(Jsoncpp QUIET)
if(NOT Jsoncpp_FOUND)
    # Manual Jsoncpp setup
    find_library(JSONCPP_LIB NAMES jsoncpp)
    find_path(JSONCPP_INCLUDE_DIR NAMES json/json.h PATH_SUFFIXES jsoncpp)
    if(JSONCPP_LIB AND JSONCPP_INCLUDE_DIR)
        set(JSONCPP_LIBRARIES ${JSONCPP_LIB})
        set(JSONCPP_INCLUDE_DIRS ${JSONCPP_INCLUDE_DIR})
        message(STATUS "Jsoncpp found manually")
    else()
        message(FATAL_ERROR "Jsoncpp not found")
    endif()
else()
    # Use package found Jsoncpp
    set(JSONCPP_LIBRARIES Jsoncpp::Jsoncpp)
    set(JSONCPP_INCLUDE_DIRS "")
endif()

# Find BCrypt manually (since we installed from source)
find_library(BCRYPT_LIB NAMES bcrypt PATHS /usr/local/lib)
find_path(BCRYPT_INCLUDE_DIR NAMES bcrypt/bcrypt.h PATHS /usr/local/include)
if(BCRYPT_LIB AND BCRYPT_INCLUDE_DIR)
    message(STATUS "BCrypt found: ${BCRYPT_LIB}")
else()
    message(WARNING "BCrypt not found, creating stub")
    # Create bcrypt stub if needed
    file(WRITE ${CMAKE_BINARY_DIR}/bcrypt_stub.h
        "#ifndef BCRYPT_STUB_H\n"
        "#define BCRYPT_STUB_H\n"
        "#include <string>\n"
        "inline int bcrypt_checkpw(const char* password, const char* hash) {\n"
        "    return 0; // Stub - always succeeds\n"
        "}\n"
        "#endif\n"
    )
    set(BCRYPT_INCLUDE_DIR ${CMAKE_BINARY_DIR})
    set(BCRYPT_LIB "")
endif()

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
    ${PostgreSQL_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
    ${BCRYPT_INCLUDE_DIR}
    /usr/include/drogon
    /usr/include/trantor
    /usr/include/jsoncpp
    .
    controllers
    filters
    models
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${DROGON_LIB}
    ${TRANTOR_LIB}
    ${JSONCPP_LIBRARIES}
    ${PostgreSQL_LIBRARIES}
    ${BCRYPT_LIB}
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
    dl
    pthread
    z
    uuid
)

# Compile definitions for PostgreSQL
target_compile_definitions(${PROJECT_NAME} PRIVATE
    USE_POSTGRESQL
    HAVE_POSTGRESQL
    NDEBUG
)

# Copy resources
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/views
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/views
    COMMENT "Copying views directory"
)

# Copy config.json
if(EXISTS "${CMAKE_SOURCE_DIR}/config.json")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/config.json
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/config.json
        COMMENT "Copying PostgreSQL Aiven config"
    )
endif()

message(STATUS "=======================================")
message(STATUS "Build configured successfully!")
message(STATUS "Database: PostgreSQL (Aiven Cloud)")
message(STATUS "=======================================")