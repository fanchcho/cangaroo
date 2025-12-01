cmake_minimum_required(VERSION 3.10)

# Project
project(Cangaroo LANGUAGES CXX C)
message(STATUS "Project: ${PROJECT_NAME}")

# Qt
find_package(Qt5 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    SerialPort
    Xml
    Charts
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Enable automoc/autouic/autorcc so .ui/.qrc and Q_OBJECT are handled automatically
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Default to Debug if not provided
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Build type not set, defaulting to Debug")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Option to exclude building PCAN adapter (matching .pro conditional)
option(EXCLUDE_PCAN "Exclude building PCAN adapter" OFF)

# Option to disable dark mode
option(DISABLE_DARK_MODE "Disable dark mode support" ON)

# Automatically collect source files from src/ and canifconfig/ directories
file(GLOB_RECURSE PROJECT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

# Exclude Windows-only CandleApiDriver on non-Windows platforms
if(NOT WIN32)
    list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*CandleApiDriver/.*")
endif()

# Exclude Linux-only SocketCanDriver on non-Linux platforms
if(NOT (UNIX AND NOT APPLE))
    list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*SocketCanDriver/.*")
endif()

# Only include canifconfig on Linux (it uses linux/if.h)
if(UNIX AND NOT APPLE)
    file(GLOB_RECURSE CANIFCONFIG_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/canifconfig/*.c"
    )
    list(APPEND PROJECT_SOURCES ${CANIFCONFIG_SOURCES})
endif()

# Conditional: PCAN adapter
if(NOT EXCLUDE_PCAN)
    message(STATUS "Including PCAN adapter (BUILD_PCAN)")
    add_compile_definitions(BUILD_PCAN)
else()
    message(STATUS "Excluding PCAN adapter")
    # Remove PCAN adapter source if it exists
    list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*PeakCanDriver\\.cpp$")
endif()

# Conditional: Dark mode
if(DISABLE_DARK_MODE)
    message(STATUS "Dark mode disabled")
    add_compile_definitions(DISABLE_DARK_MODE)
else()
    message(STATUS "Dark mode enabled")
endif()

# Automatically collect header files from src/ and canifconfig/ directories
file(GLOB_RECURSE PROJECT_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/canifconfig/*.h"
)

# Exclude Windows-only CandleApiDriver headers on non-Windows platforms
if(NOT WIN32)
    list(FILTER PROJECT_HEADERS EXCLUDE REGEX ".*CandleApiDriver/.*")
endif()

# Exclude Linux-only SocketCanDriver headers on non-Linux platforms
if(NOT (UNIX AND NOT APPLE))
    list(FILTER PROJECT_HEADERS EXCLUDE REGEX ".*SocketCanDriver/.*")
endif()

# Automatically collect UI forms from src/ and canifconfig/ directories
file(GLOB_RECURSE PROJECT_FORMS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/canifconfig/*.ui"
)

# Collect Qt resource files (.qrc) for autorcc
file(GLOB_RECURSE PROJECT_RESOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.qrc"
)

# Create executable target
add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${PROJECT_SOURCES} ${PROJECT_HEADERS} ${PROJECT_FORMS} ${PROJECT_RESOURCES})

# Set output name to match original TARGET if desired
set_target_properties(${PROJECT_NAME} PROPERTIES 
    OUTPUT_NAME "Cangaroo"
    AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/src;${CMAKE_CURRENT_SOURCE_DIR}/src/window;${CMAKE_CURRENT_SOURCE_DIR}/src/window/CanCfgWindow;${CMAKE_CURRENT_SOURCE_DIR}/src/window/CanStatusWindow;${CMAKE_CURRENT_SOURCE_DIR}/src/window/GraphWindow;${CMAKE_CURRENT_SOURCE_DIR}/src/window/LogWindow;${CMAKE_CURRENT_SOURCE_DIR}/src/window/RawTxWindow;${CMAKE_CURRENT_SOURCE_DIR}/src/window/SetupDialog;${CMAKE_CURRENT_SOURCE_DIR}/src/window/TraceWindow;${CMAKE_CURRENT_SOURCE_DIR}/src/driver"
)

# macOS bundle configuration
if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_INFO_STRING "Cangaroo CAN Bus Analyzer"
        MACOSX_BUNDLE_BUNDLE_NAME "Cangaroo"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.github.cangaroo"
        MACOSX_BUNDLE_BUNDLE_VERSION "0.1"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1"
        MACOSX_BUNDLE_COPYRIGHT "Copyright © 2015-2025"
        MACOSX_BUNDLE_INFO_PLIST_ADDITIONS "
            <key>NSAppleEventsUsageDescription</key>
            <string>Cangaroo needs access to Apple events to open files and communicate with the system.</string>
            <key>NSDocumentsFolderUsageDescription</key>
            <string>Cangaroo needs access to documents folder to open CAN database files (.dbc).</string>
            <key>NSDesktopFolderUsageDescription</key>
            <string>Cangaroo needs access to desktop folder to open CAN database files (.dbc).</string>
            <key>NSDownloadsFolderUsageDescription</key>
            <string>Cangaroo needs access to downloads folder to open CAN database files (.dbc).</string>
        "
    )
endif()

# Include directories (project subdirs)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/canifconfig
)

# Link Qt libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
    Qt5::SerialPort
    Qt5::Xml
    Qt5::Charts
)

# Link local libPCBUSB if present - platform-specific library selection
set(PCAN_API_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/driver/PeakCanDriver/api)

if(WIN32)
    # Windows: use .dll
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(LOCAL_PCB_LIB ${PCAN_API_DIR}/PCANBasic.dll)
    else()
        set(LOCAL_PCB_LIB ${PCAN_API_DIR}/PCANBasic.dll)
    endif()
elseif(APPLE)
    # macOS: use .dylib
    set(LOCAL_PCB_LIB ${PCAN_API_DIR}/OSX/libPCBUSB.0.13.dylib)
elseif(UNIX)
    # Linux: use .so
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(LOCAL_PCB_LIB ${PCAN_API_DIR}/libpcanbasic.so)
    else()
        set(LOCAL_PCB_LIB ${PCAN_API_DIR}/libpcanbasic.so)
    endif()
endif()

if(EXISTS ${LOCAL_PCB_LIB})
    message(STATUS "Found local PCAN library: ${LOCAL_PCB_LIB}")
    # Link against the local library
    target_link_libraries(${PROJECT_NAME} PRIVATE ${LOCAL_PCB_LIB})
    
    if(APPLE)
        get_filename_component(LOCAL_PCB_LIB_NAME ${LOCAL_PCB_LIB} NAME)
        
        # Copy dylib into app bundle and fix install names for proper bundling
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Frameworks"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LOCAL_PCB_LIB}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Frameworks/${LOCAL_PCB_LIB_NAME}"
            # Fix the dylib id to use @rpath
            COMMAND install_name_tool -id "@rpath/${LOCAL_PCB_LIB_NAME}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Frameworks/${LOCAL_PCB_LIB_NAME}"
            # Update executable to look for dylib via @rpath
            COMMAND install_name_tool -change "${LOCAL_PCB_LIB}" "@rpath/${LOCAL_PCB_LIB_NAME}" "$<TARGET_FILE:${PROJECT_NAME}>"
            COMMENT "Bundling and fixing libPCBUSB in app bundle"
        )
        
        # Set rpath so app finds the bundled dylib at runtime
        set_target_properties(${PROJECT_NAME} PROPERTIES 
            BUILD_RPATH "@executable_path/../Frameworks"
            INSTALL_RPATH "@executable_path/../Frameworks"
        )
    elseif(WIN32)
        # Windows: copy DLL to output directory
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LOCAL_PCB_LIB}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMENT "Copying PCAN DLL to output directory"
        )
    elseif(UNIX)
        # Linux: set rpath to look in executable directory
        set_target_properties(${PROJECT_NAME} PROPERTIES 
            BUILD_RPATH "$ORIGIN"
            INSTALL_RPATH "$ORIGIN"
        )
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LOCAL_PCB_LIB}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMENT "Copying PCAN library to output directory"
        )
    endif()
else()
    message(STATUS "PCAN library not found at: ${LOCAL_PCB_LIB}")
endif()

# Deployment notes: on Windows users may use windeployqt; on macOS bundle is created automatically when MACOSX_BUNDLE is used

message(STATUS "CMake configuration complete. To build: mkdir build && cd build && cmake .. && cmake --build .")

