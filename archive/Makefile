# Simple Makefile for single-file Qt5 project using moc
CXX := g++
MOC := moc
PKG_CONFIG := pkg-config

CXXFLAGS := -std=c++17 -O2 $(shell $(PKG_CONFIG) --cflags Qt5Widgets)
LDLIBS := $(shell $(PKG_CONFIG) --libs Qt5Widgets)

SRC := src/main.cpp
MOC_SRC := src/moc_main.cpp
BIN := build/ssh_mounter
OBJS := build/main.o build/moc_main.o

.PHONY: all clean run

all: $(BIN)

# generate moc file from the .cpp (moc scans for Q_OBJECT)
$(MOC_SRC): $(SRC)
	$(MOC) $(SRC) -o $(MOC_SRC)

# compile sources
build/main.o: $(SRC)
	$(CXX) $(CXXFLAGS) -c $(SRC) -o build/main.o

build/moc_main.o: $(MOC_SRC)
	$(CXX) $(CXXFLAGS) -c $(MOC_SRC) -o build/moc_main.o

# link
$(BIN): $(OBJS)
	$(CXX) -o $(BIN) $(OBJS) $(LDLIBS)

run: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN) $(OBJS) $(MOC_SRC)