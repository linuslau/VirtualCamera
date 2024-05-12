/* Copyright(c), 2024, linuslau (liukezhao@gmail.com) */

#include "dll_utils.h"   // NOLINT(build/include_subdir)

#include <windows.h>

#include <iostream>
#include <regex>  // NOLINT(build/c++11)
#include <string>

// Define function pointers and HINSTANCE variable
InitFunc Init;
FreeFunc Free;
GetNumDevicesFunc GetNumDevices;
GetDevicePathFunc GetDevicePath;
DestroyDeviceFunc DestroyDevice;
SetDeviceFunc SetDevice;
SetBufferFunc SetBuffer;

HINSTANCE hDll;

// Function to extract the device identifier
std::string ExtractDeviceIdentifier(const std::string& devicePath) {
	// Define the modified regex pattern to match the device identifier
	std::regex pattern("@device:pnp:\\\\.*root.*unknown.*global");

	// Match using the regex pattern
	std::smatch match;
	if (std::regex_search(devicePath, match, pattern)) {
		if (match.size() > 0) {
			// Return the first matched group as the device identifier
			return match.str(0);
		}
	}
	// Return an empty string if no matching device identifier is found
	return "";
}

bool init_dll() {
	// Load the dynamic link library
	hDll = LoadLibrary(TEXT("DriverInterface.dll"));
	if (hDll == NULL) {
		// Get detailed error information
		LPVOID lpMsgBuf;
		DWORD dw = GetLastError();

		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0,
			NULL);

		std::cerr << "Failed to load DLL. Error code: " << dw << ", Error message: "
				  << (LPCTSTR)lpMsgBuf << std::endl;

		LocalFree(lpMsgBuf);  // Free memory
		return false;
	}

	// Get the function addresses
	Init = reinterpret_cast<InitFunc>(GetProcAddress(hDll, "Init"));
	if (Init == nullptr) {
		std::cerr << "Failed to get function address." << std::endl;
	}

	Free = reinterpret_cast<FreeFunc>(GetProcAddress(hDll, "Free"));
	if (Free == nullptr) {
		std::cerr << "Failed to get function address." << std::endl;
	}

	GetNumDevices = reinterpret_cast<GetNumDevicesFunc>
		(GetProcAddress(hDll, "GetNumDevices"));
	if (GetNumDevices == nullptr) {
		std::cerr << "Failed to get GetNumDevicesFunc address." << std::endl;
	}

	GetDevicePath = reinterpret_cast<GetDevicePathFunc>
		(GetProcAddress(hDll, "GetDevicePath"));
	if (GetDevicePath == nullptr) {
		std::cerr << "Failed to get GetDevicePathFunc address." << std::endl;
	}

	DestroyDevice = reinterpret_cast<DestroyDeviceFunc>
		(GetProcAddress(hDll, "DestroyDevice"));
	if (DestroyDevice == nullptr) {
		std::cerr << "Failed to get DestroyDeviceFunc address." << std::endl;
	}

	SetDevice = reinterpret_cast<SetDeviceFunc>
		(GetProcAddress(hDll, "SetDevice"));
	if (SetDevice == nullptr) {
		std::cerr << "Failed to get SetDeviceFunc address." << std::endl;
	}

	SetBuffer = reinterpret_cast<SetBufferFunc>(GetProcAddress(hDll, "SetBuffer"));
	if (SetBuffer == nullptr) {
		std::cerr << "Failed to get SetBufferFunc address." << std::endl;
	}

	if (!Init || !Free || !GetNumDevices || !GetDevicePath ||
		!DestroyDevice || !SetDevice || !SetBuffer) {
		std::cerr << "Failed to get function pointers" << std::endl;
		FreeLibrary(hDll);
		return false;
	}

	// Initialize the DLL
	if (!Init()) {
		std::cerr << "Failed to initialize DLL" << std::endl;
		FreeLibrary(hDll);
		return false;
	}

	// Get the number of devices
	int numDevices = GetNumDevices();
	if (numDevices <= 0) {
		std::cerr << "Failed to get number of devices" << std::endl;
		Free();
		FreeLibrary(hDll);
		return false;
	}

	// Find the device with the maximum index
	int maxIndex = numDevices - 1;

	/* //hardcode index reference
	// Get the device path
	char devicePath[256];
	if (!GetDevicePath(1, devicePath, sizeof(devicePath))) {
		std::cerr << "Failed to get device path" << std::endl;
		Free();
		FreeLibrary(hDll);
		return;
	}

	// Set the device
	if (!SetDevice(devicePath, strlen(devicePath))) {
		std::cerr << "Failed to set device" << std::endl;
		Free();
		FreeLibrary(hDll);
		return;
	}
	*/

	// Query the index of all devices
	std::cout << std::endl << "1. Checking vCam presence:" << std::endl;
	for (int index = 0; index <= maxIndex; ++index) {
		char devicePath[256];
		if (!GetDevicePath(index, devicePath, sizeof(devicePath))) {
			std::cerr << "Failed to get device path for index " << index << std::endl;
			continue;  // Continue to the next device query
		}

		// Extract device identifier
		std::string deviceIdentifier = ExtractDeviceIdentifier(devicePath);

		// Output device identifier
		if (!deviceIdentifier.empty()) {
			std::cout << "Successfully get vCam device at index "
				      << index << ": " << deviceIdentifier << std::endl;
			if (!SetDevice(devicePath, static_cast<int>(strlen(devicePath)))) {
				std::cerr << "Failed to set device" << std::endl;
				Free();
				FreeLibrary(hDll);
				return false;
			}
			break;
		} else {
			std::cout << "Ignore device identifier for device at index "
				      << index << std::endl;
		}
	}
	return true;
}

void free_dll() {
    if (hDll != NULL) {
        Free();
        FreeLibrary(hDll);
        hDll = NULL;
    }
}
