#pragma once

#include <boost/system.hpp>

#if BOOST_WINDOWS
#define WIN32_MEAN_AND_LEAN
#include <boost/winapi/get_proc_address.hpp>
#include <Windows.h>
#include <WinBase.h>
#elif BOOST_LINUX
#include <dlfcn.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <memory>

#include "IntPtr.hpp"
