# Use Ubuntu 22.04 as base image
FROM ubuntu:22.04

# Set non-interactive installation
ENV DEBIAN_FRONTEND=noninteractive

# Install basic dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    g++ \
    make \
    cmake \
    git \
    wget \
    curl \
    unzip \
    libgmp-dev \
    libssl-dev \
    libntl-dev \
    libmpfr-dev \
    m4 \
    flex \
    bison \
    autoconf \
    automake \
    libtool \
    pkg-config \
    python3 \
    && rm -rf /var/lib/apt/lists/*

# Install FLINT from GitHub (use stable version 2.9.0)
RUN cd /tmp && \
    git clone --depth 1 --branch v2.9.0 https://github.com/flintlib/flint.git && \
    cd flint && \
    ./configure --prefix=/usr/local && \
    make -j$(nproc) && \
    make install && \
    ldconfig && \
    cd / && rm -rf /tmp/flint

# Install PBC (Pairing-Based Cryptography Library)
RUN cd /tmp && \
    wget https://crypto.stanford.edu/pbc/files/pbc-0.5.14.tar.gz && \
    tar -xzf pbc-0.5.14.tar.gz && \
    cd pbc-0.5.14 && \
    ./configure --prefix=/usr/local && \
    make -j$(nproc) && \
    make install && \
    ldconfig && \
    cd / && rm -rf /tmp/pbc-0.5.14*

# Install Crypto++ library
RUN cd /tmp && \
    wget https://github.com/weidai11/cryptopp/releases/download/CRYPTOPP_8_9_0/cryptopp890.zip && \
    unzip -q cryptopp890.zip -d cryptopp && \
    cd cryptopp && \
    make -j$(nproc) && \
    make install PREFIX=/usr/local && \
    ldconfig && \
    cd / && rm -rf /tmp/cryptopp*

# Install libhcs (Homomorphic Cryptosystems Library)
RUN cd /tmp && \
    git clone --depth 1 https://github.com/tiehuis/libhcs.git && \
    cd libhcs && \
    cmake . && \
    make -j$(nproc) && \
    make install && \
    ldconfig && \
    cd / && rm -rf /tmp/libhcs

# Set working directory
WORKDIR /app

# Copy project files
COPY . /app/

# Create Makefile for Docker environment
RUN cat > Makefile.docker <<'EOF'
# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -O2 -Wall

# Include paths
INCLUDES = -I. \
           -I./BGN \
           -I./AIPE \
           -I/usr/local/include \
           -I/usr/local/include/flint

# Library paths
LDFLAGS = -L/usr/local/lib

# Libraries to link
LIBS = -lhcs -lgmp -lflint -lntl -lm -lcryptopp -lssl -lcrypto -lpbc -lgmpxx

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

.PHONY: all run clean rebuild
EOF

# Build the project
RUN make -f Makefile.docker clean && \
    make -f Makefile.docker -j$(nproc)

# Set default command
CMD ["./main"]
