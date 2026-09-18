option(GUIT_BUILD_UISNAP "Build the dev-only UI snapshot tool" ON)

if(GUIT_BUILD_UISNAP)
    add_executable(uisnap main.cpp)
    target_link_libraries(uisnap PRIVATE guit_core)
    target_include_directories(uisnap PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_compile_options(uisnap PRIVATE
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
    )
endif()
