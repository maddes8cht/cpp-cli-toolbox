/**
 * @file largest.cpp
 * @brief List the largest files in a specified directory and its subdirectories.
 *
 * The "largest" tool lists the largest files in a directory and its subdirectories.
 * It provides options for customizing the file size analysis, including the number
 * of files to list, the depth of subdirectories to consider, file mask filtering,
 * progress feedback, and output format.
 *
 * @note Usage:
 *     largest [-n num] [-d num] [-b] [-r] [-p] [filemask]
 *
 * @note Options:
 *   -n num    : Number of largest files to list (default: 50, -1 for all)
 *   -d num    : Depth of subdirectories to consider (default: -1, infinite)
 *   -b        : Display only file paths without file sizes
 *   -r        : Display relative paths
 *   -p        : Show progress (file count, current depth, max depth)
 *   filemask  : File mask to filter files (e.g., *.txt, default: *)
 *   -h        : Display this help text
 */

#include <Windows.h>
#include <algorithm>
#include <codecvt> // for std::codecvt_utf8
#include <filesystem>
#include <iomanip> // for std::setw, std::setfill
#include <iostream>
#include <locale> // for std::locale, std::wcout
#include <queue>
#include <regex>
#include <sstream>
#include <vector>
#include <chrono>

namespace fs = std::filesystem;

/**
 * @brief Comparator function for min-heap (smallest size at top).
 */
struct FileEntry {
    fs::directory_entry entry;
    uintmax_t size;
    bool operator<(const FileEntry& other) const { return size > other.size; } // Min-heap
};

/**
 * @brief Check if filename matches the filemask using wildcard patterns.
 */
bool matchesFileMask(const std::string& filename, const std::string& mask) {
    if (mask == "*") return true;
    // Convert wildcard to regex (e.g., "*.txt" -> ".*\.txt$")
    std::string regexPattern = std::regex_replace(mask, std::regex("\\*"), ".*");
    regexPattern = std::regex_replace(regexPattern, std::regex("\\?"), ".");
    try {
        return std::regex_match(filename, std::regex(regexPattern));
    } catch (const std::regex_error&) {
        return filename.find(mask) != std::string::npos; // Fallback to substring matching
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
        while (size >= 1000 && suffixIndex < sizeof(suffixes) / sizeof(suffixes[0])) {
            size /= 1000;
            suffixIndex++;
        }
        oss << std::setw(3) << std::right << size << suffixes[suffixIndex];
    }
    return oss.str();
}

/**
 * @brief List the largest files in the specified directory and its subdirectories.
 */
void listLargestFiles(const fs::path& path, const std::string& fileMask = "*",
                     int depth = -1, int numFiles = 50, bool bare = false,
                     bool relative = false, bool showProgress = false) {
    std::priority_queue<FileEntry> heap; // Min-heap for top N files
    size_t fileCount = 0;
    int maxDepth = 0;

    if (showProgress) {
        std::cout << "\033[?25l"; // Hide cursor
    }

    try {
        auto lastUpdate = std::chrono::steady_clock::now();
        for (auto it = fs::recursive_directory_iterator(path, fs::directory_options::skip_permission_denied);
             it != fs::recursive_directory_iterator(); ++it) {
            if (depth != -1 && it.depth() > depth) {
                it.disable_recursion_pending(); // Skip deeper subdirectories
                continue;
            }
            if (fs::is_regular_file(*it) && matchesFileMask(it->path().filename().string(), fileMask)) {
                fileCount++;
                maxDepth = std::max(maxDepth, it.depth());
                uintmax_t size = fs::file_size(*it);
                if (numFiles == -1 || heap.size() < static_cast<size_t>(numFiles)) {
                    heap.push({*it, size});
                } else if (size > heap.top().size) {
                    heap.pop();
                    heap.push({*it, size});
                }

                if (showProgress) {
                    auto now = std::chrono::steady_clock::now();
                    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate).count() >= 100) {
                        std::ostringstream oss;
                        oss << "\rFiles: " << fileCount
                            << " | Depth: " << std::setw(2) << std::setfill('0') << it.depth()
                            << " | Max Depth: " << std::setw(2) << std::setfill('0') << maxDepth
                            << std::string(10, ' '); // Extra padding to ensure full line clear
                        std::cout << oss.str() << std::flush;
                        lastUpdate = now;
                    }
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error accessing file: " << e.what() << std::endl;
    }

    if (showProgress) {
        std::cout << "\r" << std::string(50, ' ') << "\r\033[?25h"; // Clear line, show cursor
    }

    std::cout << "Sorting and displaying files..." << std::endl;

    // Convert heap to vector for sorted output (largest first)
    std::vector<FileEntry> files;
    while (!heap.empty()) {
        files.push_back(heap.top());
        heap.pop();
    }
    std::reverse(files.begin(), files.end()); // Largest to smallest

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
}

int main(int argc, char* argv[]) {
    int numFiles = 50;
    int depth = -1; // Infinite depth by default
    std::string fileMask = "*";
    bool bare = false;
    bool relative = false;
    bool showProgress = false;
    std::locale utf8_locale(std::locale(), new std::codecvt_utf8<wchar_t>);
    std::wcout.imbue(utf8_locale);

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
            } else if (arg == "-h") {
                std::cout << "Usage: largest [-n num] [-d num] [-b] [-r] [-p] [filemask]\n"
                          << "Options:\n"
                          << "  -n num    : Number of largest files to list (default: 50, -1 for all)\n"
                          << "  -d num    : Depth of subdirectories to consider (default: -1, infinite)\n"
                          << "  -b        : Display only file paths without file sizes\n"
                          << "  -r        : Display relative paths\n"
                          << "  -p        : Show progress (file count, current depth, max depth)\n"
                          << "  filemask  : File mask to filter files (e.g., *.txt, default: *)\n"
                          << "  -h        : Display this help text\n";
                return 0;
            } else {
                fileMask = arg;
            }
        } catch (const std::exception& e) {
            std::cerr << "Invalid argument: " << arg << " (" << e.what() << ")" << std::endl;
            return 1;
        }
    }

    // Get the current working directory
    fs::path currentPath = fs::current_path();

    // List the largest files
    listLargestFiles(currentPath, fileMask, depth, numFiles, bare, relative, showProgress);

    return 0;
}