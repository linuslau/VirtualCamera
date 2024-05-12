/* Copyright(c), 2024, linuslau (liukezhao@gmail.com) */
// file_utils.h

#pragma once

#ifndef FILE_UTILS_HPP
#define FILE_UTILS_HPP

#include <string>

std::string GetExecutablePath();
std::string validate_media_path(const std::string& media_type,
                                const std::string& media_path);

#endif  // FILE_UTILS_HPP

