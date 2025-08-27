/**
 * @file
 * @brief Simple program to display a countdown or progress bar and optionally execute a command at a specified time or after a delay.
 *
 * Usage: on [options] Time [Command [CommandArgs...]]
 * - Time should be in the format hh, hh:mm, or hh:mm:ss (for clock time).
 * - For delay mode: format hh:mm:ss, mm:ss, or ss. The leading unit can exceed standard limits and will be normalized (e.g., 90 becomes 1:30; 120:00 becomes 2:00:00; 26:00:00 becomes 26:00:00). Subsequent units must be 0-59.
 * - Default output: countdown timer, updating in the same line.
 * - If no Command is provided, the program only displays the countdown or progress bar.
 * Options:
 *   -h, --help            Show this help message
 *   -d, --delay           Interpret Time as duration instead of clock time
 *   -c, --no-clear        Disable in-place countdown (print new line each update)
 *   -o, --output=MODE     Set output mode: time, progress, both, none (or t, p, b, n)
 *   -l, --length=NUM      Set progress bar length (default: 50, min: 5, max: 300)
 * Examples:
 *   on 12:30 dir /b              (executes at 12:30 with countdown)
 *   on -d 20 dir                 (executes after 20 seconds with countdown)
 *   on -o p 21:30                (shows progress bar until 21:30, no command)
 *   on -o n 12:30 dir /b         (executes at 12:30 with no output)
 */

#include <iostream>
#include <string>
#include <ctime>
#include <chrono>
#include <thread>
#include <iomanip>
#include <sstream>

// Compile-time constants
const std::string DEFAULT_OUTPUT = "time"; // Options: "time", "progress", "both", "none"
const int PROGRESS_BAR_LENGTH = 50; // Default length of the progress bar in characters
const std::string SPINNER_CHARS = "|/-\\"; // Spinner sequence for progress bar
const char FILL_CHAR = '.'; // Fill character for uncompleted progress bar
const int UPDATE_INTERVAL_MS = 125; // Update interval for progress bar in milliseconds (8 updates/sec)

int main(int argc, char *argv[]) {
    bool showHelp = false;
    bool delayMode = false;
    bool noClear = false;
    std::string outputMode = DEFAULT_OUTPUT;
    int progressBarLength = PROGRESS_BAR_LENGTH; // Default progress bar length
    int argIndex = 1; // Start after program name

    // Parse options
    while (argIndex < argc && argv[argIndex][0] == '-') {
        std::string option = argv[argIndex];
        if (option == "-h" || option == "--help") {
            showHelp = true;
        } else if (option == "-d" || option == "--delay") {
            delayMode = true;
        } else if (option == "-c" || option == "--no-clear") {
            noClear = true;
        } else if (option == "-o" || option == "--output") {
            if (++argIndex >= argc) {
                std::cerr << "Option " << option << " requires a value: time, progress, both, none" << std::endl;
                return 1;
            }
            outputMode = argv[argIndex];
            if (outputMode != "time" && outputMode != "progress" && outputMode != "both" && outputMode != "none" &&
                outputMode != "t" && outputMode != "p" && outputMode != "b" && outputMode != "n") {
                std::cerr << "Invalid output mode: " << outputMode << ". Use: time, progress, both, none (or t, p, b, n)" << std::endl;
                return 1;
            }
        } else if (option == "-l" || option == "--length") {
            if (++argIndex >= argc) {
                std::cerr << "Option " << option << " requires a numeric value" << std::endl;
                return 1;
            }
            try {
                progressBarLength = std::stoi(argv[argIndex]);
                if (progressBarLength < 5 || progressBarLength > 300) {
                    std::cerr << "Progress bar length must be between 5 and 300" << std::endl;
                    return 1;
                }
            } catch (const std::exception&) {
                std::cerr << "Invalid progress bar length: " << argv[argIndex] << ". Must be a number." << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Unknown option: " << option << std::endl;
            return 1;
        }
        ++argIndex;
    }

    // If help is requested, show it and exit
    if (showHelp) {
        std::cout << "Usage: " << argv[0] << " [options] Time [Command [CommandArgs...]]" << std::endl;
        std::cout << "\nTime format: hh, hh:mm, or hh:mm:ss (for clock time)" << std::endl;
        std::cout << "For delay mode: hh:mm:ss, mm:ss, or ss." << std::endl;
        std::cout << "  The leading unit can exceed standard limits and will be normalized" << std::endl;
        std::cout << "  (e.g., 90 becomes 1:30; 120:00 becomes 2:00:00; 26:00:00 becomes 26:00:00). Subsequent units must be 0-59." << std::endl;
        std::cout << "Default output: countdown timer, updating in the same line." << std::endl;
        std::cout << "If no Command is provided, the program only displays the countdown or progress bar." << std::endl;
        std::cout << "\nOptions:" << std::endl;
        std::cout << "  -h, --help            Show this help message" << std::endl;
        std::cout << "  -d, --delay           Interpret Time as duration instead of clock time" << std::endl;
        std::cout << "  -c, --no-clear        Disable in-place countdown (print new line each update)" << std::endl;
        std::cout << "  -o, --output=MODE     Set output mode: time, progress, both, none (or t, p, b, n)" << std::endl;
        std::cout << "  -l, --length=NUM      Set progress bar length (default: 50, min: 5, max: 300)" << std::endl;
        std::cout << "\nExamples:" << std::endl;
        std::cout << "  on 12:30 dir /b              (executes at 12:30 with countdown)" << std::endl;
        std::cout << "  on -d 20 dir                 (executes after 20 seconds with countdown)" << std::endl;
        std::cout << "  on -o p 21:30                (shows progress bar until 21:30, no command)" << std::endl;
        std::cout << "  on -o n 12:30 dir /b         (executes at 12:30 with no output)" << std::endl;
        return 0;
    }

    // Check remaining arguments: at least Time is required
    if (argIndex >= argc) {
        std::cerr << "Usage: " << argv[0] << " [options] Time [Command [CommandArgs...]]" << std::endl;
        std::cerr << "Use '" << argv[0] << " --help' for more information." << std::endl;
        return 1;
    }

    // Extract time/duration argument
    std::string timeArgument = argv[argIndex];
    ++argIndex;

    // Combine all remaining arguments into a command string, if any
    std::string command;
    for (int i = argIndex; i < argc; ++i) {
        if (!command.empty()) {
            command += " ";
        }
        command += argv[i];
    }

    // Parse the time/duration value
    int hh = 0, mm = 0, ss = 0;
    int fields = sscanf(timeArgument.c_str(), "%d:%d:%d", &hh, &mm, &ss);
    if (fields < 1) {
        std::cerr << "Invalid time/duration format." << std::endl;
        return 1;
    }
    int original_fields = fields; // Store original number of fields for validation
    if (fields == 1) {
        // Only one value: for delay mode, treat as seconds; for time mode, as hours
        if (delayMode) {
            ss = hh;
            hh = 0;
            mm = 0;
        } else {
            mm = 0;
            ss = 0;
        }
    } else if (fields == 2) {
        // Two values: hh:mm for time, or mm:ss for delay
        if (delayMode) {
            ss = mm;  // mm becomes seconds
            mm = hh;  // hh becomes minutes
            hh = 0;   // hours set to 0
        } else {
            ss = 0;   // seconds set to 0 for time mode
        }
    }

    // Validate values: negative not allowed; in non-delay, strict bounds; in delay, bounds depend on original fields
    if (hh < 0 || mm < 0 || ss < 0) {
        std::cerr << "Invalid values. All components must be non-negative." << std::endl;
        return 1;
    }
    if (!delayMode) {
        if (hh > 23 || mm > 59 || ss > 59) {
            std::cerr << "Invalid values. Hours: 0-23, Minutes/Seconds: 0-59." << std::endl;
            return 1;
        }
    } else {
        // In delay mode, allow overflow only in the leading unit
        if (original_fields == 2) {
            if (ss > 59) {
                std::cerr << "Invalid values. Seconds must be 0-59." << std::endl;
                return 1;
            }
        } else if (original_fields == 3) {
            if (mm > 59 || ss > 59) {
                std::cerr << "Invalid values. Minutes/Seconds must be 0-59." << std::endl;
                return 1;
            }
        }
        // For original_fields == 1, no upper bounds checks (seconds can overflow)
    }

    // Calculate seconds
    long long targetSeconds = static_cast<long long>(hh) * 3600 + mm * 60 + ss; // Use long long to handle large values

    long long timeDifference;
    if (delayMode) {
        // In delay mode, wait exactly the duration
        timeDifference = targetSeconds;
        if (timeDifference <= 0) {
            std::cerr << "Duration must be positive." << std::endl;
            return 1;
        }
    } else {
        // In time mode, calculate difference to next occurrence
        std::time_t currentTime = std::time(nullptr);
        struct tm *now = std::localtime(&currentTime);
        int currentSeconds = now->tm_hour * 3600 + now->tm_min * 60 + now->tm_sec;
        timeDifference = targetSeconds - currentSeconds;
        if (timeDifference < 0) {
            timeDifference += 24 * 3600;
        }
    }

    // Determine output behavior based on DEFAULT_OUTPUT and options
    bool showTimer = (outputMode == "time" || outputMode == "t" || outputMode == "both" || outputMode == "b");
    bool showBar = (outputMode == "progress" || outputMode == "p" || outputMode == "both" || outputMode == "b");
    if (outputMode.empty()) { // If no --output option was provided, use DEFAULT_OUTPUT
        showTimer = (DEFAULT_OUTPUT == "time" || DEFAULT_OUTPUT == "both");
        showBar = (DEFAULT_OUTPUT == "progress" || DEFAULT_OUTPUT == "both");
    }

    // Wait with countdown timer and/or progress bar
    if (showTimer || showBar) {
        using clock = std::chrono::steady_clock;
        auto start = clock::now();
        auto end = start + std::chrono::seconds(timeDifference);
        int tick = 0; // Counter for spinner sequence
        if (noClear) {
            // Print new line each update
            while (clock::now() < end) {
                auto now = clock::now();
                auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
                auto remainingSec = std::chrono::duration_cast<std::chrono::seconds>(end - now).count();
                double progressRatio = static_cast<double>(elapsedMs) / (timeDifference * 1000.0);
                if (progressRatio > 1.0) progressRatio = 1.0;
                int progress = static_cast<int>(progressRatio * progressBarLength);
                int hours = static_cast<int>(remainingSec / 3600);
                int minutes = static_cast<int>(remainingSec % 3600 / 60);
                int seconds = static_cast<int>(remainingSec % 60);
                std::stringstream ss;
                ss << std::setfill('0') << std::setw(2) << hours << ":"
                   << std::setw(2) << minutes << ":" << std::setw(2) << seconds;
                std::string output;
                if (showBar) {
                    std::string bar = std::string(progress, '#');
                    if (progress < progressBarLength) {
                        bar += SPINNER_CHARS[tick % SPINNER_CHARS.size()];
                        bar += std::string(progressBarLength - progress - 1, FILL_CHAR);
                    } else {
                        bar += std::string(progressBarLength - progress, FILL_CHAR);
                    }
                    output += "[" + bar + "] ";
                }
                if (showTimer) {
                    output += "Remaining: " + ss.str();
                }
                std::cout << output << std::endl;
                // Use different update intervals based on output mode
                if (showBar) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(UPDATE_INTERVAL_MS));
                } else {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
                tick++;
            }
        } else {
            // Update in-place with \r, hide cursor
            std::cout << "\033[?25l"; // Hide cursor
            while (clock::now() < end) {
                auto now = clock::now();
                auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
                auto remainingSec = std::chrono::duration_cast<std::chrono::seconds>(end - now).count();
                double progressRatio = static_cast<double>(elapsedMs) / (timeDifference * 1000.0);
                if (progressRatio > 1.0) progressRatio = 1.0;
                int progress = static_cast<int>(progressRatio * progressBarLength);
                int hours = static_cast<int>(remainingSec / 3600);
                int minutes = static_cast<int>(remainingSec % 3600 / 60);
                int seconds = static_cast<int>(remainingSec % 60);
                std::stringstream ss;
                ss << std::setfill('0') << std::setw(2) << hours << ":"
                   << std::setw(2) << minutes << ":" << std::setw(2) << seconds;
                std::string output;
                if (showBar) {
                    std::string bar = std::string(progress, '#');
                    if (progress < progressBarLength) {
                        bar += SPINNER_CHARS[tick % SPINNER_CHARS.size()];
                        bar += std::string(progressBarLength - progress - 1, FILL_CHAR);
                    } else {
                        bar += std::string(progressBarLength - progress, FILL_CHAR);
                    }
                    output += "[" + bar + "] ";
                }
                if (showTimer) {
                    output += "Remaining: " + ss.str();
                }
                std::cout << "\r" << output << std::flush;
                // Use different update intervals based on output mode
                if (showBar) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(UPDATE_INTERVAL_MS));
                } else {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
                tick++;
            }
            // Clear the line and show cursor again
            std::cout << "\r" << std::string(progressBarLength + 22, ' ') << "\r\033[?25h" << std::flush;
        }
    } else {
        // No output, just wait
        std::this_thread::sleep_for(std::chrono::seconds(timeDifference));
    }

    // Execute the command if provided
    if (!command.empty()) {
        return std::system(command.c_str());
    }
    return 0;
}