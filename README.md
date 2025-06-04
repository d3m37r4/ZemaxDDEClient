# ZemaxDDEClient
A C++ GUI test application for interacting with Zemax via DDE (Dynamic Data Exchange).

## Prerequisites
- MSYS2 with MinGW-w64 (`g++` 15.1.0 or later).
- GLFW: `pacman -S mingw-w64-x86_64-glfw`.
- Dear ImGui (included as submodule).
- Zemax installed.

## Setup
1. Clone the repository:
   ```bash
   git clone --recurse-submodules https://github.com/dmitry/ZemaxDDEClient.git
   cd zemax_dde_client
2. Install MSYS2 and dependencies:
   ```bash
    pacman -Syu
    pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-glfw
3. Initialize submodules (if not cloned with --recurse-submodules):
    ```bash
    git submodule update --init --recursive

4. Set up environment in Git Bash:
   ```bash
    export PATH=/c/msys64/mingw64/bin:/c/msys64/usr/bin:$PATH
    export LIBRARY_PATH=/c/msys64/mingw64/lib:$LIBRARY_PATH
    export CPLUS_INCLUDE_PATH=/c/msys64/mingw64/include:$CPLUS_INCLUDE_PATH
   
## Build
   ```bash
    chmod +x build.sh
    ./build.sh
```

## Run
1. Launch Zemax
2. Run Extensions (`F11` hotkey by default)
3. From the list, select an executable file of the form: `ZemaxDDEClient_YYYYMMDDHHMMSS.exe`

## License
This project is licensed under the MIT License.