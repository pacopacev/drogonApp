cmake_minimum_required(VERSION 3.8)
project(DrogonApp LANGUAGES CXX)

# Force console app on Windows
if(WIN32)
    set(CMAKE_WIN32_EXECUTABLE OFF)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(PostgreSQL REQUIRED)  # This finds libpq
find_package(OpenSSL REQUIRED)

# MANUAL DROGON SETUP
set(DROGON_ROOT "H:/vcpkg/installed/x64-windows")
set(DROGON_INCLUDE_DIR "${DROGON_ROOT}/include")

# Debug: Check what libraries exist
message(STATUS "Looking for Drogon libraries in: ${DROGON_ROOT}")

# Try to find the actual library files
find_library(DROGON_LIBRARY
    NAMES drogon drogon.lib
    PATHS "${DROGON_ROOT}/lib"
    REQUIRED
)

find_library(TRANTOR_LIBRARY
    NAMES trantor trantor.lib
    PATHS "${DROGON_ROOT}/lib"
    REQUIRED
)

find_library(JSONCPP_LIBRARY
    NAMES jsoncpp jsoncpp.lib
    PATHS "${DROGON_ROOT}/lib"
    REQUIRED
)

# Also look for PostgreSQL library (libpq)
find_library(POSTGRESQL_LIB
    NAMES pq libpq pq.lib libpq.lib
    PATHS "${DROGON_ROOT}/lib"
    REQUIRED
)

message(STATUS "==========================================")
message(STATUS "LIBRARIES FOUND:")
message(STATUS "  Drogon: ${DROGON_LIBRARY}")
message(STATUS "  Trantor: ${TRANTOR_LIBRARY}")
message(STATUS "  JsonCpp: ${JSONCPP_LIBRARY}")
message(STATUS "  PostgreSQL (libpq): ${POSTGRESQL_LIB}")
message(STATUS "==========================================")

# Create executable
add_executable(${PROJECT_NAME} 
    main.cpp
    controllers/AuthController.cpp
    filters/AuthFilter.cpp
    models/User.cpp
    DatabaseConfig.cpp
)

# Force console subsystem
if(WIN32)
    target_link_options(${PROJECT_NAME} PRIVATE "/SUBSYSTEM:CONSOLE")
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${DROGON_INCLUDE_DIR}
    ${PostgreSQL_INCLUDE_DIRS}
    .
    controllers
    filters
    models
)

# Link libraries - use the found PostgreSQL library
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${DROGON_LIBRARY}
    ${TRANTOR_LIBRARY}
    ${JSONCPP_LIBRARY}
    ${POSTGRESQL_LIB}  # Use the found libpq library
    OpenSSL::SSL
    OpenSSL::Crypto
    ws2_32.lib
    crypt32.lib
    advapi32.lib
    user32.lib
    shell32.lib
)

# Add PostgreSQL compile definition
target_compile_definitions(${PROJECT_NAME} PRIVATE 
    _CRT_SECURE_NO_WARNINGS
    USE_POSTGRESQL
)

# Copy views directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/views
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/views
    COMMENT "Copying views directory to build output"
)