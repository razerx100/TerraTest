cmake_minimum_required(VERSION 3.24)

project(TerraTest
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")

include(FetchContent)

set(GOOGLE_TEST_COMMIT_ID 12a5852e451baabc79c63a86c634912c563d57bc CACHE STRING "Supply the latest commit ID from the GitHub repository.")

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/${GOOGLE_TEST_COMMIT_ID}.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

if(MSVC)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

FetchContent_MakeAvailable(googletest)

enable_testing()

file(GLOB_RECURSE SRC includes/*.hpp src/*.cpp)

if(PLATFORM STREQUAL "WINDOWS")
    file(GLOB_RECURSE SRCOPT Win32/*.hpp Win32/*.cpp)
endif()

add_executable(${PROJECT_NAME}
    ${SRC} ${SRCOPT}
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    GTest::gtest_main
)

set(LIBRARY_TYPE STATIC)

find_package(Vulkan REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Vulkan::Vulkan
)

FetchContent_Declare(
    Terra
    GIT_REPOSITORY https://github.com/razerx100/Terra.git
    GIT_TAG ${BRANCH}
)

FetchContent_GetProperties(Terra)

message(STATUS "Fetching Terra from the branch ${BRANCH}....")

set(TERRA_DIR "../Library")

if(NOT Terra_POPULATED)
    FetchContent_Populate(Terra
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${TERRA_DIR}
    )
    add_subdirectory(${terra_SOURCE_DIR} ${terra_BINARY_DIR})
else()
    FetchContent_MakeAvailable(Terra)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${TERRA_DIR}/includes/ ${TERRA_DIR}/includes/VK/ ${TERRA_DIR}/includes/Exceptions/ ${TERRA_DIR}/templates/ 
    ${TERRA_DIR}/exports/ ${TERRA_DIR}/DirectXMath/Inc/ ${TERRA_DIR}/DirectXMath/Extensions/
)

target_compile_definitions(${PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:_DEBUG>" "$<$<CONFIG:RELEASE>:NDEBUG>")

target_link_libraries(${PROJECT_NAME} PUBLIC
    Terra
)

if(PLATFORM STREQUAL "WINDOWS")
    target_include_directories(${PROJECT_NAME} PRIVATE ${TERRA_DIR}/Win32/includes/ Win32/)
    target_compile_definitions(${PROJECT_NAME} PRIVATE TERRA_WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        dxgi.lib
    )
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /fp:fast /MP /EHa /Ot /W4 /Gy)
endif()

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
