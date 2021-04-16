# CMakeList.txt : projet CMake pour Dummy server, incluez la source et définissez
# la logique spécifique au projet ici.
#
project(dummyemulator)
cmake_minimum_required (VERSION 3.8)
set(CMAKE_BUILD_TYPE Debug)

# Ajoutez une source à l'exécutable de ce projet.
add_executable (dummyemulator dummy.c)

set_property(TARGET dummyemulator PROPERTY C_STANDARD 99)


if(WIN32)
  target_link_libraries(dummyemulator wsock32 ws2_32)
endif()

target_include_directories(dummyemulator PUBLIC "../../c_includes/")
target_include_directories(dummyemulator PUBLIC "../../generic poll server")

# TODO: Ajoutez des tests et installez des cibles si nécessaire.
