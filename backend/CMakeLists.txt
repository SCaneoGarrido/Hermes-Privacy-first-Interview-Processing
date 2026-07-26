cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# IMPORTANTE: Esto debe ir ANTES de la función project()
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "" FORCE)
endif()

# No hay Visual Studio instalado en esta maquina, asi que vcpkg no puede
# compilar los puertos con el triplet por defecto (x64-windows, requiere MSVC).
# Usamos el triplet mingw para compilar con el MinGW-w64 ya instalado.
if(NOT DEFINED VCPKG_TARGET_TRIPLET)
    set(VCPKG_TARGET_TRIPLET "x64-mingw-static" CACHE STRING "")
endif()
if(NOT DEFINED VCPKG_HOST_TRIPLET)
    set(VCPKG_HOST_TRIPLET "x64-mingw-static" CACHE STRING "")
endif()

project(backend)

find_package(Crow CONFIG REQUIRED)
add_executable(backend
                main.cpp
                api/controllers/fileController.cpp
                api/controllers/healthController.cpp
                api/shared/utils/logger.cpp
                api/helpers/fileController.helper.cpp
            )

target_compile_features(backend PRIVATE cxx_std_20)
set_target_properties(backend PROPERTIES CMAKE_CXX_EXTENSIONS OFF)

target_link_libraries(backend PRIVATE Crow::Crow)

if(WIN32)
    # Asio (usado por Crow) necesita las librerias de sockets de Windows;
    # con MSVC se enlazan via #pragma comment, pero MinGW no lo hace solo.
    target_link_libraries(backend PRIVATE ws2_32 mswsock)
endif()