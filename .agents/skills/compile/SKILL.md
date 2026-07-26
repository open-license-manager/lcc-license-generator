# Project Compilation Skill

## Overview

This skill provides instructions for configuring and compiling the lcc-license-generator project.

## Configuration Steps

Below: 
${ workspace } is the folder where the project has been checked out.
${ build-dir } is "${workspace}/build" the folder where the project is built

### 1. Configure with CMake

```bash
cd ${ build-dir } && rm -rf CMakeCache.txt; cmake -S ${ workspace } -B ${ build-dir }
```

Or with specific options:
```bash
${ build-dir } && && rm -rf CMakeCache.txt; cmake -S ${ workspace } -B ${ build-dir } -DSTATIC_RUNTIME=ON
```

### 2. Build the Project

```bash
cd ${ build-dir } && cmake --build . -j 8
```

## Build Options


### Static Runtime Linking
- `-DSTATIC_RUNTIME=ON` (default): Creates statically linked executable
- `-DSTATIC_RUNTIME=OFF`: Creates dynamically linked executable

### OpenSSL Configuration

- On Linux: OpenSSL is mandatory and will be automatically detected
- On Windows: OpenSSL is optional, will fall back to Windows Crypto API if not found. 
  - If OpenSSL found but still want to compile without it use `-DUSE_OPENSSL=OFF`

### Boost Configuration

- Boost libraries are automatically detected
- Required components: date_time, filesystem, program_options, unit_test_framework

## Testing

After building, run tests with:
```bash
ctest ${ build-dir }
```

## Location of Executable
The main executable is built at: 
`build/src/license_generator/lccgen`

## Troubleshooting
- If the project does not compile try to do the configuration step again.
- If OpenSSL is not found on Linux, install OpenSSL development packages
- If Boost is not found, install Boost development packages
- For cross-compilation to Windows, ensure MinGW toolchain is installed