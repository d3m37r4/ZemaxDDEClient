# ZemaxDDEClient
A C++ GUI test application for interacting with Zemax via DDE (Dynamic Data Exchange).

<details>
<summary><b>About DDE and Zemax Interaction</b></summary>

### What is DDE?
Dynamic Data Exchange (DDE) is a Windows interprocess communication protocol that enables data exchange between applications. Two applications can establish a DDE link:
- **Server** (Zemax in this case) - provides data
- **Client** (your application) - requests and receives data

Zemax implements a DDE server interface, allowing external applications to access optical design data and functionality.

### How ZemaxDDEClient Works
This project is a DDE client that:
1. Establishes DDE connection with Zemax as server
2. Sends data requests (e.g., optical system parameters)
3. Receives and processes responses from Zemax

## Documentation Reference
For complete DDE command reference, see "Chapter 28: ZEMAX EXTENSIONS" in official Zemax documentation.
</details>

## Prerequisites
- MSYS2 with MinGW-w64 (`g++` 15.1.0 or later).
- GLFW: `pacman -S mingw-w64-x86_64-glfw`.
- Dear ImGui (included as submodule).
- Zemax installed.

## Setup
1. Clone the repository:
   ```bash
   git clone --recurse-submodules https://github.com/d3m37r4/ZemaxDDEClient.git
   cd ZemaxDDEClient
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