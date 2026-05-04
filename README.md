# McLaren Senna VR Viewer

EEEE2076 – Software Project | University of Nottingham  
Qt 6 + VTK 9 + OpenVR | Group Assessment

## Overview

An interactive 3D viewer for the McLaren Senna CAD model. Load STL parts into a tree view, edit their properties, apply VTK filters, animate the scene, and view the model in VR via SteamVR (Windows only).

## Features

| Feature | Details |
|---------|---------|
| STL loading | Load one or more `.stl` files into a hierarchical tree |
| Part editing | Name, colour (QColorDialog picker), visibility per part |
| VTK filters | Shrink filter and Clip filter, toggleable per part |
| Lighting | Key + fill lights in both 3D viewer and VR |
| Animation | Camera auto-rotation at ~60 fps |
| VR | Threaded OpenVR session (start/stop/restart without UI freeze) |
| VR environment | Virtual floor plane at real-world floor height |

## Dependencies

| Dependency | Version | Notes |
|-----------|---------|-------|
| Qt | 6.5+ | Widgets, OpenGLWidgets |
| VTK | 9.x | Built with Qt + OpenVR support (Windows) |
| OpenVR | 1.x | Windows only (`C:/OpenVR/`) |
| CMake | 3.19+ | Build system |

## Building

### macOS (Homebrew)

```bash
brew install cmake qt vtk

git clone https://github.com/MoSoltan/UniProject.git
cd UniProject

cmake -B build -S .
cmake --build build -- -j$(sysctl -n hw.logicalcpu)

./build/Worksheet6
```

> VR is not available on macOS. The VR button will show an informational message.

### Windows (Visual Studio)

Prerequisites:
- Visual Studio 2022
- Qt 6.5+ (add to PATH or set `Qt6_DIR`)
- VTK 9.x compiled with `Module_vtkRenderingOpenVR=ON` at `C:/Program Files (x86)/VTK/`
- OpenVR SDK at `C:/OpenVR/`

```bat
git clone https://github.com/MoSoltan/UniProject.git
cd UniProject

cmake -B build -S . -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Open `build/Worksheet6.sln` in Visual Studio, or run the executable from `build/Release/`.

> Copy `Mclaren_Senna_clean_centered.stl` into the build directory before running.

## Usage

1. **File → Open** (or toolbar icon) to load an STL file
2. Select a part in the tree view, then click **Button 1** to open Part Options
3. In Part Options: change name, pick a colour, toggle visibility, enable filters
4. Click **Animate** to start/stop camera rotation
5. Click **Start VR** (Windows) to launch the VR session — click **Stop VR** to end it

## Project Structure

```
Exercise3/
├── main.cpp               — Entry point
├── mainwindow.cpp/h       — Main window, VTK pipeline, animation
├── mainwindow.ui          — Qt Designer UI layout
├── ModelPart.cpp/h        — Tree node: STL reader + VTK pipeline per part
├── ModelPartList.cpp/h    — QAbstractItemModel backing the tree view
├── optiondialog.cpp/h     — Part properties dialog (colour picker, filters)
├── optiondialog.ui        — Dialog layout
├── VRRenderThread.cpp/h   — Threaded VR render loop (Windows only)
├── vrbindings/            — SteamVR controller binding JSON files
├── icons/                 — Toolbar icons
└── CMakeLists.txt         — Cross-platform build (macOS + Windows)
```

## Documentation

Doxygen comments are applied to all classes, functions, and member variables.  
Generate HTML docs:

```bash
doxygen Doxyfile
# Output in docs/html/index.html
```

## Git

- `main` — stable baseline
- `mac-build-support` — macOS compatibility + feature additions

## Authors

Group project — EEEE2076, 2024/25
