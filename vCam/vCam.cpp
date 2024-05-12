/* Copyright(c), 2024, linuslau (liukezhao@gmail.com) */

#include <iostream>

#include "utils/args_utils.h"
#include "utils/file_utils.h"
#include "utils/dll_utils.h"
#include "media_processor/media_processor.h"

int main(int argc, char* argv[]) {
	std::cout << "Hello, KZ vCam Test App. \n";

	std::string media_type, media_path;
	bool loop = false, cv_log = false;

	if (!parseArguments(argc, argv, media_type, media_path, loop, cv_log)) {
		std::system("pause");
		return 1;
	}

	init_dll();

	if (!start_media_processing(media_type, media_path, loop, cv_log))
		return 1;

	free_dll();

	return 0;
}

