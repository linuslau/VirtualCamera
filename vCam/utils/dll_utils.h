/* Copyright(c), 2024, linuslau (liukezhao@gmail.com) */
// dll_utils.h

#pragma once

#ifndef DLL_UTILS_HPP
#define DLL_UTILS_HPP

#include <windows.h>

// Function pointer type declaration
typedef int  (*InitFunc)();
typedef int  (*FreeFunc)();
typedef int  (*GetNumDevicesFunc)();
typedef int  (*GetDevicePathFunc)(int, char*, int);
typedef void (*DestroyDeviceFunc)();
typedef int  (*SetDeviceFunc)(char*, int);
typedef int  (*SetBufferFunc)(void*, DWORD, DWORD, DWORD);

// Declare function pointers as extern
extern InitFunc Init;
extern FreeFunc Free;
extern GetNumDevicesFunc GetNumDevices;
extern GetDevicePathFunc GetDevicePath;
extern DestroyDeviceFunc DestroyDevice;
extern SetDeviceFunc SetDevice;
extern SetBufferFunc SetBuffer;

// Function declarations
bool init_dll();
void free_dll();

#endif  // DLL_UTILS_HPP
