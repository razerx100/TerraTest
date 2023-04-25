cmake_minimum_required(VERSION 3.24)

file(
    GLOB_RECURSE SH_SRC
     src/*.frag src/*.vert src/*.comp src/*.mesh
)

find_program(
    GLSLC
     glslc
    HINTS
     /usr/bin
     /usr/local/bin
     $ENV{VULKAN_SDK}/Bin/
)

set(SHADER_PATH resources/shaders)

foreach(SHADER ${SH_SRC})
    get_filename_component(FILE_NAME ${SHADER} NAME_WLE)
    set(SPIRV "${CMAKE_BINARY_DIR}/${SHADER_PATH}/${FILE_NAME}.spv")
    list(APPEND SPIRV_BINARY ${SPIRV})
endforeach(SHADER)

add_custom_target(
    GLSL
    ALL
    DEPENDS
     ${SPIRV_BINARY}
    SOURCES
     ${SH_SRC}
)

foreach(SHADER ${SH_SRC})
    message(STATUS "Adding shader...")
    get_filename_component(FILE_NAME ${SHADER} NAME_WLE)
    set(SPIRV "${CMAKE_BINARY_DIR}/${SHADER_PATH}/${FILE_NAME}.spv")
    message(STATUS ${SHADER})

    add_custom_command(
        TARGET
         GLSL
        POST_BUILD
        COMMAND ${CMAKE_COMMAND}
         -E make_directory ${CMAKE_BINARY_DIR}/${SHADER_PATH}
        COMMAND
         ${GLSLC} ${SHADER} -o ${SPIRV} --target-spv=spv1.4
        COMMAND ${CMAKE_COMMAND}
         -E copy ${SPIRV} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/${SHADER_PATH}/${FILE_NAME}.spv
        DEPENDS
         ${SHADER}
    )
endforeach(SHADER)

source_group("Source Files" FILES ${SH_SRC})


