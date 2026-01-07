# Remote Debugging streamget in Docker with VSCode

This setup allows you to compile and debug the streamget C application inside a Docker container using VSCode.

## Prerequisites

1. **Docker and Docker Compose** installed on your system
2. **Visual Studio Code** with the following extensions:
   - C/C++ Extension (ms-vscode.cpptools)
   - Docker Extension (ms-azuretools.vscode-docker) - optional but recommended

## Setup Overview

The debugging setup consists of:

- `Dockerfile` - GCC container with gdb and required dependencies
- `docker-compose.yml` - Container configuration with volume mounts and port mappings
- `.vscode/tasks.json` - Build tasks for compiling in Docker
- `.vscode/launch.json` - Debug configuration for remote debugging
- `.vscode/c_cpp_properties.json` - IntelliSense configuration

## Quick Start

### Initial setup

Run task "Full build in Docker" to set up the build system inside the container:

   1. Open VSCode in the streamget project folder
   2. Open Command Palette (`Ctrl+Shift+P`)
   3. Type "Tasks: Run Task" and select it
   4. Select "Full build in Docker"

### Build the project

Run task "Build in Docker". This is the default task.

Press `Ctrl+Shift+B` (or `Cmd+Shift+B` on Mac) to run this default build task.
Or use Command Palette (`Ctrl+Shift+P`) → "Tasks: Run Task" → "Build in Docker".

### Start debugging

There is one launch configuration for debugging inside Docker.

In VSCode, press `F5` or go to Run → Start Debugging and select "(gdb) Launch in Docker".

### Set breakpoints

Click in the left margin of any `.c` file to set breakpoints.
The debugger will stop at your breakpoints.

## Available VSCode Tasks

Access tasks via `Ctrl+Shift+P` → "Tasks: Run Task":

- **Start Docker Container** - Starts the development container
- **Full build in Docker** - Full build including ./autogen.sh and ./configure
- **Build in Docker** - Quick rebuild running make only (default: `Ctrl+Shift+B`)
- **Clean in Docker** - Run make clean
- **Stop Docker Container** - Stops and removes the container

Most tasks automatically start the container if it's not already running (with a dependency on "Start Docker Container").

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
docker-compose exec dev bash
```

Inside the container you can:

```bash
./autogen.sh
./configure
make
./src/streamget --help
```

## Tips

- **Edit code on host, build in container** - The workspace folder is mounted as a volume, so changes on your host machine are immediately visible in the container
- **Fast rebuilds** - After initial setup, use "Build in Docker (make only)" for quick rebuilds
- **Persistent container** - The container stays running with `sleep infinity`, so you don't need to restart it between debugging sessions
- **Multiple terminals** - You can have multiple shells open in the same container:

  ```bash
  docker-compose exec dev bash
  ```
