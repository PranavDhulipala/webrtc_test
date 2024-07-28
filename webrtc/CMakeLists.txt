cmake_minimum_required(VERSION 3.21.0)
project(webrtc C CXX)

# Detect system architecture
if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "amd64")
    set(CMAKE_BUILD_ARCHITECTURE "amd64")
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    set(CMAKE_BUILD_ARCHITECTURE "arm64")
else()
    message(WARNING "Unknown system processor: ${CMAKE_SYSTEM_PROCESSOR}")
    set(CMAKE_BUILD_ARCHITECTURE "unknown")
endif()

# Verify build architecture and set appropriate variables
if (CMAKE_BUILD_ARCHITECTURE MATCHES "arm64|amd64")
    set(CMAKE_SYSTEM_NAME Linux)
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

    if (CMAKE_BUILD_ARCHITECTURE MATCHES "arm64")
        set(CMAKE_SYSTEM_PROCESSOR arm64)
        set(CMAKE_SYSROOT ${CMAKE_CURRENT_SOURCE_DIR}/webrtc/src/build/linux/debian_bullseye_arm64-sysroot)
        set(CMAKE_C_COMPILER /usr/bin/aarch64-linux-gnu-gcc)
        set(CMAKE_CXX_COMPILER /usr/bin/aarch64-linux-gnu-g++)
    else()
        set(CMAKE_SYSTEM_PROCESSOR amd64)
    endif()
endif()

# Set default build type
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Set CMake policies
if (POLICY CMP0014)
    cmake_policy(SET CMP0014 NEW)
endif()

if (POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Define build-specific flags
if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DDEBUG=1)
elseif (CMAKE_BUILD_TYPE MATCHES Release)
    add_definitions(-DRELEASE=1)
endif()

set(CMAKE_CXX_FLAGS "-fPIC")
add_definitions(-DWEBRTC_POSIX)

# Set depot_tools directory
set(DEPOT_TOOLS_DIR "${CMAKE_CURRENT_LIST_DIR}/depot_tools")

# Clone depot_tools if not already present
if(NOT EXISTS "${DEPOT_TOOLS_DIR}/.git")
    message(STATUS "Cloning depot_tools...")
    execute_process(
        COMMAND git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git "${DEPOT_TOOLS_DIR}"
        RESULT_VARIABLE result
    )
    if(NOT result EQUAL "0")
        message(FATAL_ERROR "Failed to clone depot_tools")
    endif()
endif()

# Update PATH to include depot_tools
set(ENV{PATH} "$ENV{PATH}:${DEPOT_TOOLS_DIR}")

# Ensure the webrtc directory exists
if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/webrtc")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/webrtc")
endif()

# Fetch WebRTC source if not already present
if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/webrtc/src")
    execute_process(
        COMMAND fetch --nohooks webrtc
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/webrtc"
        RESULT_VARIABLE fetch_webrtc_result
    )
    if(NOT ${fetch_webrtc_result} EQUAL 0)
        message(FATAL_ERROR "\"fetch --nohooks webrtc\" failed (${fetch_webrtc_result})")
    endif()

    execute_process(
        COMMAND git fetch
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/webrtc/src"
        RESULT_VARIABLE git_fetch_webrtc_result
    )
    if(NOT ${git_fetch_webrtc_result} EQUAL 0)
        message(FATAL_ERROR "\"git fetch\" failed (${git_fetch_webrtc_result})")
    endif()

    execute_process(
        COMMAND git checkout -f main
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/webrtc/src"
        RESULT_VARIABLE git_checkout_webrtc_result
    )
    if(NOT ${git_checkout_webrtc_result} EQUAL 0)
        message(FATAL_ERROR "\"git checkout\" failed (${git_checkout_webrtc_result})")
    endif()

    execute_process(
        COMMAND gclient sync -D
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/webrtc/src"
        RESULT_VARIABLE gclient_sync_webrtc_result
    )
    if(NOT ${gclient_sync_webrtc_result} EQUAL 0)
        message(FATAL_ERROR "\"gclient sync -D\" failed (${gclient_sync_webrtc_result})")
    endif()
endif()

# Function to install sysroot
function(install_sysroot arch)
    execute_process(
        COMMAND python3 install-sysroot.py --arch=${arch}
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/webrtc/src/build/linux/sysroot_scripts"
        RESULT_VARIABLE build_sysroot_architecture
    )
    if(NOT build_sysroot_architecture EQUAL 0)
        message(FATAL_ERROR "\"python3 install-sysroot.py --arch=${arch}\" failed (${build_sysroot_architecture})")
    endif()
endfunction()

# Install sysroot based on architecture
install_sysroot(${CMAKE_BUILD_ARCHITECTURE})

set(ENV{LD_LIBRARY_PATH} "${CMAKE_CURRENT_LIST_DIR}/webrtc/src/lib/x86_64-linux-gnu:$ENV{LD_LIBRARY_PATH}")

# Set architecture-specific variables
if(${CMAKE_BUILD_ARCHITECTURE} MATCHES "amd64")
    set(A "x64")
    set(EXTRA_FLAGS "is_clang=false target_os=\"linux\"")
elseif(${CMAKE_BUILD_ARCHITECTURE} MATCHES "arm64")
    set(A "arm64")
    set(EXTRA_FLAGS "is_clang=false use_sysroot=true sysroot=\"${CMAKE_SYSROOT}\" target_os=\"linux\" arm_control_flow_integrity=\"none\"")
else()
    message(WARNING "Unknown target platform: ${CMAKE_BUILD_ARCHITECTURE}, continuing anyway")
endif()

set(COMMON_FLAGS "is_component_build=false treat_warnings_as_errors=false fatal_linker_warnings=false use_gio=false use_rtti=true use_custom_libcxx=false use_custom_libcxx_for_host=false rtc_enable_protobuf=false rtc_include_tests=false rtc_use_h264=true proprietary_codecs=true ffmpeg_branding=\"Chrome\" target_cpu=\"${A}\" enable_iterator_debugging=false rtc_build_examples=true rtc_use_pipewire=false")

# Function to generate ninja files
function(generate_ninja_files build_type)
    if(${build_type} MATCHES "Debug")
        set(is_debug true)
    else()
        set(is_debug false)
    endif()
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/webrtc/src/out/${build_type}")
    execute_process(COMMAND gn gen out/${build_type} "--args=is_debug=${is_debug} ${COMMON_FLAGS} ${EXTRA_FLAGS}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/webrtc/src"
        RESULT_VARIABLE generate_ninja_result
    )
    if(NOT ${generate_ninja_result} EQUAL 0)
        message(FATAL_ERROR "\"generate ninja ${build_type} files\" failed (${generate_ninja_result})")
    endif()
endfunction()

generate_ninja_files("Release")

# List the arguments for the build
execute_process(COMMAND gn args --list ${CMAKE_CURRENT_LIST_DIR}/webrtc/src/out/${CMAKE_BUILD_TYPE}
    WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/webrtc/src"
    RESULT_VARIABLE list_debug_args_result
)
if(NOT ${list_debug_args_result} EQUAL 0)
    message(FATAL_ERROR "\"gn args --list\" failed (${list_debug_args_result})")
endif()

# Build WebRTC project
execute_process(COMMAND ninja -C ${CMAKE_CURRENT_LIST_DIR}/webrtc/src/out/${CMAKE_BUILD_TYPE} webrtc stunserver turnserver
    WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/webrtc/src"
    RESULT_VARIABLE build_webrtc_result
)
if(NOT ${build_webrtc_result} EQUAL 0)
    message(FATAL_ERROR "\"build webrtc project\" failed (${build_webrtc_result})")
endif()

# Specify the include directories and libraries
list(APPEND boringssl_INCLUDE "${CMAKE_CURRENT_LIST_DIR}/webrtc/src/third_party/boringssl/src/include")
list(APPEND webrtc_LIBRARY "${CMAKE_CURRENT_LIST_DIR}/webrtc/src/out/${CMAKE_BUILD_TYPE}/obj/libwebrtc.a" dl X11 expat)
list(APPEND boringssl_LIBRARY "${CMAKE_CURRENT_LIST_DIR}/webrtc/src/out/${CMAKE_BUILD_TYPE}/obj/third_party/boringssl/libboringssl.a")
list(APPEND libyuv_LIBRARY "${CMAKE_CURRENT_LIST_DIR}/webrtc/src/out/${CMAKE_BUILD_TYPE}/obj/third_party/libyuv/libyuv_internal.a")

# Manual installation of all dependencies.
list(GET webrtc_LIBRARY 0 webrtc_LIBRARY_only)
install(FILES ${webrtc_LIBRARY_only} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${PROJECT_NAME})
install(FILES ${boringssl_LIBRARY} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${PROJECT_NAME})
install(FILES ${libyuv_LIBRARY} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${PROJECT_NAME})

# Function to install directories only if they contain .a files
function(install_if_not_empty src_dir dest_dir)
    file(GLOB files "${src_dir}/*.a")
    if(files)
        install(DIRECTORY ${src_dir} DESTINATION ${dest_dir}
            FILES_MATCHING PATTERN "*.a")
    endif()
endfunction()

# Install only .a files from the specified directories
install_if_not_empty("${CMAKE_CURRENT_LIST_DIR}/webrtc/src/out/${CMAKE_BUILD_TYPE}/obj/third_party/boringssl/boringssl" "${CMAKE_INSTALL_PREFIX}/lib/${PROJECT_NAME}")
install_if_not_empty("${CMAKE_CURRENT_LIST_DIR}/webrtc/src/out/${CMAKE_BUILD_TYPE}/obj/third_party/boringssl/boringssl_asm" "${CMAKE_INSTALL_PREFIX}/lib/${PROJECT_NAME}")
install_if_not_empty("${CMAKE_CURRENT_LIST_DIR}/webrtc/src/out/${CMAKE_BUILD_TYPE}/obj/third_party/libyuv/libyuv_internal" "${CMAKE_INSTALL_PREFIX}/lib/${PROJECT_NAME}")

# Install the webrtc package.xml file
install(FILES "package.xml" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/webrtc")

# Install all includes except from third_party
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/webrtc/src/ DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}
    FILES_MATCHING PATTERN "*.h"
    PATTERN webrtc/src/third_party EXCLUDE)

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/webrtc/src/third_party/abseil-cpp/absl DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}
    FILES_MATCHING PATTERN "*.h")

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/webrtc/src/third_party/boringssl/src/include/openssl DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}
    FILES_MATCHING PATTERN "*.h")

install(FILES ${CMAKE_CURRENT_LIST_DIR}/webrtc/src/third_party/libyuv/include/libyuv.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME})
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/webrtc/src/third_party/libyuv/include/libyuv DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}
    FILES_MATCHING PATTERN "*.h")

# Install stunserver and turnserver executables
install(PROGRAMS ${CMAKE_CURRENT_LIST_DIR}/webrtc/src/out/${CMAKE_BUILD_TYPE}/stunserver DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(PROGRAMS ${CMAKE_CURRENT_LIST_DIR}/webrtc/src/out/${CMAKE_BUILD_TYPE}/turnserver DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

# Read the webrtc.ninja file
file(READ "${CMAKE_CURRENT_LIST_DIR}/webrtc/src/out/${CMAKE_BUILD_TYPE}/obj/webrtc.ninja" ninja_content)

# Extract variables from the ninja file
string(REGEX MATCHALL "([a-zA-Z0-9_]+) = ([^\n]+)" ninja_vars "${ninja_content}")

foreach(var ${ninja_vars})
    string(REGEX REPLACE "([a-zA-Z0-9_]+) = (.*)" "\\1" var_name ${var})
    string(REGEX REPLACE "([a-zA-Z0-9_]+) = (.*)" "\\2" var_value ${var})
    set(${var_name} ${var_value})
endforeach()

# Process definitions
string(REGEX MATCHALL "-D[^ ]+" webrtc_defs "${defines}")

# Check for pthread definition and append to libraries if present
if("-DHAVE_PTHREAD" IN_LIST webrtc_defs)
    list(APPEND webrtc_libs "pthread")
endif()

# Generate webrtcConfig.cmake
file(WRITE "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/cmake/webrtcConfig.cmake" "
set(webrtc_INSTALL_PREFIX \"${CMAKE_INSTALL_PREFIX}\")
set(webrtc_INCLUDE_DIRS \"\${webrtc_INSTALL_PREFIX}/include\;\${webrtc_INSTALL_PREFIX}/include/webrtc\")
set(webrtc_DEFINITIONS \"${webrtc_defs}\")
set(webrtc_LIBRARIES \"webrtc\")
add_library(webrtc STATIC IMPORTED)
set_property(TARGET webrtc PROPERTY IMPORTED_LOCATION \"\${webrtc_INSTALL_PREFIX}/lib/${PROJECT_NAME}/libwebrtc.a\")
")