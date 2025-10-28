# Makefile for ssh-mounter
# Simple, educational Makefile with Qt version detection

# Force Qt5 if desired - uncomment the line below:
PREFER_QT5 = 1

# Detect Qt version
ifdef PREFER_QT5
    QMAKE := $(shell which qmake 2>/dev/null)
    MOC := $(shell which moc 2>/dev/null)
    QT_VERSION := 5
else
    # Prefer Qt6
    QMAKE6 := $(shell which qmake6 2>/dev/null || which qmake-qt6 2>/dev/null)
    QMAKE5 := $(shell which qmake 2>/dev/null)
    
    ifneq ($(QMAKE6),)
        QMAKE := $(QMAKE6)
        MOC := $(shell which moc6 2>/dev/null || which moc-qt6 2>/dev/null)
        QT_VERSION := 6
    else ifneq ($(QMAKE5),)
        QMAKE := $(QMAKE5)
        MOC := $(shell which moc 2>/dev/null)
        # Check if it's actually Qt6
        QT_VER := $(shell $(QMAKE) -v | grep -o "Qt version [0-9]" | cut -d' ' -f3)
        ifeq ($(QT_VER),6)
            QT_VERSION := 6
        else
            QT_VERSION := 5
        endif
    else
        $(error Qt not found. Install qt6-base-dev or qt5-base-dev)
    endif
endif

# Get Qt paths from qmake
QT_INCLUDE := $(shell $(QMAKE) -query QT_INSTALL_HEADERS)
QT_LIBS := $(shell $(QMAKE) -query QT_INSTALL_LIBS)

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -fPIC -Wall -Wextra -g
INCLUDES = -Isrc \
           -I$(QT_INCLUDE) \
           -I$(QT_INCLUDE)/QtCore \
           -I$(QT_INCLUDE)/QtGui \
           -I$(QT_INCLUDE)/QtWidgets

LDFLAGS = -L$(QT_LIBS) -g

# Link with correct Qt version
ifeq ($(QT_VERSION),6)
    QT_LIBS_LINK = -lQt6Core -lQt6Gui -lQt6Widgets
else
    QT_LIBS_LINK = -lQt5Core -lQt5Gui -lQt5Widgets
endif

# Source files
SOURCES = src/ssh_store.cpp src/ssh_mounter.cpp src/main.cpp
HEADERS = src/ssh_store.hpp src/ssh_mounter.hpp src/console.hpp

# Object files (in build directory)
OBJECTS = build/ssh_store.o build/ssh_mounter.o build/main.o

# Moc-generated files (in src directory so #include "main.moc" works)
MOC_FILES = src/main.moc

# Output binary
TARGET = build/ssh-mounter

# Phony targets
.PHONY: all clean rebuild run info help

# Default target
all: $(TARGET)

# Help target
help:
	@echo "SSH Mounter Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  make          - Build the project (default)"
	@echo "  make clean    - Remove all build files"
	@echo "  make rebuild  - Clean and build"
	@echo "  make run      - Build and run the program"
	@echo "  make info     - Show build configuration"
	@echo "  make help     - Show this help message"

# Create build directory
build:
	@mkdir -p build

# Run moc on files with Q_OBJECT
# IMPORTANT: Clean old moc files to avoid version mismatches
# generate moc for main.cpp (you already have something like this)
src/main.moc: src/main.cpp src/ssh_store.hpp src/ssh_mounter.hpp
	@echo "[MOC] Generating main.moc (Qt$(QT_VERSION))..."
	@rm -f src/main.moc
	$(MOC) $(INCLUDES) src/main.cpp -o src/main.moc

# generate moc for ssh_store and ssh_mounter headers
src/ssh_store.moc: src/ssh_store.hpp
	@echo "[MOC] Generating ssh_store.moc..."
	$(MOC) $(INCLUDES) src/ssh_store.hpp -o src/ssh_store.moc

src/ssh_mounter.moc: src/ssh_mounter.hpp
	@echo "[MOC] Generating ssh_mounter.moc..."
	$(MOC) $(INCLUDES) src/ssh_mounter.hpp -o src/ssh_mounter.moc

# Compile object files
build/ssh_store.o: src/ssh_store.cpp src/ssh_store.hpp | build
	@echo "[CXX] Compiling ssh_store.cpp..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c src/ssh_store.cpp -o build/ssh_store.o

build/ssh_mounter.o: src/ssh_mounter.cpp src/ssh_mounter.hpp src/console.hpp | build
	@echo "[CXX] Compiling ssh_mounter.cpp..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c src/ssh_mounter.cpp -o build/ssh_mounter.o

build/main.o: src/main.cpp src/console.hpp | build
	@echo "[CXX] Compiling main.cpp..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c src/main.cpp -o build/main.o


# Link the final binary
$(TARGET): $(OBJECTS)
	@echo "[LD]  Linking ssh-mounter..."
	$(CXX) $(OBJECTS) $(LDFLAGS) $(QT_LIBS_LINK) -o $(TARGET)
	@echo ""
	@echo "✓ Build complete! (Qt$(QT_VERSION))"
	@echo "  Run with: ./$(TARGET)"
	@echo ""

clean:
	@echo "Cleaning build artifacts..."
	@rm -rf build
	@rm -f src/main.moc src/ssh_store.moc src/ssh_mounter.moc
	@echo "✓ Clean complete"


# Rebuild everything
rebuild: clean all

# Run the program
run: $(TARGET)
	@echo "Running ssh-mounter..."
	@./$(TARGET)

# Show configuration
info:
	@echo "Build Configuration:"
	@echo "  Qt Version: $(QT_VERSION)"
	@echo "  QMAKE:      $(QMAKE)"
	@echo "  MOC:        $(MOC)"
	@echo "  QT_INCLUDE: $(QT_INCLUDE)"
	@echo "  QT_LIBS:    $(QT_LIBS)"
	@echo "  CXX:        $(CXX)"
	@echo "  CXXFLAGS:   $(CXXFLAGS)"
	@echo "  LDFLAGS:    $(LDFLAGS) $(QT_LIBS_LINK)"