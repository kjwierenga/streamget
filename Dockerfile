FROM gcc:latest

# Install required packages for building and debugging
RUN apt-get update && apt-get install -y \
    gdb \
    valgrind \
    libcurl4-openssl-dev \
    autoconf \
    automake \
    make \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /workspace

# No need to copy project files, will be mounted as volume
# COPY . /workspace

# Keep container running
CMD ["sleep", "infinity"]
