# Contributing to Guit

Thank you for your interest in contributing to Guit! This document outlines the guidelines for contributing to the project.

## Code of Conduct

Please read and follow our [Code of Conduct](CODE_OF_CONDUCT.md).

## Getting Started

### Prerequisites
- CMake 3.21+
- Ninja
- MinGW-w64 (GCC 13+)
- Qt 6.5+ (MinGW build)
- Git for Windows
- C++20 compatible compiler

### Development Setup

```bash
# Clone the repository
git clone https://github.com/OpenLabs-OSS/guit.git
cd guit

# Configure
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build

# Run tests
ctest --test-dir build --output-on-failure
```

### Running the Application
```bash
# From build directory
./build/src/guit.exe

# Or with a repository path
./build/src/guit.exe /path/to/repo
```

## Development Workflow

### Branching Strategy
- `main` - stable releases
- Feature branches from `main`
- Bugfix branches from `main` or release branches

### Commit Messages
Follow conventional commits:
```
feat(scope): brief description
fix(scope): brief description
docs(scope): brief description
refactor(scope): brief description
test(scope): brief description
chore(scope): brief description
```

Examples:
```
feat(changes): add staging area badge colors
fix(history): handle missing parent in graph
docs(readme): update build instructions
```

### Pull Requests
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Run tests locally: `ctest --test-dir build --output-on-failure`
5. Submit PR with clear description

## Code Style

### C++
- C++20
- Modern C++ (RAII, smart pointers, `std::optional`, `std::variant`)
- RAII for resource management
- Const correctness
- No raw owning pointers
- Small, focused classes
- Clear ownership semantics

### Qt
- Use Qt's parent-child ownership
- Signals/slots for communication
- Model/View for lists
- No business logic in widgets

### Formatting
- 4 spaces indentation
- No tabs
- Braces on same line (Allman style for functions, K&R for control)
- 100 column limit

### Naming
- Classes: `PascalCase` (`GitRepository`, `ChangesController`)
- Functions/methods: `camelCase` (`refreshStatus`, `stagePaths`)
- Variables: `camelCase` (`m_repository`, `currentStatus`)
- Constants: `SCREAMING_SNAKE_CASE` (`MAX_RECENT_REPOS`)
- Enums: `PascalCase` with `EnumClass` suffix (`PendingOperation`, `ResetMode`)

## Testing

### Running Tests
```bash
# All tests
ctest --test-dir build --output-on-failure

# Specific test
ctest --test-dir build -R tst_gitclient --output-on-failure
```

### Writing Tests
- Use Qt Test framework
- Test file naming: `tst_<feature>.cpp`
- Use `TestTempRepo` for isolated Git repositories
- Test both success and failure paths
- Never use real repositories

```cpp
class TestFeature : public QObject {
    Q_OBJECT
private slots:
    void testFeature() {
        GuitTest::TempRepo repo;
        repo.writeFile("file.txt", "content");
        repo.commit("Initial");
        
        GitRepository repository;
        QVERIFY(repository.open(repo.path()));
        // ... test assertions
    }
};
```

## Architecture

Guit follows a layered architecture:
```
UI (Widgets)
    ↓
Controllers (AsyncController)
    ↓
GitRepository (Repository operations)
    ↓
GitClient (Clean C++ API over Git)
    ↓
GitProcess (QProcess wrapper)
    ↓
git.exe
```

Key principles:
- UI never calls Git directly
- Git logic testable without UI
- Async operations via `AsyncController` base class
- Generation-based cancellation for rapid refreshes

## Reporting Issues

Use GitHub Issues with:
- Clear title
- Steps to reproduce
- Expected vs actual behavior
- Screenshots if UI-related
- Git version (`git --version`)
- OS and Qt version

## Security

See [SECURITY.md](SECURITY.md) for reporting vulnerabilities.

## License

By contributing, you agree that your contributions will be licensed under the MIT License.