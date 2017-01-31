CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

# variables declaration

SET(PROJECT jps2jps)

SET(DEPENDS_ON_PROJECTS Greis Common)


FILE(GLOB_RECURSE HEADERS "include/*.h")

FILE(GLOB_RECURSE SOURCES "src/*.cpp")

FILE(GLOB_RECURSE MOC_SOURCES "*_automoc.cpp")

# source grouping

source_group ("Header Files" FILES ${HEADERS})
source_group ("Source Files" FILES ${SOURCES})
source_group ("Generated Files" FILES ${MOC_SOURCES})

# project definition

PROJECT(${PROJECT} CXX)

# includes

include_directories("${Common_source_dir}")
include_directories("${Greis_source_dir}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT}")

# linking

add_executable(${PROJECT} ${HEADERS} ${SOURCES} ${MOC_SOURCES})
set_property(TARGET ${PROJECT} PROPERTY CXX_STANDARD 14)
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES LINKER_LANGUAGE CXX)
add_dependencies(Greis Common)

target_link_libraries(
    ${PROJECT}
    ${DEPENDS_ON_PROJECTS}
    ${Boost_LIBRARIES}
    Qt5::Core
    Qt5::Sql)

# dependencies

add_dependencies(${PROJECT} ${DEPENDS_ON_PROJECTS})
