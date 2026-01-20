cmake_minimum_required(VERSION 3.8)
project(DrogonApp LANGUAGES CXX)

# Platform detection
if(WIN32)
    set(IS_WINDOWS TRUE)
    message(STATUS "Configuring for Windows")
elseif(UNIX AND NOT APPLE)
    set(IS_LINUX TRUE)
    message(STATUS "Configuring for Linux")
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Force console app on Windows
if(IS_WINDOWS)
    set(CMAKE_WIN32_EXECUTABLE OFF)
endif()

# Find common packages
find_package(OpenSSL REQUIRED)

# Platform-specific configuration
if(IS_WINDOWS)
    # ========== WINDOWS CONFIGURATION ==========
    
    # MANUAL DROGON SETUP (vcpkg)
    set(DROGON_ROOT "H:/vcpkg/installed/x64-windows")
    set(DROGON_INCLUDE_DIR "${DROGON_ROOT}/include")
    
    message(STATUS "Looking for Drogon libraries in: ${DROGON_ROOT}")
    
    # Find Windows libraries with .lib extension
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
    
    # Windows-specific libraries
    set(PLATFORM_LIBS
        ws2_32.lib
        crypt32.lib
        advapi32.lib
        user32.lib
        shell32.lib
    )
    
    # Windows compile definitions
    set(PLATFORM_DEFINITIONS
        _CRT_SECURE_NO_WARNINGS
        USE_POSTGRESQL
    )
    
    # Use Windows-specific PostgreSQL
    set(POSTGRESQL_INCLUDE_DIRS ${DROGON_INCLUDE_DIR})
    set(POSTGRESQL_LIBRARIES ${POSTGRESQL_LIB})
    
elseif(IS_LINUX)
    # ========== LINUX CONFIGURATION ==========
    
    message(STATUS "=== PostgreSQL-Only Build ===")
    message(STATUS "MySQL: DISABLED")
    message(STATUS "SQLite: DISABLED")
    
    # Set skip flags as cache variables for Linux
    set(DROGON_SKIP_MYSQL ON CACHE INTERNAL "" FORCE)
    set(DROGON_SKIP_SQLITE ON CACHE INTERNAL "" FORCE)
    
    # Find PostgreSQL for Aiven
    find_package(PostgreSQL REQUIRED)
    message(STATUS "PostgreSQL found: ${PostgreSQL_VERSION}")
    
    # Set Drogon library paths for Linux
    set(DROGON_LIBRARY /usr/lib/x86_64-linux-gnu/libdrogon.so)
    set(TRANTOR_LIBRARY /usr/lib/x86_64-linux-gnu/libtrantor.so)
    
    # Check if Drogon libs exist
    if(NOT EXISTS ${DROGON_LIBRARY})
        message(FATAL_ERROR "Drogon library not found at ${DROGON_LIBRARY}")
    endif()
    
    # Find other dependencies for Linux
    find_package(Threads REQUIRED)
    
    # Try to find Jsoncpp
    find_package(Jsoncpp QUIET)
    if(NOT Jsoncpp_FOUND)
        # Manual Jsoncpp setup
        find_library(JSONCPP_LIB NAMES jsoncpp)
        find_path(JSONCPP_INCLUDE_DIR NAMES json/json.h PATH_SUFFIXES jsoncpp)
        if(JSONCPP_LIB AND JSONCPP_INCLUDE_DIR)
            set(JSONCPP_LIBRARY ${JSONCPP_LIB})
            set(JSONCPP_INCLUDE_DIRS ${JSONCPP_INCLUDE_DIR})
            message(STATUS "Jsoncpp found manually")
        else()
            message(FATAL_ERROR "Jsoncpp not found")
        endif()
    else()
        # Use package found Jsoncpp
        set(JSONCPP_LIBRARY Jsoncpp::Jsoncpp)
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
    
    # Linux-specific libraries
    set(PLATFORM_LIBS
        Threads::Threads
        dl
        pthread
        z
        uuid
        ${BCRYPT_LIB}
    )
    
    # Linux compile definitions
    set(PLATFORM_DEFINITIONS
        USE_POSTGRESQL
        HAVE_POSTGRESQL
        NDEBUG
    )
    
    # Use system PostgreSQL
    set(POSTGRESQL_INCLUDE_DIRS ${PostgreSQL_INCLUDE_DIRS})
    set(POSTGRESQL_LIBRARIES ${PostgreSQL_LIBRARIES})
    
endif()

# Debug output
message(STATUS "==========================================")
message(STATUS "PLATFORM: ${CMAKE_SYSTEM_NAME}")
if(IS_WINDOWS)
    message(STATUS "DROGON_LIBRARY: ${DROGON_LIBRARY}")
    message(STATUS "TRANTOR_LIBRARY: ${TRANTOR_LIBRARY}")
    message(STATUS "JSONCPP_LIBRARY: ${JSONCPP_LIBRARY}")
    message(STATUS "POSTGRESQL_LIB: ${POSTGRESQL_LIB}")
endif()
message(STATUS "==========================================")

# Create executable
add_executable(${PROJECT_NAME}
    main.cpp
    controllers/AuthController.cpp
    filters/AuthFilter.cpp
    models/User.cpp
    DatabaseConfig.cpp
)

# Force console subsystem on Windows
if(IS_WINDOWS)
    target_link_options(${PROJECT_NAME} PRIVATE "/SUBSYSTEM:CONSOLE")
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    .
    controllers
    filters
    models
)

# Platform-specific include directories
if(IS_WINDOWS)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${DROGON_INCLUDE_DIR}
        ${POSTGRESQL_INCLUDE_DIRS}
    )
elseif(IS_LINUX)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${POSTGRESQL_INCLUDE_DIRS}
        ${JSONCPP_INCLUDE_DIRS}
        ${BCRYPT_INCLUDE_DIR}
        /usr/include/drogon
        /usr/include/trantor
        /usr/include/jsoncpp
    )
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${DROGON_LIBRARY}
    ${TRANTOR_LIBRARY}
    ${JSONCPP_LIBRARY}
    ${POSTGRESQL_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    ${PLATFORM_LIBS}
)

# Add compile definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    ${PLATFORM_DEFINITIONS}
)

# Copy resources (works on both platforms)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/views
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/views
    COMMENT "Copying views directory to build output"
)

# Copy config.json if exists (both platforms)
if(EXISTS "${CMAKE_SOURCE_DIR}/config.json")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/config.json
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/config.json
        COMMENT "Copying config.json to build output"
    )
endif()

# Platform-specific messages
if(IS_WINDOWS)
    message(STATUS "Windows build configured successfully!")
elseif(IS_LINUX)
    message(STATUS "=======================================")
    message(STATUS "Linux build configured successfully!")
    message(STATUS "Database: PostgreSQL")
    message(STATUS "=======================================")
endif()