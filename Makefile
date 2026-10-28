# Сборка Multi-Algo Cryptotool (cryptum)
#   Linux:   make, make test, make clean
#   Windows: mingw32-make, mingw32-make test, mingw32-make clean

CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror
INCLUDES = -Iinclude -Isrc -Iciphers/common

ifeq ($(OS),Windows_NT)
    EXE         = .exe
    LIB_PREFIX  =
    LIB_EXT     = .dll
    PIC         =
    MAIN_LIBS   = -static-libgcc -static-libstdc++
    CIPHER_LIBS = -lbcrypt -static-libgcc -static-libstdc++
    MAKE_DIRS   = if not exist bin mkdir bin & if not exist build mkdir build
    REMOVE_DIRS = if exist bin rmdir /S /Q bin & if exist build rmdir /S /Q build
    RUN_TESTS   = bin\tests.exe
else
    EXE         =
    LIB_PREFIX  = lib
    LIB_EXT     = .so
    PIC         = -fPIC -fvisibility=hidden
    MAIN_LIBS   = -ldl
    CIPHER_LIBS =
    MAKE_DIRS   = mkdir -p bin build
    REMOVE_DIRS = rm -rf bin build
    RUN_TESTS   = ./bin/tests
endif

PROGRAM   = bin/cryptum$(EXE)
TESTS     = bin/tests$(EXE)
LIBRARIES = bin/$(LIB_PREFIX)atbash$(LIB_EXT) bin/$(LIB_PREFIX)transposition$(LIB_EXT) bin/$(LIB_PREFIX)tea$(LIB_EXT)

MAIN_OBJECTS  = build/main.o build/menu.o build/console_ui.o build/file_browser.o build/platform.o \
                build/library_loader.o build/byte_io.o build/crypto_operations.o
TEST_OBJECTS  = build/cipher_tests.o build/platform.o build/library_loader.o

COMMON_HEADERS = include/crypto_api.h include/secure_memory.h

all: directories $(LIBRARIES) $(PROGRAM) $(TESTS)

directories:
	@$(MAKE_DIRS)

$(PROGRAM): $(MAIN_OBJECTS)
	$(CXX) $(MAIN_OBJECTS) -o $@ $(MAIN_LIBS)

$(TESTS): $(TEST_OBJECTS)
	$(CXX) $(TEST_OBJECTS) -o $@ $(MAIN_LIBS)

build/%.o: src/%.cpp src/*.h $(COMMON_HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

build/cipher_tests.o: tests/cipher_tests.cpp src/*.h $(COMMON_HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

CIPHER_DEPENDENCIES = ciphers/common/random_bytes.h $(COMMON_HEADERS)
BUILD_LIBRARY = $(CXX) $(CXXFLAGS) $(INCLUDES) $(PIC) -shared $< -o $@ $(CIPHER_LIBS)

bin/$(LIB_PREFIX)atbash$(LIB_EXT): ciphers/atbash/atbash.cpp $(CIPHER_DEPENDENCIES)
	$(BUILD_LIBRARY)

bin/$(LIB_PREFIX)transposition$(LIB_EXT): ciphers/transposition/transposition.cpp $(CIPHER_DEPENDENCIES)
	$(BUILD_LIBRARY)

bin/$(LIB_PREFIX)tea$(LIB_EXT): ciphers/tea/tea.cpp $(CIPHER_DEPENDENCIES)
	$(BUILD_LIBRARY)

test: all
	$(RUN_TESTS)

clean:
	-@$(REMOVE_DIRS)

.PHONY: all directories test clean
