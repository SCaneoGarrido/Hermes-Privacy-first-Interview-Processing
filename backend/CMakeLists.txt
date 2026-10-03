cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Generador de un solo config (Ninja): con CMAKE_BUILD_TYPE vacio, vcpkg
# resuelve algunas dependencias desde vcpkg_installed/.../lib (Release) y
# otras desde .../debug/lib (Debug) de forma inconsistente entre paquetes -
# se confirmo en la practica que esto mezcla ggml-cpu.a (Debug) con
# libwhisper.a/ggml-base.a (Release), un ABI incompatible que causa un
# crash silencioso (access violation 0xC0000005) dentro de whisper_full(),
# sin ninguna excepcion de C++ capturable. Fijar Release explicitamente
# evita la ambiguedad.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()
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

project(backend VERSION 0.1.0)

find_package(Crow CONFIG REQUIRED)
find_package(unofficial-libmariadb CONFIG REQUIRED)
find_package(whisper CONFIG REQUIRED)
# El puerto de vcpkg expone FFmpeg via el modulo clasico FindFFmpeg.cmake
# (variables FFMPEG_LIBRARIES/FFMPEG_INCLUDE_DIRS), no targets con namespace
# como FFmpeg::avcodec.
find_package(FFMPEG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)
# cpp-httplib habilita CPPHTTPLIB_USE_NON_BLOCKING_GETADDRINFO por defecto en
# todas las plataformas, pero usa GetAddrInfoExCancel, que este toolchain de
# MinGW no declara (falla la compilacion). No hace falta resolucion DNS
# asincrona/cancelable para hablarle a Ollama en localhost - se remueve la
# definicion del target antes de linkearlo.
get_target_property(_httplib_defs httplib::httplib INTERFACE_COMPILE_DEFINITIONS)
if(_httplib_defs)
    list(REMOVE_ITEM _httplib_defs "$<$<BOOL:ON>:CPPHTTPLIB_USE_NON_BLOCKING_GETADDRINFO>")
    set_target_properties(httplib::httplib PROPERTIES INTERFACE_COMPILE_DEFINITIONS "${_httplib_defs}")
endif()

add_executable(backend
                main.cpp
                api/controllers/fileController.cpp
                api/controllers/healthController.cpp
                api/controllers/interviewController.cpp
                api/services/InterviewService.cpp
                api/services/TranscriptDocumentBuilder.cpp
                api/shared/utils/logger.cpp
                api/shared/database/DatabaseManager.cpp
                api/shared/database/MySqlInterviewRepository.cpp
                api/helpers/fileController.helper.cpp
                api/helpers/interviewController.helper.cpp
                jobs/src/JobQueue.cpp
                jobs/src/WorkerPool.cpp
                jobs/src/InterviewJobRepository.cpp
                jobs/src/InterviewProcessingJobHandler.cpp
                audio/src/FfmpegAudioNormalizer.cpp
                transcription/src/WhisperTranscriber.cpp
                llm/src/OllamaClient.cpp
                llm/src/TranscriptEnhancer.cpp
                llm/src/GlossarySanitizer.cpp
                llm/src/TextUtils.cpp
            )

target_compile_features(backend PRIVATE cxx_std_20)
set_target_properties(backend PROPERTIES CMAKE_CXX_EXTENSIONS OFF)

target_include_directories(backend PRIVATE ${FFMPEG_INCLUDE_DIRS})

# cpp-httplib rechaza compilar sin esto (asume Windows 8 o anterior si no
# esta definido) - ver ADR-015.
target_compile_definitions(backend PRIVATE _WIN32_WINNT=0x0A00)

target_link_libraries(backend PRIVATE
    Crow::Crow
    unofficial::libmariadb
    whisper
    ${FFMPEG_LIBRARIES}
    nlohmann_json::nlohmann_json
    httplib::httplib
)

if(WIN32)
    # Asio (usado por Crow) necesita las librerias de sockets de Windows;
    # con MSVC se enlazan via #pragma comment, pero MinGW no lo hace solo.
    target_link_libraries(backend PRIVATE ws2_32 mswsock)
endif()
