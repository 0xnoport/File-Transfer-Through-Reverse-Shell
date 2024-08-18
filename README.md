# Reverse Shell File Infiltration Tool

<br>

## **IMPORTANT: This tool currently only supports file infiltration**

<br>

## Overview

This program is a simple Linux tool that allows files to be infiltrated into Windows and Linux systems through a reverse shell. The current development state only supports file uploads to the target system, while **file exfiltration is not yet implemented**. The program leverages active reverse shells to transmit files by simulating user input.

## Features

- **Cross-Platform**: Supports file uploads to both Windows and Linux systems.
- **Reverse Shell Utilization**: Infiltrates files through active reverse shells by simulating user input.
- **Windows Support**: Currently, only PowerShell is supported as the interface on Windows systems.

## Installation

To compile the program, execute the following commands:

```
gcc -c base64.c -o base.o
gcc file_upload.c base64.o -o file_upload
```

## Usage

The program offers a simple interface for infiltrating files through a reverse shell. Here are some basic commands for usage:

- To display help, execute:

```
./file_upload -h
```

- Example usage with a specific tty connection (e.g., /dev/pts/3):

```
./file_upload /dev/pts/3
```

## Note on tty Connection Usage

To find the correct tty connection for the reverse shell, run the following command in the terminal:

```
tty
```

This command will display the current terminal, which can then be used for file transfer.

## Current Development Status

This tool is still under development. As of now, only file infiltration is possible. File exfiltration will be supported in future versions.
License

This project is licensed under the MIT License. For more details, see the LICENSE file.

## Acknowledgements

This project uses the base64 library code from [joedf's base64.c repository](https://github.com/joedf/base64.c)
