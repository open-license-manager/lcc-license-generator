# Copilot Instructions for lcc-license-generator /src/license_generator folder

- respect general coding standards and conventions of the project in main copilot-instructions.md
- this folder contains the main application logic for license generation, including command-line parsing and execution of commands.

## Key Classes

- `License` - Handles license creation and parameter management
- `Project` (files:[project.hpp, project.cpp])- Manages project initialization and key generation
- `CommandLineParser` (files:[command_line-parser.hpp, command_line-parser.cpp]): Parses command-line arguments and executes commands. All the command-line options and validation for user parameters are defined in this class.