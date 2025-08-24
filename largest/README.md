# Largest

The "largest" tool is a simple C++ command-line utility designed to list the largest files in a specified directory and its subdirectories. This tool is particularly useful for identifying and displaying the largest files based on their sizes, with options to filter files, limit directory depth, and show progress during the search.

## Overview

When dealing with a large number of files, it can be beneficial to quickly identify and analyze the largest ones. The "largest" tool provides a convenient way to achieve this, allowing users to specify a target directory, file mask, directory depth, and other options for customized file size analysis.

## Usage

```plaintext
largest [-n num] [-d num] [-b] [-r] [-p] [filemask]
```

### Options:
```plaintext
  -n num    : Number of largest files to list (default: 50, use -1 to list all files)
  -d num    : Depth of subdirectories to consider (default: -1, infinite depth)
  -b        : Display only file paths without file sizes
  -r        : Display relative paths
  -p        : Show progress with file count, current depth, and max depth
  filemask  : File mask to filter files (e.g., *.txt, default: *)
  -h        : Display this help text
```

## Examples
```plaintext
largest                    // List the 50 largest files in the current directory and its subdirectories
largest -n 10              // List the top 10 largest files
largest -n -1 -d 2         // List all files up to a depth of 2 subdirectories
largest -b *lord*.mkv      // List file paths of files matching the specified mask
largest -n 20 -b -d 3      // List the top 20 largest file paths up to a depth of 3 subdirectories
largest -p *.txt           // List the 50 largest text files with progress (file count, depth, max depth)
largest -p -n 10 -d 1      // List the top 10 largest files in the current directory and immediate subdirectories with progress
```

## Notes
* The tool sorts files by size in descending order, displaying the largest files first.
* File sizes are formatted for better readability, including thousands separators (e.g., "10 MB").
* The `-p` option displays a progress line with the number of files scanned, current directory depth, and maximum depth reached (formatted as two digits, e.g., "Depth: 05").
* File masks support wildcard patterns (e.g., `*.txt` for text files, `file?.log` for files with a single character before ".log").
* The tool efficiently handles large directories by using a min-heap to store only the largest files, reducing memory usage.

## Build
To build the "largest" tool, use the following command:

```plaintext
g++ -std=c++17 largest.cpp -o largest
```