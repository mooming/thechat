#pragma once

#include <chrono>
#include <cstdint>
#include <winsock2.h>


namespace Network
{
	using TSocket = SOCKET;
	using TTimeStamp = std::chrono::time_point<std::chrono::steady_clock>;
	bool Initialize();
	void Deinit();
}
