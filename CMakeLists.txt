# Created by the script cgal_create_CMakeLists
# This is the CMake script for compiling a set of CGAL applications.

project( executable )


cmake_minimum_required(VERSION 2.8.11)

# CGAL and its components
find_package( CGAL QUIET COMPONENTS  )

if ( NOT CGAL_FOUND )

  message(STATUS "This project requires the CGAL library, and will not be compiled.")
  return()  

endif()

# include helper file
include( ${CGAL_USE_FILE} )


# Boost and its components
find_package( Boost REQUIRED )

if ( NOT Boost_FOUND )

  message(STATUS "This project requires the Boost library, and will not be compiled.")

  return()  

endif()

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})

# include for local directory

# include for local package
find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})

find_package(glfw3 REQUIRED)



# Creating entries for target: executable
# ############################

add_executable( executable  Voronoi.cpp )

add_to_cached_list( CGAL_EXECUTABLE_TARGETS executable )

# Link the executable to CGAL and third-party libraries
target_link_libraries(executable   ${CGAL_LIBRARIES} ${CGAL_3RD_PARTY_LIBRARIES} )
target_link_libraries(executable ${GLEW_LIBRARIES})
target_link_libraries(executable glfw)
target_link_libraries(executable ${OPENGL_gl_LIBRARY})