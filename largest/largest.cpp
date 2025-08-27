/**
 * @file largest.cpp
 * @brief List the largest files in a specified directory and its subdirectories.
 *
 * The "largest" tool lists the largest files in a directory and its subdirectories.
 * It provides options for customizing the file size analysis, including the number
 * of files to list, the depth of subdirectories to consider, file mask filtering,
 * progress feedback, and output format.
 */

#include <Windows.h>
#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <queue>
#include <regex>
#include <sstream>
#include <vector>
#include <chrono>

namespace fs = std::filesystem;

/**
 * @brief Comparator function for max-heap (largest size at top).
 */
struct FileEntry {
    fs::directory_entry entry;
    uintmax_t size;
    bool operator<(const FileEntry& other) const { return size < other.size; } // Max-heap
};

/**
 * @brief Initialize console for UTF-8 output on Windows
 */
void initConsoleUTF8() {
    // Set console code page to UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    // Enable virtual terminal processing for better ANSI support
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
    
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    if (hIn != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hIn, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
            SetConsoleMode(hIn, dwMode);
        }
    }
}

/**
 * @brief Format number with thousands separators.
 */
std::string formatNumber(size_t number) {
    std::string numStr = std::to_string(number);
    if (numStr.length() <= 3) {
        return numStr;
    }
    
    std::string result;
    int count = 0;
    for (int i = static_cast<int>(numStr.length()) - 1; i >= 0; --i) {
        if (count > 0 && count % 3 == 0) {
            result = '.' + result; // thousands separator
        }
        result = numStr[i] + result;
        count++;
    }
    return result;
}

/**
 * @brief Check if filename matches the filemask using wildcard patterns.
 */
bool matchesFileMask(const std::string& filename, const std::string& mask) {
    if (mask == "*") return true;
    
    // Convert Windows-style wildcards to regex
    std::string pattern = mask;
    // Escape special regex characters except * and ?
    pattern = std::regex_replace(pattern, std::regex(R"([.^$|()\[\]{}\\])"), R"(\$&)");
    // Convert wildcards to regex
    pattern = std::regex_replace(pattern, std::regex(R"(\*)"), ".*");
    pattern = std::regex_replace(pattern, std::regex(R"(\?)"), ".");
    pattern = "^" + pattern + "$";
    
    try {
        return std::regex_match(filename, std::regex(pattern, std::regex_constants::icase));
    } catch (const std::regex_error&) {
        // Fallback to simple matching
        return filename.find(mask) != std::string::npos;
    }
}

/**
 * @brief Format file size with thousands separators and right-align the numbers.
 */
std::string formatFileSize(uintmax_t size) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(0);
    if (size < 1000) {
        oss << std::setw(3) << std::right << size << " bytes";
    } else {
        const char* suffixes[] = {" BY", " KB", " MB", " GB", " TB", " PB", " EB", " ZB", " YB"};
        int suffixIndex = 0;
        double scaledSize = static_cast<double>(size);
        while (scaledSize >= 1000.0 && suffixIndex < sizeof(suffixes) / sizeof(suffixes[0]) - 1) {
            scaledSize /= 1000.0;
            suffixIndex++;
        }
        oss << std::setw(3) << std::right << static_cast<uintmax_t>(scaledSize) << suffixes[suffixIndex];
    }
    return oss.str();
}

/**
 * @brief Clear the current line properly.
 */
void clearLine() {
    std::cout << "\r" << std::string(120, ' ') << "\r" << std::flush;
}

/**
 * @brief List the largest files in the specified directory and its subdirectories.
 */
void listLargestFiles(const fs::path& path, const std::string& fileMask = "*",
                     int depth = -1, int numFiles = 50, bool bare = false,
                     bool relative = false, bool showProgress = false, bool verbose = false) {
    std::priority_queue<FileEntry> heap; // Max-heap for top N files
    size_t fileCount = 0;
    size_t inaccessibleCount = 0;
    int maxDepth = 0;

    if (showProgress) {
        std::cout << "\033[?25l"; // Hide cursor
    }

    try {
        auto lastUpdate = std::chrono::steady_clock::now();
        
        // Use a manual approach to handle errors gracefully
        std::vector<fs::path> directoriesToProcess;
        directoriesToProcess.push_back(path);
        
        while (!directoriesToProcess.empty()) {
            fs::path currentDir = directoriesToProcess.back();
            directoriesToProcess.pop_back();
            
            // Calculate current depth
            int currentDepth = 0;
            if (currentDir != path) {
                try {
                    auto relPath = fs::relative(currentDir, path);
                    currentDepth = static_cast<int>(std::distance(relPath.begin(), relPath.end()));
                } catch (...) {
                    currentDepth = 0; // Fallback
                }
            }
            
            if (depth != -1 && currentDepth > depth) {
                continue;
            }
            
            maxDepth = std::max(maxDepth, currentDepth);
            
            try {
                // Try to iterate through the directory
                for (const auto& entry : fs::directory_iterator(currentDir, fs::directory_options::skip_permission_denied)) {
                    try {
                        if (fs::is_directory(entry)) {
                            // Add subdirectory to processing queue
                            directoriesToProcess.push_back(entry.path());
                        } else if (fs::is_regular_file(entry) && matchesFileMask(entry.path().filename().string(), fileMask)) {
                            fileCount++;
                            uintmax_t size = fs::file_size(entry);
                            
                            if (numFiles == -1) {
                                heap.push({entry, size});
                            } else if (heap.size() < static_cast<size_t>(numFiles)) {
                                heap.push({entry, size});
                            } else if (size > heap.top().size) {
                                heap.pop();
                                heap.push({entry, size});
                            }

                            if (showProgress) {
                                auto now = std::chrono::steady_clock::now();
                                if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate).count() >= 100) {
                                    clearLine();
                                    std::ostringstream oss;
                                    oss << "Files: " << formatNumber(fileCount)
                                        << " | Depth: " << std::setw(2) << std::setfill(' ') << currentDepth
                                        << " | Max Depth: " << std::setw(2) << std::setfill(' ') << maxDepth;
                                    
                                    if (inaccessibleCount > 0) {
                                        oss << " | Inaccessible: " << inaccessibleCount;
                                    }
                                    
                                    std::cout << "\r" << oss.str() << std::flush;
                                    lastUpdate = now;
                                }
                            }
                        }
                    } catch (const fs::filesystem_error& e) {
                        inaccessibleCount++;
                        if (verbose) {
                            clearLine();
                            std::cerr << "Inaccessible file/directory skipped: " << entry.path().string() << std::endl;
                        }
                    }
                }
            } catch (const fs::filesystem_error& e) {
                inaccessibleCount++;
                if (verbose) {
                    clearLine();
                    std::cerr << "Inaccessible directory skipped: " << currentDir.string() << std::endl;
                }
                
                if (showProgress) {
                    auto now = std::chrono::steady_clock::now();
                    clearLine();
                    std::ostringstream oss;
                    oss << "Files: " << formatNumber(fileCount)
                        << " | Depth: " << std::setw(2) << std::setfill(' ') << currentDepth
                        << " | Max Depth: " << std::setw(2) << std::setfill(' ') << maxDepth
                        << " | Inaccessible: " << inaccessibleCount;
                    
                    std::cout << "\r" << oss.str() << std::flush;
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        if (verbose) {
            std::cerr << "Error accessing root directory: " << e.what() << std::endl;
        }
    }

    if (showProgress) {
        // Clear the progress line
        clearLine();
        std::cout << "\033[?25h"; // Show cursor
    }

    // Convert heap to vector for sorted output (largest first - already sorted due to max-heap)
    std::vector<FileEntry> files;
    while (!heap.empty()) {
        files.push_back(heap.top());
        heap.pop();
    }
    // No need to reverse since we're using a max-heap

    // Display the largest files
    int count = 0;
    for (const auto& entry : files) {
        if (numFiles == -1 || count < numFiles) {
            std::string filePath = relative ? fs::relative(entry.entry.path(), path).string() : entry.entry.path().string();
            if (bare) {
                std::cout << filePath << "\n";
            } else {
                std::cout << formatFileSize(entry.size) << " " << filePath << "\n";
            }
            count++;
        }
    }
    
    if (verbose && inaccessibleCount > 0) {
        std::cerr << "Skipped " << inaccessibleCount << " inaccessible file(s)/directorie(s)." << std::endl;
    }
}

int main(int argc, char* argv[]) {
    // Initialize console for UTF-8 output
    initConsoleUTF8();
    
    int numFiles = 50;
    int depth = -1; // Infinite depth by default
    std::string fileMask = "*";
    bool bare = false;
    bool relative = false;
    bool showProgress = false;
    bool verbose = false;
    fs::path targetPath = fs::current_path();

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        try {
            if (arg == "-n") {
                if (++i < argc) numFiles = std::stoi(argv[i]);
                if (numFiles < -1) numFiles = 50; // Reset to default
            } else if (arg == "-d") {
                if (++i < argc) depth = std::stoi(argv[i]);
                if (depth < -1) depth = -1; // Reset to default
            } else if (arg == "-b") {
                bare = true;
            } else if (arg == "-r") {
                relative = true;
            } else if (arg == "-p") {
                showProgress = true;
            } else if (arg == "-v" || arg == "--verbose") {
                verbose = true;
            } else if (arg == "-h") {
                std::cout << "Usage: largest [-n num] [-d num] [-b] [-r] [-p] [-v] [directory] [filemask]\n"
                          << "Options:\n"
                          << "  -n num    : Number of largest files to list (default: 50, -1 for all)\n"
                          << "  -d num    : Depth of subdirectories to consider (default: -1, infinite)\n"
                          << "  -b        : Display only file paths without file sizes\n"
                          << "  -r        : Display relative paths\n"
                          << "  -p        : Show progress (file count, current depth, max depth)\n"
                          << "  -v        : Verbose mode (show inaccessible files/directories)\n"
                          << "  directory : Directory to scan (default: current directory)\n"
                          << "  filemask  : File mask to filter files (e.g., *.txt, default: *)\n"
                          << "  -h        : Display this help text\n"
                          << "\nExamples:\n"
                          << "  largest -n 10 -d 2 *.log\n"
                          << "  largest C:\\Windows -n 20 *.dll\n"
                          << "  largest -b -r -p\n";
                return 0;
            } else {
                // Check if this is a directory or filemask
                fs::path potentialPath(arg);
                if (fs::exists(potentialPath) && fs::is_directory(potentialPath)) {
                    targetPath = potentialPath;
                } else {
                    fileMask = arg;
                }
            }
        } catch (const std::exception& e) {
            if (verbose) {
                std::cerr << "Invalid argument: " << arg << " (" << e.what() << ")" << std::endl;
            }
            // Continue processing other arguments
        }
    }

    // Validate target path
    if (!fs::exists(targetPath)) {
        std::cerr << "Error: Directory does not exist: " << targetPath.string() << std::endl;
        return 1;
    }
    
    if (!fs::is_directory(targetPath)) {
        std::cerr << "Error: Specified path is not a directory: " << targetPath.string() << std::endl;
        return 1;
    }

    // List the largest files
    listLargestFiles(targetPath, fileMask, depth, numFiles, bare, relative, showProgress, verbose);

    return 0;
}