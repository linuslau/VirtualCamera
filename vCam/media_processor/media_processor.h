/* Copyright(c), 2024, linuslau (liukezhao@gmail.com) */
// media_processor.h

#pragma once

#ifndef VIDEO_PROCESSING_H
#define VIDEO_PROCESSING_H

#include <string>

typedef void (*ProducerFunction)(const std::string&);

void producer_video(const std::string& video_file);
void producer_image(const std::string& directory);
void consumer();
int  start_media_processing(const std::string& input_type,
                            const std::string& input_path,
                            bool loop,
                            bool detailed_logging);

#endif  // VIDEO_PROCESSING_H
