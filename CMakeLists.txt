cmake_minimum_required(VERSION 3.17)
project(RunFleanceRun C)

set(CMAKE_C_STANDARD 99)

if( ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
        set(USE_FLAGS "-s USE_SDL=2")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${USE_FLAGS}")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${USE_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${USE_FLAGS}")
        set(CMAKE_EXECUTABLE_SUFFIX .html)
else()
        find_package(SDL2 REQUIRED)
endif()

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH} D:/Program Files (x86)/portaudio")
include_directories("D:/portaudio/build/inc")
link_directories("D:/portaudio/build/lib")

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH} D:/libopenmpt/bin")
set(CMAKE_VERBOSE_MAKEFILE 1)
include_directories("D:/libopenmpt/")
link_directories("D:/libopenmpt/bin")

include_directories(src/libs)

add_executable(RunFleanceRun src/main.c src/Sprite.c src/Sprite.h src/libs/stbttf.c src/libs/stbttf.h)

include(cmake/FileEmbed.cmake)
FileEmbedSetup()
FileEmbedAdd(${CMAKE_SOURCE_DIR}/radix-mountain_king.mod)
FileEmbedAdd(${CMAKE_SOURCE_DIR}/fleance-crying-sheet.png)
FileEmbedAdd(${CMAKE_SOURCE_DIR}/fleance-running-sheet.png)
FileEmbedAdd(${CMAKE_SOURCE_DIR}/fleance-ducking-sheet.png)
FileEmbedAdd(${CMAKE_SOURCE_DIR}/background-sheet.png)
FileEmbedAdd(${CMAKE_SOURCE_DIR}/rock.png)
FileEmbedAdd(${CMAKE_SOURCE_DIR}/bird-sheet.png)
FileEmbedAdd(${CMAKE_SOURCE_DIR}/ChronoTrigger.ttf)

if( ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
        target_link_libraries(RunFleanceRun file_embed SDL2 m pthread ${REQUIRED_LIBS})
else()
        target_link_libraries(RunFleanceRun file_embed portaudio openmpt SDL2main SDL2_ttf SDL2 m pthread ${REQUIRED_LIBS})
endif()

add_definitions(-DSDL_MAIN_HANDLED)

install(CODE [[
        message(STATUS "Looking for deps in ${CMAKE_SYSTEM_LIBRARY_PATH};${CMAKE_MINGW_SYSTEM_LIBRARY_PATH}")
        file(GET_RUNTIME_DEPENDENCIES
            RESOLVED_DEPENDENCIES_VAR deps_resolved
            UNRESOLVED_DEPENDENCIES_VAR deps_unresolved
            LIBRARIES $<TARGET_FILE:RunFleanceRun>
            DIRECTORIES ${CMAKE_SYSTEM_LIBRARY_PATH} ${CMAKE_MINGW_SYSTEM_LIBRARY_PATH}
            PRE_EXCLUDE_REGEXES "api-ms-*" "ext-ms-*"
            POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
            )
        message(STATUS "Resolving runtime dependencies for $<TARGET_FILE:RunFleanceRun>")
        foreach(dep ${deps_resolved})
            file(INSTALL ${dep} DESTINATION ${CMAKE_INSTALL_PREFIX})
        endforeach()
        foreach(dep ${deps_unresolved})
            message(WARNING "Runtime dependency ${dep} could not be resolved.")
        endforeach()
        ]])