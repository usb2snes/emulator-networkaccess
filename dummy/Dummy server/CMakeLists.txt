# CMakeList.txt : projet CMake pour Dummy server, incluez la source et définissez
# la logique spécifique au projet ici.
#
project(dummyserver)
cmake_minimum_required (VERSION 3.8)
set(CMAKE_BUILD_TYPE Debug)

# Ajoutez une source à l'exécutable de ce projet.
add_executable (dummyserver dummy.c)

set_property(TARGET dummyserver PROPERTY C_STANDARD 99)
add_compile_definitions("DEFAULT_ROM=\"usb2snes-testlorom.sfc\"")
add_compile_definitions("DEFAULT_STATE=\"usb2snes-testlorom\"")


if(WIN32)
  target_link_libraries(dummyserver wsock32 ws2_32)
endif()

target_include_directories(dummyserver PUBLIC "../../c_includes/")
target_include_directories(dummyserver PUBLIC "../../generic poll server")

# TODO: Ajoutez des tests et installez des cibles si nécessaire.
