# AI Instructions for SSH Mounter

This document provides essential context for AI coding agents working with the SSH Mounter codebase.

## Project Overview

SSH Mounter is a Qt-based GUI application that simplifies SSHFS mount management. It provides a user-friendly interface for mounting remote filesystems without requiring manual SSHFS commands.

### Key Components

- `SSHMounter` (src/ssh_mounter.hpp): Core class managing mount operations
  - Handles SSHFS process execution
  - Manages mount states and error handling
  - Provides password/key authentication handling

- `SSHStore` (src/ssh_store.hpp): Configuration storage
  - Manages saved SSH host configurations
  - Handles JSON serialization/deserialization
  - Provides CRUD operations for host entries

### Architecture Patterns

1. **Qt Integration**
   - Uses Qt's signal/slot mechanism for async operations
   - Inherits from QObject for event handling
   - Employs Qt's process management via QProcess

2. **State Management**
   - Mount states tracked via `MountState` enum
   - Asynchronous state transitions with signals
   - Error handling with detailed message propagation

## Development Workflow

### Build System

The project uses a custom Makefile with Qt version detection:

```bash
make        # Build the project
make clean  # Clean build artifacts
make run    # Build and run
make info   # Show build configuration
```

Key build requirements:
- Qt5 or Qt6 development files (qt5-base-dev or qt6-base-dev)
- GNU Make
- g++ with C++17 support
- FUSE and SSHFS installed for runtime

### Code Generation

The build system automatically handles Qt MOC (Meta-Object Compiler) generation:
- MOC files are generated for Qt classes with the Q_OBJECT macro
- Generated files are placed in src/ with .moc extension

## Integration Points

1. **SSHFS Integration**
   - System calls are made via QProcess
   - Mount points are verified before operations
   - Host key verification is handled automatically

2. **Filesystem Integration**
   - FUSE mount point management
   - Local directory permissions checking
   - Configuration storage in user's home directory

## Common Tasks

When modifying the codebase:

1. **Adding New Mount Options**
   - Extend the `SSHHost` struct in `ssh_store.hpp`
   - Update JSON serialization methods
   - Add corresponding UI controls in `main.cpp`

2. **Error Handling**
   - Use signal/slot mechanism for async errors
   - Emit detailed error messages via `error()` signals
   - Check process exit codes in `onProcessFinished()`

3. **State Management**
   - Update `MountState` enum for new states
   - Use `setState()` to trigger state changes
   - Connect to `stateChanged` signal for UI updates

## Project Conventions

1. **Signal/Slot Naming**
   - Signals use past tense (e.g., `mounted`, `error`)
   - Slots use command form (e.g., `mount`, `unmount`)
   - Progress signals include descriptive messages

2. **Error Handling**
   - All user-facing errors are emitted as signals
   - System-level errors include detailed context
   - State is reset to `Idle` after errors

3. **Resource Management**
   - RAII pattern for resource handling
   - QObject parent-child relationship for memory management
   - Explicit cleanup in destructors