# Release Notes - Version 2.1.0

## New & Changed
- **CMake 3.16** minimum version required
- **RSA 2048/4096-bit** key support (not compatible with licensecc 2.0.0)
- **ARM** support for Linux and Windows
- **macOS** support added
- New CLI options: `--version`, `--base64`, `--verbose`

## Tested Systems
| Platform | OS Versions | Arch |
|----------|-------------|------|
| Linux | Ubuntu 24.04, 22.04, 26.04 | x86_64, ARM64 |
| Windows | Server 2025, Server 2022, 11 | x86_64, ARM64 |
| macOS | macOS 26 | ARM64 |

## Compatibility Notes
- **CentOS/Rocky Linux** (RPM-based): untested
- **OpenSSL 1.1.x**: untested (tested with OpenSSL 3.x/4.x)
- **licensecc 2.0.0**: incompatible with keys > 1024 bits (still can use the old ones)

## Build
- Updated CMake configuration and workflows
- Improved Windows packaging and cross-compilation