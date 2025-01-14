# syntax = docker/dockerfile:1
FROM fedora:latest as builder

# Install necessary packages for building
RUN dnf update -y && \
    dnf install -y mingw64-gcc mingw64-gcc-c++ git && \
    dnf clean all

# Create a working directory
WORKDIR /app

# Copy the source code
COPY hello_world.cpp .

# Compile the code
RUN x86_64-w64-mingw32-g++ hello_world.cpp -o hello_world.exe

# Final runtime stage
FROM fedora:latest

# Install Wine only
RUN dnf update -y && \
    dnf install -y wine && \
    dnf clean all

# Copy the executable from the builder stage
COPY --from=builder /app/hello_world.exe /app/

# Set the working directory
WORKDIR /app

# Run the executable with Wine
CMD ["wine", "hello_world.exe"]
