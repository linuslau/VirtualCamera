/* Copyright(c), 2024, linuslau (liukezhao@gmail.com) */

#include "media_processor.h"  // NOLINT(build/include_subdir)

#include <windows.h>

#include <vector>
#include <string>
#include <iostream>
#include <filesystem>
#include <thread>  // NOLINT(build/c++11)
#include <queue>
#include <mutex>  // NOLINT(build/c++11)
#include <condition_variable>  // NOLINT(build/c++11)

#pragma comment(lib, "winmm.lib")

#include "../utils/dll_utils.h"
#include "../utils/console_utils.h"
#include "../utils/file_utils.h"

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/core/utils/logger.hpp>

#if STB_ENABLED
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"  // NOLINT(build/include_subdir)
#endif

std::queue<cv::Mat> image_queue;
std::mutex image_queue_mtx;
std::condition_variable queueCond;
std::atomic<bool> stop_flag(false);
std::atomic<bool> loop_flag(false);
std::atomic<bool> producer_finished(false);
std::mutex console_mtx;

double fps = 30;
double frame_duration = 1000.0 / 30;

ProducerFunction function_pointer = nullptr;

cv::VideoCapture cap;

int start_media_processing(const std::string& media_type,
                           const std::string& media_path,
                           bool loop,
                           bool detailed_logging) {
	std::string valid_media_path = validate_media_path(media_type, media_path);
	if (valid_media_path.empty())
		return 1;

	get_console_height();
	loop_flag = loop;

	if (!detailed_logging)
		cv::utils::logging::setLogLevel(
			cv::utils::logging::LogLevel::LOG_LEVEL_SILENT);

	if (media_type == "-v") {
		function_pointer = producer_video;
		//  Create a VideoCapture object and open the video file.
		cap.open(valid_media_path);

		if (!cap.isOpened()) {
			std::cerr << "Failed to open video file: " << valid_media_path << std::endl;
			return 0;
		}

		//  Get the frame rate of the video.
		fps = cap.get(cv::CAP_PROP_FPS);
		if (fps <= 0) {
			std::cerr << "Failed to get video frame rate." << std::endl;
		}

		frame_duration = 1000.0 / fps;
	} else if (media_type == "-i") {
		function_pointer = producer_image;
	} else {
		std::cerr << "Invalid input type: " << media_type << std::endl;
		return 0;
	}

#if DEBUG == 1
	std::cout << "========================= fps:            " << fps;
	std::cout << "========================= frame_duration: " << frame_duration;
#endif

	std::thread producerThread(function_pointer, valid_media_path);
	std::thread consumerThread(consumer);

	producerThread.join();
	consumerThread.join();

	return 1;
}

void producer_video(const std::string& video_file) {
	int frames = 0;
	int iteration = 1;
	while (!stop_flag) {
		// Loop through reading each frame of the video
		cv::Mat frame;
		if (!cap.read(frame)) {
			// Unable to read next frame: end of video or error.
			if (loop_flag) {
				// Reset the video frame position to the beginning of the video
				while (!image_queue.empty() && !stop_flag) {
					std::this_thread::sleep_for(std::chrono::milliseconds(20));
				}
				cap.set(cv::CAP_PROP_POS_FRAMES, 0);
				frames = 0;  // Reset the frame counter
				iteration++;
				continue;
			} else {
				// If continuous playback is not allowed, stop the producer thread
				break;
			}
		}

		// Put the frame into the queue
        {
			std::lock_guard<std::mutex> lock(image_queue_mtx);
			image_queue.push(frame.clone());  // Use cloning to avoid concurrency issues
		}

		frames++;
		std::lock_guard<std::mutex> lock(console_mtx);
		if (loop_flag) {
			gotoxy(0, console_height - 6);
			std::cout << "\rIteration #:           " << iteration;
		}
		gotoxy(0, console_height - 5);
		std::cout << "\rFrame # (Decoded):     " << frames;

		// Notify the consumer thread that a new image has arrived
		queueCond.notify_one();

		// Pause for a while before continuing to read the new frame
		// std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	producer_finished = true;
}

void producer_image(const std::string& directory) {
	int frames = 0;
	int iteration = 1;
	while (!stop_flag) {
		// Generate images and put them into the queue
		for (const auto& entry : std::filesystem::directory_iterator(directory)) {
			if (entry.is_regular_file()) {
				std::string path = entry.path().string();
                {
					std::lock_guard<std::mutex> console_lock(console_mtx);
					// Move the cursor to the third line from the bottom of the console
					gotoxy(0, console_height - 6);
					std::cout << "\rQueuing image:         " << path << "\n";
					cv::Mat image = cv::imread(path);
					if (!image.empty()) {
						// Put the image into the queue
                        {
							std::lock_guard<std::mutex> lock(image_queue_mtx);
							image_queue.push(image);
						}
						// Notify the consumer thread that a new image has arrived
						// queueCond.notify_one();
					}
					frames++;
					// std::lock_guard<std::mutex> lock(console_mtx);
					gotoxy(0, console_height - 5);
					std::cout << "\rFrame # (Decoded):     " << frames;
					if (loop_flag) {
						gotoxy(0, console_height - 7);
						std::cout << "\rIteration #:           " << iteration;
					}
				}
			}
		}

		// if (!loop_flag && allImagesAdded) {
		if (loop_flag) {
			// Reset the video frame position to the beginning of the video
			while (!image_queue.empty() && !stop_flag) {
				std::this_thread::sleep_for(std::chrono::milliseconds(20));
			}
			cap.set(cv::CAP_PROP_POS_FRAMES, 0);
			frames = 0;  // Reset the frame counter
			iteration++;
			continue;
		} else {
			// If continuous playback is not allowed, stop the producer thread
			break;
		}

		// Pause for a while before continuing to read new images
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	producer_finished = true;
}

void consumer() {
	int frames = 0;
	while (!stop_flag) {
		if (producer_finished && image_queue.empty())
			break;

		auto loop_start = std::chrono::high_resolution_clock::now();
		bool hasData = false;
        {
			std::unique_lock<std::mutex> lock(image_queue_mtx);
			if (!image_queue.empty()) {
				hasData = true;
				cv::Mat currentImage = image_queue.front();
				image_queue.pop();
				lock.unlock();
				SetBuffer(currentImage.data,
					     static_cast<DWORD>(currentImage.step),
					     1280,
					     720);
				frames++;
			}
		}

		auto setbuffer_done = std::chrono::steady_clock::now();
		auto elapsed_time = std::chrono::duration_cast
						    <std::chrono::milliseconds>(setbuffer_done - loop_start).count();

		// Calculate remaining waiting time
		auto remaining_time = static_cast<int64_t>
			(hasData ? frame_duration - elapsed_time - 1 : fps);

#if DEBUG == 1
		std::cout << "elapsed_time: " << elapsed_time << " ms" << std::endl;
		std::cout << "frame_duration: " << frame_duration << " ms" << std::endl;
		std::cout << "remaining: " << remaining_time << " ms" << std::endl;
#endif

		timeBeginPeriod(1);
		// Wait for the remaining time
		if (remaining_time > 0)
			std::this_thread::sleep_for(std::chrono::milliseconds(remaining_time));
		timeEndPeriod(1);

		auto loop_end = std::chrono::high_resolution_clock::now();
		auto loop_duration = std::chrono::duration_cast\
			                 <std::chrono::milliseconds>(loop_end - loop_start);

		std::lock_guard<std::mutex> lock(console_mtx);
		double real_fps = 1000.0 / loop_duration.count();
		// Move the cursor to the third line from the bottom of the console
		gotoxy(0, console_height - 3);
		std::cout << "\rFrame # (Consumed):    " << frames;

		// Move the cursor to the second line from the bottom of the console
		gotoxy(0, console_height - 2);
		std::cout << "Frame time:            " << loop_duration.count()\
			      << " ms" << std::endl;

		// Move the cursor to the last line of the console and print the FPS
		gotoxy(0, console_height - 1);
		std::cout << "\rFPS:                   " << std::fixed << real_fps;
	}
}

// the following code are test only
#if 0
void vcam(const std::string& imagePath) {
	auto loop_start = std::chrono::high_resolution_clock::now();

#if STB_ENABLED
	int width, height, channels;
	// unsigned char* imageData = stbi_load(imagePath.c_str(),
	//                                     &width,
	//                                     &height,
	//                                     &channels,
	//                                     STBI_rgb);
	uchar* imageData = stbi_load(imagePath.c_str(), &width, &height, &channels, 0);
	int stride = width * channels;
	// auto loop_end_1 = std::chrono::high_resolution_clock::now();
	// auto loop_duration_1 = std::chrono::duration_cast\
	//                        <std::chrono::milliseconds>(loop_end_1 - loop_start);
	// cout << "imread time for this iteration: " << loop_duration_1.count()\
	//      << " ms" << endl;

#else
	cv::Mat image = cv::imread(imagePath);

	auto loop_end_1 = std::chrono::high_resolution_clock::now();
	auto loop_duration_1 = std::chrono::duration_cast\
		                   <std::chrono::milliseconds>(loop_end_1 - loop_start);
	// cout << "imread time for this iteration: " << loop_duration_1.count()\
	//      << " ms" << endl;

	// Check if the image is successfully loaded
	if (image.empty()) {
		std::cerr << "Failed to load image." << std::endl;
		return;
	}

	// Get the width and height of the image
	int width = image.cols;
	int height = image.rows;

	// Get the pointer to the image data and its stride
	uchar* imageData = image.data;
	int stride = image.step;

#endif


	cv::Mat videoBuffer;

// Auto adapt width and height, but it takes time.
# if 0
	if (width != 1280 || height != 720) {
		double scaleX = static_cast<double>(1280) / width;
		double scaleY = static_cast<double>(720) / height;

		double scale;
		if (scaleX < scaleY) {
			scale = scaleX;
		} else {
			scale = scaleY;
		}

		int newWidth = static_cast<int>(scale * width);
		int newHeight = static_cast<int>(scale * height);

		resize(image, videoBuffer, Size(newWidth, newHeight));

		// Create a black background image with the same size as videoBuffer
		Mat blackBackground(720, 1280, videoBuffer.type(), Scalar(0, 0, 0));

		// Calculate the region of interest (ROI) to place the resized image in
		// the center of the background
		Rect roi((blackBackground.cols - videoBuffer.cols) / 2,
			     (blackBackground.rows - videoBuffer.rows) / 2,
			      videoBuffer.cols, videoBuffer.rows);

		// Copy the resized image to the ROI of the black background
		videoBuffer.copyTo(blackBackground(roi));

		imageData = blackBackground.data;
		stride = blackBackground.step;
		SetBuffer(imageData, stride, 1280, 720);
	} else {
		SetBuffer(imageData, stride, 1280, 720);
	}
#else
	SetBuffer(imageData, stride, 1280, 720);
#endif

	// auto loop_end = std::chrono::high_resolution_clock::now();

	// auto loop_duration = std::chrono::duration_cast
	//                      <std::chrono::milliseconds>(loop_end - loop_end_1);
	// cout << "Resizing time for this iteration: " << loop_duration.count()
	//      << " ms" << endl;
}

// Load a image and display it one by one
void loopThroughImages(const std::string& directory, bool loopFlag) {
	// Get all files in the directory
	std::vector<std::string> imagePaths;
	for (const auto& entry : std::filesystem::directory_iterator(directory)) {
		if (entry.is_regular_file()) {
			std::string path = entry.path().string();
			if (path.find("Image") != std::string::npos &&
				path.find(".bmp") != std::string::npos) {
				imagePaths.push_back(path);
			}
		}
	}

	// Loop through image files
	while (true) {
		for (const auto& imagePath : imagePaths) {
			auto loop_start = std::chrono::high_resolution_clock::now();

			vcam(imagePath);

			auto loop_end_2 = std::chrono::high_resolution_clock::now();
			auto loop_duration_1 = std::chrono::duration_cast
				                   <std::chrono::milliseconds>(loop_end_2 - loop_start);
			std::cout << "vcam time for this iteration: " << loop_duration_1.count()
				      << " ms" << std::endl;

			std::this_thread::sleep_for(std::chrono::milliseconds(5));

			auto loop_end_3 = std::chrono::high_resolution_clock::now();
			auto loop_duration_2 = std::chrono::duration_cast
				                   <std::chrono::milliseconds>(loop_end_3 - loop_end_2);
			std::cout << "sleep time for this iteration: " << loop_duration_2.count()
				      << " ms" << std::endl;
		}

		if (!loopFlag) {
			break;  // Exit loop_flag if no looping is needed
		}
	}
}

// Load all images to memory first then iterate them.
void loopThroughImages_Mem(const std::string& directory, bool loopFlag) {
	// Get all files in the directory
	std::vector<cv::Mat> images;
	for (const auto& entry : std::filesystem::directory_iterator(directory)) {
		if (entry.is_regular_file()) {
			std::string path = entry.path().string();
			if (path.find("Image") != std::string::npos &&
				path.find(".bmp") != std::string::npos) {
				cv::Mat image = cv::imread(path);
				if (!image.empty()) {
					images.push_back(image);
					std::cout << "pushing: image: " << path << "\n";
				}
			}
		}
	}

	// Check if there are any image
	if (images.empty()) {
		std::cerr << "No images found in directory." << std::endl;
		return;
	}

	auto start = std::chrono::high_resolution_clock::now();

	// Loop through image files
	int currentIndex = 0;
	while (true) {
		auto loop_start = std::chrono::high_resolution_clock::now();

		cv::Mat currentImage = images[currentIndex];
		SetBuffer(currentImage.data, currentImage.step, 1280, 720);

		// If the loop_flag flag is false and we've reached the last image, exit
		if (!loopFlag && currentIndex == images.size() - 1) {
			break;
		}

		// Wait for 30ms
		std::this_thread::sleep_for(std::chrono::milliseconds(15));

		// Update the index to switch to the next image
		currentIndex = (currentIndex + 1) % images.size();

		auto loop_end_2 = std::chrono::high_resolution_clock::now();
		auto loop_duration_1 = std::chrono::duration_cast\
			                   <std::chrono::milliseconds>(loop_end_2 - loop_start);
		std::cout << "vcam time for this iteration: " << loop_duration_1.count()
			      << " ms" << std::endl;
	}

	auto end = std::chrono::high_resolution_clock::now();
	auto loop_duration_1 = std::chrono::duration_cast\
		                   <std::chrono::milliseconds>(end - start);
	std::cout << "vcam full time: " << loop_duration_1.count()\
		      << " ms" << std::endl;
}

#endif
