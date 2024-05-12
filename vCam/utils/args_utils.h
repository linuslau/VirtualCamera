/* Copyright(c), 2024, linuslau (liukezhao@gmail.com) */
// args_utils.h

#pragma once

#ifndef ARGS_UTILS_H
#define ARGS_UTILS_H

#include <string>

// Function to parse command line arguments
bool parseArguments(int argc, char* argv[],  // NOLINT(runtime/references)
                    std::string& media_type,  // NOLINT(runtime/references)
                    std::string& media_path,  // NOLINT(runtime/references)
                    bool& loop,  // NOLINT(runtime/references)
                    bool& cv_log);  // NOLINT(runtime/references)

// Function to print usage information
void print_usage(const char* programName);

#endif  // ARGS_UTILS_H
