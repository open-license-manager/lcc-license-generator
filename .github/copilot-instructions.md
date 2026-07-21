# Copilot Instructions for lcc-license-generator

## Project Overview
C/C++ license generator for the licensecc project. Creates projects with public/private key pairs and generates hardware-locked licenses. Built with CMake and uses OpenSSL (or Windows Crypto API) for cryptographic operations.

## Key Components
- **License Generation**: Creates signed license files with hardware binding
- **Project Initialization**: Generates new projects with public/private key pairs
- **Crypto Implementation**: Cross-platform cryptographic helper using OpenSSL or Windows Crypto API
- **Command-Line Interface**: Tool for project management and license generation

## Architecture
- `src/base_lib/` - Core cryptographic functionality and base utilities
- `src/license_generator/` - Main application logic for license generation
- `src/inja/` - Inja template engine (third party library): https://github.com/pantor/inja/blob/main/README.md . Do not modify this folder.
- `test/` - Unit tests using Boost.Test framework
- `cmake/` - CMake modules and build configurations

## Key Classes
- `CryptoHelper` - Abstract class for cryptographic operations with platform-specific implementations
- `License` - Handles license creation and parameter management
- `Project` - Manages project initialization and key generation
- `CommandLineParser` - Parses command-line arguments and executes commands

## Build Configuration
- C++11 standard required
- Supports both static and dynamic linking (controlled by STATIC_RUNTIME flag)
- Cross-compilation support for Windows from Linux using MinGW
- OpenSSL support (mandatory on Linux, optional on Windows)
- Uses Boost libraries for various utilities

## Cryptographic Features
- RSA key pairs (default 1024-bit, configurable)
- SHA256withRSA signature algorithm
- Base64 encoding for signatures
- Support for both OpenSSL and Windows Crypto API
- Private keys in OpenSSL format, public keys in binary format

## Command-Line Interface
- `lcc project init` - Initialize a new licensing project
- `lcc license generate` - Generate a license file
- Parameters passed via command-line options

## Testing
- Unit tests using Boost.Test framework
- CI/CD integration with Linux and Windows builds

## Platform Support
- Linux (primary development platform)
- Windows (with cross-compilation support)
- Cross-platform abstractions for crypto operations
- Mac OS

## Version Requirements
Compatible with the following versions:
- OpenSSL from 1.1.1 to 4.0 (mandatory on Linux and Mac OS, optional on Windows)
- Boost 1.69 to 1.90 (for various utilities and testing)
- CMake 3.10 to 4.0 for build configuration
Code for specific versions must be protected with preprocessor directives to ensure compatibility across different environments.

## Coding Standards
- C++11 compliant code
- RAII principles for resource management
- Exception handling for error conditions
- Consistent naming conventions (snake_case for functions/variables)
- Proper const-correctness
- Memory-safe operations with smart pointers

## Common Tasks
- Adding new license parameters: Extend the License class and its values_map
- Modifying cryptographic algorithms: Implement in CryptoHelper subclasses
- Adding command-line options: Update CommandLineParser class
- Creating new tests: Add to test/ directory using Boost.Test framework

