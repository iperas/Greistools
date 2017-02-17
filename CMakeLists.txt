CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

# Common settings

CMAKE_POLICY(SET CMP0003 NEW)
CMAKE_POLICY(SET CMP0054 NEW)

# Get all propreties that cmake supports
execute_process(COMMAND cmake --help-property-list OUTPUT_VARIABLE CMAKE_PROPERTY_LIST)

# Convert command output into a CMake list
STRING(REGEX REPLACE ";" "\\\\;" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
STRING(REGEX REPLACE "\n" ";" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")

function(print_properties)
    message ("CMAKE_PROPERTY_LIST = ${CMAKE_PROPERTY_LIST}")
endfunction(print_properties)

function(print_target_properties tgt)
    if(NOT TARGET ${tgt})
      message("There is no target named '${tgt}'")
      return()
    endif()

    foreach (prop ${CMAKE_PROPERTY_LIST})
        string(REPLACE "<CONFIG>" "${CMAKE_BUILD_TYPE}" prop ${prop})
        # message ("Checking ${prop}")
        get_property(propval TARGET ${tgt} PROPERTY ${prop} SET)
        if (propval)
            get_target_property(propval ${tgt} ${prop})
            message ("${tgt} ${prop} = ${propval}")
        endif()
    endforeach(prop)
endfunction(print_target_properties)


MACRO(add_custom_action_copy targets)
	foreach(File ${targets})
	  add_custom_command(TARGET ${PROJECT} PRE_BUILD
	                     COMMAND ${CMAKE_COMMAND} -E
	                         copy ${File} $<TARGET_FILE_DIR:${PROJECT}>)
	endforeach()
ENDMACRO()

MACRO(add_test_ex target)
    ADD_TEST(NAME ${target} COMMAND $<TARGET_FILE:${target}> --gtest_output=xml:
${CMAKE_BINARY_DIR}/build/test-reports/${target}.xml)
ENDMACRO()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/bin)

# Add some multithreaded build support
MARK_AS_ADVANCED(MULTITHREADED_BUILD)
set(MULTITHREADED_BUILD 12 CACHE STRING "How many threads are used to build the project")
if(MULTITHREADED_BUILD)
    if(CMAKE_COMPILER_IS_GNUCXX)
        message(STATUS ${CMAKE_BUILD_TOOL})
        set(CMAKE_MAKE_PROGRAM "${CMAKE_MAKE_PROGRAM} -j${MULTITHREADED_BUILD}")
        message(STATUS "Added arguments to CMAKE_BUILD_TOOL: ${CMAKE_MAKE_PROGRAM}")
    elseif(MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
        message(STATUS "Added parallel build arguments to CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
    endif()
endif()

# Libraries

## Qt

# (Make sure that CMAKE_PREFIX_PATH env var is set)
FIND_PACKAGE(Qt5Core)
FIND_PACKAGE(Qt5Sql)

## Boost

SET(Boost_USE_STATIC_LIBS ON)
SET(Boost_USE_MULTITHREADED ON)
FIND_PACKAGE(Boost COMPONENTS regex system thread REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
ADD_DEFINITIONS(-DBOOST_ALL_NO_LIB)

# Common

include(ExternalProject)
set_directory_properties(PROPERTIES EP_PREFIX ${CMAKE_BINARY_DIR}/ThirdParty)

ExternalProject_Add (
 libCommon
 GIT_REPOSITORY "https://github.com/iperas/Common"
 GIT_TAG "master"
 UPDATE_COMMAND ""
 INSTALL_COMMAND ""
 BUILD_IN_SOURCE 1
)
ExternalProject_Get_Property(libCommon source_dir)
SET(Common_source_dir ${source_dir})
add_library( Common STATIC IMPORTED )
if (UNIX)
    SET(pathCommon ${Common_source_dir}/build/lib/libCommon.a)
endif (UNIX)
if (WIN32)
    SET(pathCommon ${Common_source_dir}/build/lib/Common.lib)   
endif (WIN32)
set_target_properties( Common PROPERTIES IMPORTED_LOCATION ${pathCommon} )
add_dependencies( Common libCommon )

ExternalProject_Add (
 libGreis
 GIT_REPOSITORY "https://github.com/iperas/Greis"
 GIT_TAG "master"
 UPDATE_COMMAND ""
 INSTALL_COMMAND ""
 BUILD_IN_SOURCE 1
)
ExternalProject_Get_Property(libGreis source_dir)
SET(Greis_source_dir ${source_dir})
add_library( Greis STATIC IMPORTED )
if (UNIX)
    SET(pathGreis ${Greis_source_dir}/build/lib/libGreis.a)
endif (UNIX)
if (WIN32)
    SET(pathCommon ${Greis_source_dir}/build/lib/Greis.lib)
endif (WIN32)
set_target_properties( Greis PROPERTIES IMPORTED_LOCATION ${pathGreis} )
add_dependencies( Greis libGreis )

add_subdirectory(jps2db)
add_subdirectory(db2jps)
add_subdirectory(jps2jps)
add_subdirectory(jpsdump)

