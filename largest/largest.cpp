#include <iostream>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <iomanip> // for std::setw
#include <sstream>

namespace fs = std::filesystem;

/**
 * @brief Comparator function to sort files by size in descending order.
 */
bool sortBySize(const fs::directory_entry& a, const fs::directory_entry& b) {
    return fs::file_size(a) > fs::file_size(b);
}

/**
 * @brief Format file size with thousands separators and right-align the numbers.
 */
std::string formatFileSize(uintmax_t size) {
    std::ostringstream oss;

    // Format the file size with thousands separators and right-align
    oss << std::fixed << std::setprecision(0);
    if (size < 1000) {
        oss << std::setw(3) << std::right << size << " bytes";
    } else {
        const char* suffixes[] = {" KB", " MB", " GB", " TB", " PB", " EB", " ZB", " YB"};
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
 *
 * @param path The directory path to start the search.
 * @param fileMask The file mask to filter files (default is "*").
 * @param depth The maximum depth of subdirectories to consider (default is -1, indicating infinite depth).
 * @param numFiles The number of largest files to display (default is 50, use -1 to display all files).
 * @param bare Flag indicating whether to display only file paths without file sizes (default is false).
 * @param relative Flag indicating whether to display relative paths (default is false).
 */
void listLargestFiles(const fs::path& path, const std::string& fileMask = "*", int depth = -1, int numFiles = 50, bool bare = false, bool relative = false) {
    std::vector<fs::directory_entry> files;

    // Recursive directory iterator
    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        if (fs::is_regular_file(entry) && (fileMask == "*" || entry.path().filename().string().find(fileMask) != std::string::npos)) {
            files.push_back(entry);
        }
    }

    // Sort files by size
    std::sort(files.begin(), files.end(), sortBySize);

    // Display the largest files
    int count = 0;
    for (const auto& entry : files) {
        if (numFiles == -1 || count < numFiles) {
            std::string filePath = relative ? fs::relative(entry.path(), path).string() : entry.path().string();

            if (bare) {
                std::cout << filePath << "\n";
            } else {
                std::cout << formatFileSize(fs::file_size(entry)) << " " << filePath << "\n";
            }
            count++;
        } else {
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    int numFiles = 50;
    int depth = -1; // Infinite depth by default
    std::string fileMask = "*";
    bool bare = false;
    bool relative = false;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-n") {
            if (i + 1 < argc) {
                numFiles = std::stoi(argv[i + 1]);
                if (numFiles < -1) {
                    numFiles = 50; // Default value
                }
                i++; // Skip the next argument (number of files)
            }
        } else if (arg == "-d") {
            if (i + 1 < argc) {
                depth = std::stoi(argv[i + 1]);
                if (depth < -1) {
                    depth = -1; // Infinite depth by default
                }
                i++; // Skip the next argument (depth)
            }
        } else if (arg == "-b") {
            bare = true;
        } else if (arg == "-r") {
            relative = true;
        } else if (arg == "-h") {
            // Display help text and exit
            std::cout << "Usage: largest -n num -d num -b -r filemask\n"
                      << "Options:\n"
                      << "  -n num    : Number of largest files to list (default: 50, use -1 to list all files)\n"
                      << "  -d num    : Depth of subdirectories to consider (default: -1, infinite depth)\n"
                      << "  -b        : Display only file paths without file sizes\n"
                      << "  -r        : Display relative paths\n"
                      << "  filemask  : File mask to filter files (default: *)\n"
                      << "  -h        : Display this help text\n";
            return 0;
        } else {
            fileMask = arg;
        }
    }

    // Get the current working directory
    fs::path currentPath = fs::current_path();

    // List the largest files
    listLargestFiles(currentPath, fileMask, depth, numFiles, bare, relative);

    return 0;
}
