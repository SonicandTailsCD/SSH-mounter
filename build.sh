#!/bin/bash
# build.sh - Manual build script for ssh-mounter

set -e  # Exit on error

echo "=== Building SSH Mounter ==="

# Find Qt - prefer Qt6
if command -v qmake6 &> /dev/null; then
    QMAKE=qmake6
    MOC=moc6
    QT_VERSION=6
    echo "Using Qt6"
elif command -v qmake-qt6 &> /dev/null; then
    QMAKE=qmake-qt6
    MOC=moc-qt6
    QT_VERSION=6
    echo "Using Qt6"
elif command -v qmake &> /dev/null; then
    QMAKE=qmake
    MOC=moc
    # Check which version
    QT_VER_LINE=$($QMAKE -v | grep "Using Qt version")
    if [[ $QT_VER_LINE == *"6."* ]]; then
        QT_VERSION=6
        echo "Using Qt6"
    else
        QT_VERSION=5
        echo "Using Qt5"
    fi
else
    echo "Error: qmake not found. Please install Qt development packages."
    exit 1
fi

# Get Qt paths
QT_INCLUDE=$($QMAKE -query QT_INSTALL_HEADERS)
QT_LIBS=$($QMAKE -query QT_INSTALL_LIBS)

echo "Qt include path: $QT_INCLUDE"
echo "Qt lib path: $QT_LIBS"

# Clean old moc file if it exists (version mismatch protection)
if [ -f "src/main.moc" ]; then
    echo "Removing old moc file..."
    rm -f src/main.moc
fi

# Create build directory
mkdir -p build
cd build

echo ""
echo "=== Step 1: Running moc (Meta-Object Compiler) ==="
# moc generates the Q_OBJECT implementation
# Generate it in src/ so #include "main.moc" works
$MOC -I../src \
     -I"$QT_INCLUDE" \
     -I"$QT_INCLUDE"/QtCore \
     -I"$QT_INCLUDE"/QtGui \
     -I"$QT_INCLUDE"/QtWidgets \
     ../src/main.cpp -o ../src/main.moc

echo ""
echo "=== Step 2: Compiling source files ==="

# Compile each .cpp file to .o
g++ -c ../src/ssh_store.cpp -o ssh_store.o \
    -I"$QT_INCLUDE" \
    -I"$QT_INCLUDE"/QtCore \
    -I"$QT_INCLUDE"/QtGui \
    -I"$QT_INCLUDE"/QtWidgets \
    -fPIC -std=c++17

g++ -c ../src/ssh_mounter.cpp -o ssh_mounter.o \
    -I../src \
    -I"$QT_INCLUDE" \
    -I"$QT_INCLUDE"/QtCore \
    -I"$QT_INCLUDE"/QtGui \
    -I"$QT_INCLUDE"/QtWidgets \
    -fPIC -std=c++17

g++ -c ../src/main.cpp -o main.o \
    -I../src \
    -I"$QT_INCLUDE" \
    -I"$QT_INCLUDE"/QtCore \
    -I"$QT_INCLUDE"/QtGui \
    -I"$QT_INCLUDE"/QtWidgets \
    -fPIC -std=c++17

echo ""
echo "=== Step 3: Linking ==="

# Link everything together
if [ "$QT_VERSION" = "6" ]; then
    g++ ssh_store.o ssh_mounter.o main.o -o ssh-mounter \
        -L"$QT_LIBS" \
        -lQt6Core -lQt6Gui -lQt6Widgets
else
    g++ ssh_store.o ssh_mounter.o main.o -o ssh-mounter \
        -L"$QT_LIBS" \
        -lQt5Core -lQt5Gui -lQt5Widgets
fi

echo ""
echo "=== Build complete! ==="
echo "Binary: build/ssh-mounter"
echo ""
echo "To run: ./build/ssh-mounter"