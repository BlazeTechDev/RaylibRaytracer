# CMakeList.txt : CMake project for RaylibRaytracer, include source and define
# project specific logic here.
#
file(GLOB_RECURSE src CONFIGURE_DEPENDS "*.cpp" "*.h")

add_executable (RaylibRaytracer ${src})

target_link_libraries(RaylibRaytracer raylib)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET RaylibRaytracer PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
