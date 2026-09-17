add_library(guit_core STATIC
    git/GitResult.h
    git/GitResult.cpp
    git/GitProcess.h
    git/GitProcess.cpp
    git/GitLocator.h
    git/GitLocator.cpp
    git/GitVersion.h
    git/GitVersion.cpp
    git/GitClient.h
    git/GitClient.cpp
    git/GitRepository.h
    git/GitRepository.cpp
    git/FileStatus.h
    git/FileStatus.cpp
    git/GitModels.h
    git/GitModels.cpp
    git/DiffInfo.h
    git/DiffInfo.cpp
    utils/GitCommandFormatter.h
    utils/GitCommandFormatter.cpp
    utils/Logger.h
    utils/Logger.cpp
    app/AppSettings.h
    app/AppSettings.cpp
    app/ThemeManager.h
    app/ThemeManager.cpp
    controllers/RepositoryController.h
    controllers/RepositoryController.cpp
    controllers/ChangesController.h
    controllers/ChangesController.cpp
    controllers/HistoryController.h
    controllers/HistoryController.cpp
    controllers/BranchController.h
    controllers/BranchController.cpp
    ui/Sidebar.h
    ui/Sidebar.cpp
    ui/DiffViewer.h
    ui/DiffViewer.cpp
    ui/ChangesPage.h
    ui/ChangesPage.cpp
    ui/HistoryPage.h
    ui/HistoryPage.cpp
    ui/BranchesPage.h
    ui/BranchesPage.cpp
    ui/BranchDialog.h
    ui/BranchDialog.cpp
    ui/CloneDialog.h
    ui/CloneDialog.cpp
    ui/OverviewPage.h
    ui/OverviewPage.cpp
    ui/PlaceholderPage.h
    ui/PlaceholderPage.cpp
    ui/MainWindow.h
    ui/MainWindow.cpp
)

target_include_directories(guit_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(guit_core PUBLIC Qt6::Core Qt6::Widgets)

target_compile_options(guit_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
)

add_executable(guit app/main.cpp)
target_link_libraries(guit PRIVATE guit_core)
