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

## 📦 Prerequisites
- **MSYS2** with MinGW-w64 toolchain
- **CMake** (≥ 3.16)
- **GLFW**: `pacman -S mingw-w64-x86_64-glfw`
- **Zemax** (running as DDE server)

## 🚀 Setup
1. Clone the repository:
   ```bash
   git clone --recurse-submodules https://github.com/d3m37r4/ZemaxDDEClient.git
   cd ZemaxDDEClient
   ```
2. Install MSYS2 and dependencies:
   ```bash
   pacman -Syu
   pacman -S mingw-w64-x86_64-toolchain mingw-w64-x86_64-glfw cmake
   ```
3. Initialize submodules (if not cloned with --recurse-submodules):
   ```bash
   git submodule update --init --recursive
   ```
4. Set up environment in Git Bash:
   ```bash
    export PATH=/c/msys64/mingw64/bin:/c/msys64/usr/bin:$PATH
    export LIBRARY_PATH=/c/msys64/mingw64/lib:$LIBRARY_PATH
    export CPLUS_INCLUDE_PATH=/c/msys64/mingw64/include:$CPLUS_INCLUDE_PATH
    ```

## 🔨 Build
The project uses CMake for reliable, cross-platform builds. Use the build.sh wrapper for more convenience.
   ```bash
   # Make build.sh executable
   chmod +x build.sh

   # Build in Release mode (default)
   ./build.sh

   # Build in Debug mode (console visible, DEBUG_LOG enabled)
   ./build.sh debug

   # Build with optimization: -O2
   ./build.sh optimize=1

   # Build with maximum optimization: -O3
   ./build.sh optimize=2

   # Debug build with -O2
   ./build.sh debug optimize=1

   # Clean and rebuild
   ./build.sh clean
   ./build.sh release
   ```

Manual CMake usage (advanced)
   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   ```

## 🏁 Run
You can launch the ZemaxDDEClient in two ways: via Zemax's **Extensions menu**, or by **running the executable directly**.

### Option 1: Launch via Zemax Extensions
This method integrates your application into Zemax’s Extensions menu for easy access.
1. Launch **Zemax**
2. Open Extensions menu (`F11` hotkey by default)
3. From the list, select an executable file of the form: `ZemaxDDEClient_*.exe`
> 💡 **Tip**: To make the client permanently available, copy the `.exe` file to Zemax’s extensions directory:  
> `C:\Program Files\Zemax\Extend\`

### Option 2: Run Directly
You can run the executable directly without adding it to Zemax.
1. Launch **Zemax**
2. Run the application executable file `ZemaxDDEClient_*.exe`
3. Initiate a connection to **Zemax**

> ✅ Both methods establish a DDE connection with Zemax.

## 📄 License
This project is licensed under the MIT License.
