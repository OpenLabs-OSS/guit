function(add_guit_test target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE guit_core Qt6::Test Qt6::Core)
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src)
    add_test(NAME ${target} COMMAND ${target})
    set_tests_properties(${target} PROPERTIES TIMEOUT 120)
    # On Windows the Qt and MinGW runtime DLLs must be findable when CTest
    # launches the test executables. Prepend their directories to PATH.
    # NOTE: "\;" is an escaped semicolon: it reaches the test environment
    # as a literal PATH separator instead of a CMake list separator. The
    # same escaping is applied to the inherited PATH for the same reason.
    get_filename_component(compilerBinDir "${CMAKE_CXX_COMPILER}" DIRECTORY)
    set(parentPath "$ENV{PATH}")
    string(REPLACE ";" "\\;" parentPath "${parentPath}")
    set_tests_properties(${target} PROPERTIES
        ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>\;${compilerBinDir}\;${parentPath}")
endfunction()

add_guit_test(tst_gitprocess tst_gitprocess.cpp)
add_guit_test(tst_gitversion tst_gitversion.cpp)
add_guit_test(tst_gitclient tst_gitclient.cpp)
add_guit_test(tst_models tst_models.cpp)
add_guit_test(tst_gitrepository tst_gitrepository.cpp)
add_guit_test(tst_commandformatter tst_commandformatter.cpp)
add_guit_test(tst_diff tst_diff.cpp)
add_guit_test(tst_changes tst_changes.cpp)
add_guit_test(tst_history tst_history.cpp)
add_guit_test(tst_branches tst_branches.cpp)
add_guit_test(tst_remotes tst_remotes.cpp)
add_guit_test(tst_tags tst_tags.cpp)
add_guit_test(tst_stash tst_stash.cpp)
add_guit_test(tst_merge tst_merge.cpp)
add_guit_test(tst_rebase tst_rebase.cpp)
add_guit_test(tst_reset tst_reset.cpp)
add_guit_test(tst_graph tst_graph.cpp)
add_guit_test(tst_insight tst_insight.cpp)
add_guit_test(tst_appsettings tst_appsettings.cpp)
add_guit_test(tst_theme tst_theme.cpp)
