#pragma once

#include <stdio.h>

#ifndef _USE_OLD_OSTREAMS
using namespace std;
#endif

#include <windows.h>
#include <mutex>

#include <TlHelp32.h>

#include "spdlog\spdlog.h"

using namespace spdlog;


class DSIII
{
public:
	[[noreturn]] static void run_worker();


};

