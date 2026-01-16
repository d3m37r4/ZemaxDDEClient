# ZemaxDDEClient

<a id="en"></a>

>**🌐 Documentation available in multiple languages:**
[![EN](https://img.shields.io/badge/%F0%9F%87%AC%F0%9F%87%A7%20EN-blue?style=flat-square)](#en)
[![RU](https://img.shields.io/badge/%F0%9F%87%B7%F0%9F%87%BA%20RU-inactive?style=flat-square)](README.ru.md#ru)

[![Latest Release](https://img.shields.io/github/v/release/d3m37r4/ZemaxDDEClient?include_prereleases&style=flat-square&color=blue)](https://github.com/d3m37r4/ZemaxDDEClient/releases)
[![Build Test](https://img.shields.io/github/actions/workflow/status/d3m37r4/ZemaxDDEClient/.github%2Fworkflows%2Fbuild-test.yml?branch=main&style=flat-square&label=build%20test)](https://github.com/d3m37r4/ZemaxDDEClient/actions/workflows/build-test.yml)
[![License](https://img.shields.io/github/license/d3m37r4/ZemaxDDEClient?style=flat-square)](https://github.com/d3m37r4/ZemaxDDEClient/blob/main/LICENSE)

## 📌 About
An application for [Zemax](https://en.wikipedia.org/wiki/Zemax) that enables advanced analysis of optical systems through direct access to their parameters via DDE (Dynamic Data Exchange).

<details>
<summary><b>Working with Zemax via DDE</b></summary>

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

## 🔽 Downloads
Pre-built binaries for Windows are available on the [Releases](https://github.com/d3m37r4/ZemaxDDEClient/releases) page.

- [**Release builds**](https://github.com/d3m37r4/ZemaxDDEClient/releases) — stable, tested versions.  
- [**Dev builds**](https://github.com/d3m37r4/ZemaxDDEClient/actions) — test builds with new or experimental features not yet included in a release (may be unstable).

> 💡 Each build includes two versions:<br>
> – **Release** — for regular use,<br>
> – **Debug** — with console output and extended logging for debugging.<br>
> Both share the same build number for easy comparison.

## 🏁 Run
You can launch the application in two ways: via Zemax’s **Extensions** menu or by running the executable directly.

### Option 1: Launch via Zemax Extensions
This method integrates the application into Zemax’s Extensions menu for convenient access.
1. Launch **Zemax**
2. Open the Extensions menu (default hotkey `F11`) 
3. From the list, select an executable file named like: `ZemaxDDEClient_*.exe`
> 💡 Zemax scans the `Extend` folder inside its installation directory (typically `C:\Program Files\Zemax\Extend\`) and displays all `.exe` files from it in the **Extensions** menu. To make the application permanently available, place your executable in this folder.

### Option 2: Direct Launch
You can run the executable directly without adding it to Zemax.
1. Launch **Zemax**
2. Run the `ZemaxDDEClient_*.exe` file
3. Click the **Connect to Zemax** button in the **Sidebar** window

## 📦 Prerequisites
- **MSYS2** with MinGW-w64 toolchain
- **CMake** (≥ 3.16)
- **GLFW**: `pacman -S mingw-w64-x86_64-glfw`
- **Zemax** (acts as a DDE server once running)

## 📚 Third-Party Libraries
This project uses the following third-party libraries:
- **[Dear ImGui](https://github.com/ocornut/imgui)** by Omar Cornut — Immediate Mode GUI
- **[ImPlot](https://github.com/epezent/implot)** by Evan Pezent — 2D plotting library for Dear ImGui
- **[Native File Dialog (NFD)](https://github.com/mlabbe/nativefiledialog)** by Michael Labbe — Cross-platform file dialogs

## 🚀 Build Setup
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
3. Fetch the submodules (if not cloned with --recurse-submodules):
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
The project uses CMake for reliable builds. For convenience, it is recommended to use the `build.sh` wrapper script.
   ```bash
   # Make the build.sh file executable
   chmod +x build.sh

   # Build the Release version (used by default)
   ./build.sh

   # Build the Debug version (with visible console and DEBUG_LOG enabled)
   ./build.sh debug

   # Build with optimization: -O2
   ./build.sh optimize=1

   # Build with maximum optimization: -O3
   ./build.sh optimize=2

   # Debug build with -O2 optimization (example of combining arguments)
   ./build.sh debug optimize=1

   # Clean the build directory
   ./build.sh clean

   # Build with a fixed timestamp — for stable artifacts in CI and automation
   BUILD_TIMESTAMP=1766073737 ./build.sh release
   ```

Manual build via CMake (if fine-grained control or debugging of the build process is needed):
   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   ```

## 📄 License
This project is licensed under the [MIT License](https://github.com/d3m37r4/ZemaxDDEClient/blob/main/LICENSE).

## 🤝 Contribution and support
If you have any thoughts, bug reports, or suggestions to improve the product, please contact me at one of the following places:

[![GitHub Issues](https://img.shields.io/badge/GitHub-Issues-blue?logo=github&style=flat-square)](https://github.com/d3m37r4/ZemaxDDEClient/issues)  
[![GitHub Discussions](https://img.shields.io/badge/GitHub-Discussions-blue?logo=github&style=flat-square)](https://github.com/d3m37r4/ZemaxDDEClient/discussions)  
[![Telegram](https://img.shields.io/badge/Telegram-dmitry__isakow-blue?logo=telegram&style=flat-square)](https://t.me/dmitry_isakow)

You can also submit a [pull request](https://github.com/d3m37r4/ZemaxDDEClient/pulls).
