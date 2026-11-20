<div align="center">
  <img src="https://github.com/user-attachments/assets/44d0875e-d46f-40eb-88ea-ea89875f2f99" width="100px" alt="OpenRBX logo">

  [Discord] | [Contributing]
</div>

[Discord]: https://discord.gg/2JcBQFxVPv
[Contributing]: CONTRIBUTING.md

# OpenRBX

OpenRBX is a source port of Roblox as it was released in December 2007. It is built from the code recovered by [RBXDecomp](https://github.com/OpenRBX-org/RBXDecomp), and the goal is a modern, cross-platform codebase.

<img width="1919" height="1031" alt="Screenshot 2026-11-14 114011" src="https://github.com/user-attachments/assets/fed54e9f-48b7-46ef-9bc2-5b1b7d0a3137" />

## Getting OpenRBX

### Compiling from source

The client and the RBXGS web service build for Windows and Linux. Other platforms including macOS are planned.

| Platform | Toolchain | Notes |
|----------|-----------|-------|
| Windows (x64) | Visual Studio 2022 or later | Builds the client and the RBXGS web service. |
| Linux (x64) | GCC 14 or later | Builds the client and the RBXGS web service. |

#### Prerequisites (Windows)

* [Git](https://git-scm.com/downloads)
* [Visual Studio](https://visualstudio.microsoft.com/) 2022 or later with the Desktop development with C++ workload.
* [Strawberry Perl](https://strawberryperl.com/), used to build OpenSSL

#### Building (Windows)

Open an x64 Native Tools Command Prompt from the Visual Studio start menu folder, then:

```bat
:: Clone the repo
git clone https://github.com/OpenRBX-org/OpenRBX.git
cd OpenRBX

:: Configure and build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

Once the build completes, the client (`Roblox.exe`) and the RBXGS web service (`WebService.exe`) are in the `build` folder.

#### Building (Linux)

Install Git, CMake, Ninja, Make, Perl, GCC 14 or later and the X11, GTK 3 and WebKitGTK development packages. On Ubuntu 24.04:

```sh
sudo apt install git cmake ninja-build make perl g++-14 libgl-dev libx11-dev libxext-dev libxrandr-dev \
  libxcursor-dev libxi-dev libxfixes-dev libxss-dev libxtst-dev libxt-dev libxaw7-dev libxxf86vm-dev \
  libx11-xcb-dev libxcb1-dev libxcb-cursor-dev libxcb-glx0-dev libxcb-icccm4-dev libxcb-image0-dev \
  libxcb-keysyms1-dev libxcb-randr0-dev libxcb-render-util0-dev libxcb-shape0-dev libxcb-shm0-dev \
  libxcb-sync-dev libxcb-util-dev libxcb-xfixes0-dev libxcb-xkb-dev libxkbcommon-dev libxkbcommon-x11-dev \
  libfontconfig1-dev libfreetype-dev libglib2.0-dev libgtk-3-dev libwebkit2gtk-4.1-dev libcups2-dev
```

Then:

```sh
git clone https://github.com/OpenRBX-org/OpenRBX.git
cd OpenRBX
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

Once the build completes, the client (`Roblox`) and the RBXGS web service (`WebService`) are in the `build` folder with the libraries they load.

## Contributing

If you would like to contribute, please see the [contributing guide](CONTRIBUTING.md).

If you want to report bugs or request new features, see [issues](https://github.com/OpenRBX-org/OpenRBX/issues).

## License

The OpenRBX source code is licensed under the [GNU General Public License v3.0](LICENSE).

This project includes third-party components that are separately licensed.
Those components are not covered by the license above and remain subject to their original licenses.
