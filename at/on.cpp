/**
 * @file
 * @brief Simple program to execute a command at a specified time.
 *
 * Usage: program_name Time Command
 * - Time should be in the format hh:mm or hh:mm:ss.
 * - Example: program_name 12:30 ls
 */

#include <iostream>
#include <string>
#include <ctime>
#include <chrono>
#include <thread>

int main(int argc, char *argv[]) {
    // Check the number of arguments
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " Time Command" << std::endl;
        return 1;
    }

    // Extract arguments
    std::string timeArgument = argv[1];
    std::string command = argv[2];

    // Get the current time
    std::time_t currentTime = std::time(nullptr);
    struct tm *now = std::localtime(&currentTime);

    // Convert the input time to a specific time
    int hh, mm, ss;
    if (sscanf(timeArgument.c_str(), "%d:%d:%d", &hh, &mm, &ss) < 2) {
        if (sscanf(timeArgument.c_str(), "%d:%d", &hh, &mm) < 1) {
            std::cerr << "Invalid time format. Use hh:mm or hh:mm:ss." << std::endl;
            return 1;
        }
    }

    // Calculate seconds until the specified time
    int targetSeconds = hh * 3600 + mm * 60 + ss;

    // Calculate seconds until the next specified time
    int currentSeconds = now->tm_hour * 3600 + now->tm_min * 60 + now->tm_sec;
    int timeDifference = targetSeconds - currentSeconds;
    if (timeDifference < 0) {
        // Time has already passed, so shift to the next day
        timeDifference += 24 * 3600;
    }

    // Wait until the specified time
    std::this_thread::sleep_for(std::chrono::seconds(timeDifference));

    // Execute the command
    std::system(command.c_str());

    return 0;
}
