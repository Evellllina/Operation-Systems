#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <vector>

#ifdef _WIN32
#define OS_WIN
#include <windows.h>
#else
#define OS_LINUX
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#endif