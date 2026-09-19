cmake_minimum_required(VERSION 3.21)

project(Guit
    VERSION 0.1.0
    DESCRIPTION "A beginner-friendly desktop Git client that shows what Git is doing"
    LANGUAGES CXX
)

option(GUIT_BUILD_TESTS "Build the Guit test suite" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# The primary Windows compiler is MinGW-w64 (MSVC is not used).
# Concurrent powers the single-threaded background queue for Git reads
# (controllers stay responsive; see docs/architecture.md).
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Concurrent)

if(GUIT_BUILD_TESTS)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    enable_testing()
endif()

add_subdirectory(src)
add_subdirectory(tools)

if(GUIT_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# --- Windows deployment -----------------------------------------------------
# Produces build/deploy/ with guit.exe plus every Qt/plugin/MinGW runtime
# file it needs:  cmake --build build --target deploy
# No absolute Qt paths: windeployqt is located from the Qt installation
# that provided the configured Qt6 packages.
if(WIN32)
    set(GUIT_DEPLOY_DIR "${CMAKE_BINARY_DIR}/deploy")

    # Prefer the qmake companion tool, fall back to the Core DLL location.
    set(GUIT_QT_BIN_DIR "")
    foreach(candidate_target Qt6::qmake Qt6::Core)
        if(TARGET ${candidate_target} AND NOT GUIT_QT_BIN_DIR)
            foreach(property IMPORTED_LOCATION IMPORTED_LOCATION_RELEASE IMPORTED_LOCATION_DEBUG)
                get_target_property(candidate_path ${candidate_target} ${property})
                if(candidate_path AND NOT candidate_path MATCHES "-NOTFOUND$")
                    get_filename_component(GUIT_QT_BIN_DIR "${candidate_path}" DIRECTORY)
                    break()
                endif()
            endforeach()
        endif()
    endforeach()
    # Qt6::Core points at the import library for MinGW; the DLLs (and
    # windeployqt) live next to it in bin/.
    if(GUIT_QT_BIN_DIR AND GUIT_QT_BIN_DIR MATCHES "/lib$")
        string(REGEX REPLACE "/lib$" "/bin" GUIT_QT_BIN_DIR "${GUIT_QT_BIN_DIR}")
    endif()
    find_program(GUIT_WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt windeployqt6
        PATHS "${GUIT_QT_BIN_DIR}"
        NO_DEFAULT_PATH
    )

    if(GUIT_WINDEPLOYQT_EXECUTABLE)
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(GUIT_WINDEPLOYQT_MODE "--debug")
        else()
            set(GUIT_WINDEPLOYQT_MODE "--release")
        endif()
        add_custom_target(deploy
            DEPENDS guit
            COMMAND "${CMAKE_COMMAND}" -E remove_directory "${GUIT_DEPLOY_DIR}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${GUIT_DEPLOY_DIR}"
            COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:guit>" "${GUIT_DEPLOY_DIR}/"
            COMMAND "${GUIT_WINDEPLOYQT_EXECUTABLE}"
                ${GUIT_WINDEPLOYQT_MODE}
                --compiler-runtime
                --dir "${GUIT_DEPLOY_DIR}"
                "$<TARGET_FILE:guit>"
            COMMENT "Deploying Guit to ${GUIT_DEPLOY_DIR} (Qt + MinGW runtime via windeployqt)"
            VERBATIM
        )
    else()
        message(WARNING "windeployqt was not found next to the configured Qt6; the 'deploy' target is unavailable.")
    endif()
endif()
