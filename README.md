# MCResCalc

MCResCalc is a Qt-based desktop utility for Minecraft that calculates the total raw materials needed to craft an item or batch of items. It loads recipe data from a selected Minecraft version, resolves item tags and language names, displays the crafting tree, and shows a step-by-step crafting guide for the final result.

## Features

- Download version metadata directly from Mojang
- Download and extract the selected Minecraft client jar
- Parse recipes, tags, language files, and item metadata
- Search for craftable items by name or prefix
- Add multiple items to a batch for calculation
- Expand a full crafting tree to see all intermediate steps
- Display raw material totals with stack counts
- Show a 2D icon or 3D block preview for the selected item
- Support English and Traditional Chinese item names
- Includes a crafting tutorial view for each recipe step

## Project overview

This project is built with C++ and Qt 6. It uses:

- Qt Widgets for the main interface
- Qt Network for Mojang API access
- Qt Quick / Qt Quick 3D for block previews
- QuaZip for extracting Minecraft jar files
- CMake for building the project

The application reads recipe and tag data from the game's client jar and calculates the required materials based on the selected version.

## Supported Minecraft versions

The app queries Mojang's release version list and allows you to choose a release version before loading recipe data. It then downloads the matching client jar and extracts recipe information for that version.

## Requirements

Before building, install the following:

- CMake 3.21 or newer
- C++20 compiler
- Qt 6.11 or newer
- vcpkg (recommended for dependency management)
- QuaZip package for Qt 6

Required Qt components:

- Core
- Widgets
- Network
- Concurrent
- Qml
- Quick
- QuickWidgets
- Quick3D

## Build instructions

1. Clone the repository:

   ```bash
   git clone https://github.com/neil289506-commits/MCResCalc.git
   cd MCResCalc
   ```

2. Install dependencies with vcpkg:

   ```bash
   vcpkg install
   ```

3. Configure the project:

   ```bash
   cmake -B build -S .
   ```

   If your environment uses a custom vcpkg toolchain file, configure it like this:

   ```bash
   cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
   ```

4. Build it:

   ```bash
   cmake --build build
   ```

5. Run the application:

   ```bash
   ./build/MinecraftResourceCalculator
   ```

   On Windows, the executable may be generated under `build/Debug` or `build/Release` depending on your generator.

## How to use

1. Open the application.
2. Select a Minecraft version from the dropdown.
3. Click `Load recipe data`.
4. Search for an item in the search box.
5. Add one or more items to the batch list.
6. Click `Calculate required raw materials`.
7. Review:
   - the raw ingredient totals,
   - the full crafting tree,
   - the crafting steps tutorial,
   - and any warnings or skipped recipes.

## Repository structure

```text
MCResCalc/
├── CMakeLists.txt
├── app.ico
├── app.rc
├── resources.qrc
├── theme.qss
├── vcpkg.json
├── qml/
│   └── BlockPreview.qml
├── src/
│   ├── main.cpp
│   ├── Models/
│   ├── Services/
│   └── UI/
└── README.md
```

## License

This project does not currently declare a license in the repository metadata, so usage rights are not specified here. Please check the repository's license file or project settings before redistribution or commercial use.

## Notes

This project is designed around Minecraft recipe data and may require internet access to fetch version metadata and client jars from Mojang. The file extraction and parsing process can take some time for larger versions, but the app is built to keep the interface responsive while processing the package in the background.

## Contributing

Contributions are welcome. If you would like to improve the app, fix a bug, or add support for more recipe cases, feel free to open an issue or submit a pull request.
