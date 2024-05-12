/* Copyright(c), 2024, linuslau (liukezhao@gmail.com) */

#include "args_utils.h"   // NOLINT(build/include_subdir)

#include <iostream>
#include <algorithm>

bool parseArguments(int argc, char* argv[],
                    std::string& media_type,  // NOLINT(runtime/references)
                    std::string& media_path,  // NOLINT(runtime/references)
                    bool& loop,  // NOLINT(runtime/references)
                    bool& cv_log) {  // NOLINT(runtime/references)
    if (argc < 3 || argc > 5) {
        print_usage(argv[0]);
        return false;
    }

    media_type = argv[1];
    media_path = argv[2];

    // Check if loop argument is provided
    if (argc >= 4) {
        std::string loop_arg = argv[3];
        // Define a lambda function to convert characters to lowercase
        auto toLower = [](unsigned char c) {
            return std::tolower(c);
        };

        // Use std::transform to convert characters in the string to lowercase
        std::transform(loop_arg.begin(),
                       loop_arg.end(),
                       loop_arg.begin(),
                       toLower);

        if (loop_arg == "1" || loop_arg == "true") {
            loop = true;
        } else if (loop_arg == "0" || loop_arg == "false") {
            loop = false;
        } else {
            std::cerr << "Invalid loop argument. Please use '0' or '1'"
                << "for false || true respectively." << std::endl;
            print_usage(argv[0]);
            return false;
        }
    } else {
        // If loop argument is not provided, set default value
        loop = false;
    }

    cv_log = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-d") {
            cv_log = true;
            break;
        }
    }

    return true;
}

void print_usage(const char* programName) {
    std::cerr << "Usage: " << programName << " <-v/-i> <media_path> "
        << "[loop: 0 or 1] [-d]" << std::endl;
    std::cerr << "Arguments:" << std::endl;
    std::cerr << "  -v:           Specify video input." << std::endl;
    std::cerr << "  -i:           Specify image input." << std::endl;
    std::cerr << "  <media_path>: The path to the input directory or "
        << "file." << std::endl;
    std::cerr << "  <loop>:       Whether to loop indefinitely. "
        << "0 for false, 1 for true." << std::endl;
    std::cerr << "  -d:           Enable detailed logging." << std::endl;
    std::cerr << "\nExample:" << std::endl;
    std::cerr << "  vVam.exe -v /path/to/video/video.mp4" << std::endl;
    std::cerr << "  vVam.exe -i /path/to/image" << std::endl;
}
