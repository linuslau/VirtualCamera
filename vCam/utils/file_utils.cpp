/* Copyright(c), 2024, linuslau (liukezhao@gmail.com) */

#include "file_utils.h"   // NOLINT(build/include_subdir)

#include <windows.h>

#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

std::string GetExecutablePath() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string exe_path(buffer);
    size_t last_slash_idx = exe_path.rfind('\\');
    return exe_path.substr(0, last_slash_idx);
}

std::string validate_media_path(const std::string& media_type,
                                const std::string& media_path) {
    std::cout << std::endl << "2. Validate media path:" << std::endl;

    auto work_path = std::filesystem::current_path();
    std::cout << "vCam  work path: " << work_path << std::endl;

    std::cout << "Media file path: " << media_path << std::endl;

    if (media_type == "-v") {
        if (!fs::exists(media_path) || !fs::is_regular_file(media_path)) {
            std::cerr << "Invalid media_file path." << std::endl;
            return "";
        }
    } else if (media_type == "-i") {
        if (!fs::exists(media_path) || !fs::is_directory(media_path)) {
            std::cerr << "Invalid media_file path." << std::endl;
            return "";
        }
    } else {
        std::cerr << "Invalid input type: " << media_type << std::endl;
        return "";
    }

    return media_path;
}

