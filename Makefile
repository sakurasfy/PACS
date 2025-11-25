# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -O2 -Wall -Xpreprocessor -fopenmp

# Include paths
INCLUDES = -I. \
           -I./BGN \
           -I./AIPE \
           -I/opt/homebrew/opt/libomp/include \
           -I/opt/homebrew/opt/flint/include \
           -I/opt/homebrew/opt/flint/include/flint \
           -I/opt/homebrew/opt/openssl@3/include \
           -I/opt/homebrew/opt/pbc/include \
           -I/opt/homebrew/include \
           -I/usr/local/include

# Library paths
LDFLAGS = -L/opt/homebrew/opt/libomp/lib \
          -L/opt/homebrew/lib \
          -L/usr/local/lib

# Libraries to link
LIBS = -lhcs -lgmp -lflint -lomp -lntl -lm -lcryptopp -lssl -lcrypto -lpbc -lgmpxx

# Source files
SRCS = main.cpp \
       BGN/BGN.cpp \
       AIPE/matrices.cpp \
       AIPE/cryptorand.cpp \
       AIPE/ipe.cpp \
       AIPE/base.cpp
OBJS = $(SRCS:.cpp=.o)

# Executable name
TARGET = main

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
	@echo "Build successful! Executable: $(TARGET)"

# Compile source files to object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Run the program
run: $(TARGET)
	./$(TARGET)

# Clean build files
clean:
	rm -f $(OBJS) $(TARGET)
	@echo "Clean complete"

# Clean and rebuild
rebuild: clean all

# Show variables (for debugging Makefile)
debug:
	@echo "CXX: $(CXX)"
	@echo "CXXFLAGS: $(CXXFLAGS)"
	@echo "INCLUDES: $(INCLUDES)"
	@echo "LDFLAGS: $(LDFLAGS)"
	@echo "LIBS: $(LIBS)"
	@echo "SRCS: $(SRCS)"
	@echo "OBJS: $(OBJS)"
	@echo "TARGET: $(TARGET)"

.PHONY: all run clean rebuild debug
