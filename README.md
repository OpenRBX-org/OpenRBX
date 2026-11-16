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

For now OpenRBX builds only for Windows. This will change as the port progresses.

| Platform | Toolchain | Notes |
|----------|-----------|-------|
| Windows (x86) | Visual Studio 2022 or later | Builds the client and the RBXGS web service. |
| Windows (x64) | Visual Studio 2022 or later | Builds the client and the RBXGS web service. |

#### Prerequisites

* [Git](https://git-scm.com/downloads)
* [Visual Studio](https://visualstudio.microsoft.com/) 2022 or later with the Desktop development with C++ workload and the C++ MFC and C++ ATL components. It ships CMake and Ninja.

#### Building

Open an x64_x86 Cross Tools Command Prompt (x86) or an x64 Native Tools Command Prompt (x64) from the Visual Studio start menu folder, then:

```bat
:: Clone the repo
git clone https://github.com/OpenRBX-org/OpenRBX.git
cd OpenRBX

:: Configure and build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

Once the build completes, the client (`Roblox.exe`) and the RBXGS web service (`WebService.dll`) are in the `build` folder.

## Contributing

If you would like to contribute, please see the [contributing guide](CONTRIBUTING.md).

If you want to report bugs or request new features, see [issues](https://github.com/OpenRBX-org/OpenRBX/issues).

## License

The OpenRBX source code is licensed under the [GNU General Public License v3.0](LICENSE).

This project includes third-party components that are separately licensed.
Those components are not covered by the license above and remain subject to their original licenses.
