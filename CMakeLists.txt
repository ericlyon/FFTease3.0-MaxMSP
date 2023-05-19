cmake_minimum_required(VERSION 3.19)
project(FFTease)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES x86_64;arm64)  # Enable Apple silicon builds

include(${CMAKE_CURRENT_SOURCE_DIR}/source/max-sdk-base/script/max-package.cmake)
# Generate a project for every folder in the "source/projects" folder
MACRO(SUBDIRLIST result curdir)
    FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
    SET(dirlist "")
    FOREACH(child ${children})
        IF(IS_DIRECTORY ${curdir}/${child})
            LIST(APPEND dirlist ${child})
        ENDIF()
    ENDFOREACH()
    SET(${result} ${dirlist})
ENDMACRO()

SUBDIRLIST(PROJECT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/source/projects)
foreach (project_dir ${PROJECT_DIRS})
    set(project_path ${CMAKE_CURRENT_SOURCE_DIR}/source/projects/${project_dir})
    if (EXISTS "${project_path}/CMakeLists.txt")
        add_subdirectory(${project_path})
    endif ()
endforeach ()