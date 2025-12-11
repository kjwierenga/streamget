# Remote Debugging streamget in Docker with VSCode

This setup allows you to compile and debug the streamget C application inside a Docker container using VSCode.

## Prerequisites

1. **Docker and Docker Compose** installed on your system
2. **Visual Studio Code** with the following extensions:
   - C/C++ Extension (ms-vscode.cpptools)
   - Docker Extension (ms-azuretools.vscode-docker) - optional but recommended

## Setup Overview

The debugging setup consists of:
- `Dockerfile` - GCC container with gdbserver and required dependencies
- `docker-compose.yml` - Container configuration with volume mounts and port mappings
- `.vscode/tasks.json` - Build tasks for compiling in Docker
- `.vscode/launch.json` - Debug configurations for remote debugging
- `.vscode/c_cpp_properties.json` - IntelliSense configuration
- `debug-in-docker.sh` - Helper script to start gdbserver manually

## Quick Start

### Method 1: Using VSCode (Recommended)

1. **Start the Docker container:**
   ```bash
   docker-compose up -d
   ```

2. **Build the project:**
   - Press `Ctrl+Shift+B` (or `Cmd+Shift+B` on Mac) to run the default build task
   - Or use Command Palette (`Ctrl+Shift+P`) → "Tasks: Run Task" → "Full Build in Docker"

3. **Start debugging:**
   - First, start gdbserver manually in a terminal:
     ```bash
     ./debug-in-docker.sh --url http://example.com/stream.mp3 --output test.mp3
     ```
   - Then in VSCode, press `F5` or go to Run → Start Debugging
   - Select "(gdb) Remote Debug in Docker"

4. **Set breakpoints:**
   - Click in the left margin of any `.c` file to set breakpoints
   - The debugger will stop at your breakpoints

### Method 2: Manual gdbserver

1. **Start the container and build:**
   ```bash
   docker-compose up -d
   docker-compose exec streamget-dev bash -c './autogen.sh && ./configure && make'
   ```

2. **Start gdbserver in the container:**
   ```bash
   docker-compose exec streamget-dev gdbserver :2345 /workspace/src/streamget <args>
   ```

3. **Attach from VSCode:**
   - Press `F5` and select "(gdb) Remote Debug in Docker"

## Available VSCode Tasks

Access tasks via `Ctrl+Shift+P` → "Tasks: Run Task":

- **Start Docker Container** - Starts the development container
- **Build in Docker (autogen)** - Full build including ./autogen.sh and ./configure
- **Build in Docker (make only)** - Quick rebuild (default: `Ctrl+Shift+B`)
- **Clean in Docker** - Run make clean
- **Full Build in Docker** - Starts container and does full build
- **Stop Docker Container** - Stops and removes the container

## Debug Configurations

Two debug configurations are available in the Run panel:

1. **(gdb) Remote Debug in Docker** - Launches the program under gdbserver
   - Automatically builds before debugging
   - Stops at program entry point
   - Edit the `args` array in `.vscode/launch.json` to pass arguments

2. **(gdb) Attach to Process in Docker** - Attaches to a running process
   - Useful for debugging already-running instances

## Working Inside the Container

To open a shell in the container:
```bash
docker-compose exec streamget-dev bash
```

Inside the container you can:
```bash
cd /workspace
./autogen.sh
./configure
make
./src/streamget --help
```

## Troubleshooting

### Container won't start
```bash
docker-compose down
docker-compose up -d
docker-compose logs
```

### Build fails
Make sure all dependencies are installed:
```bash
docker-compose exec streamget-dev bash
apt-get update
apt-get install -y libcurl4-openssl-dev autoconf automake
```

### Debugger won't connect
1. Make sure gdbserver is running in the container
2. Check that port 2345 is mapped: `docker-compose ps`
3. Verify the container is running: `docker ps`

### Breakpoints not working
1. Ensure the program is built with debug symbols (`-g` flag)
2. Check that source file paths match between host and container
3. Verify `sourceFileMap` in `.vscode/launch.json` is correct

## Tips

- **Edit code on host, build in container** - The workspace folder is mounted as a volume, so changes on your host machine are immediately visible in the container
- **Fast rebuilds** - After initial setup, use "Build in Docker (make only)" for quick rebuilds
- **Persistent container** - The container stays running with `sleep infinity`, so you don't need to restart it between debugging sessions
- **Multiple terminals** - You can have multiple shells open in the same container:
  ```bash
  docker-compose exec streamget-dev bash
  ```

## Example Debug Session

1. Set a breakpoint in `src/main.c` at line where arguments are parsed
2. Run the debug script:
   ```bash
   ./debug-in-docker.sh --url http://stream.example.com/audio.mp3 --output test.mp3 --time-limit 60
   ```
3. In VSCode, press `F5` to attach the debugger
4. Step through code using:
   - `F10` - Step Over
   - `F11` - Step Into
   - `Shift+F11` - Step Out
   - `F5` - Continue

## Cleaning Up

To stop and remove the container:
```bash
docker-compose down
```

To also remove the built image:
```bash
docker-compose down --rmi all
```
