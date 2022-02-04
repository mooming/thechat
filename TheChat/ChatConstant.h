#pragma once

#include <cstdint>


namespace ChatConstant
{
	static constexpr int DEFAULT_PORT = 8089;
	static constexpr int PACKET_SIZE = 256;
	static constexpr int PACKET_LAST_INDEX = PACKET_SIZE - 1;
	static constexpr int LARGE_PACKET_SIZE = 4096;
	static constexpr int LARGE_PACKET_LAST_INDEX = LARGE_PACKET_SIZE - 1;
	static constexpr uint32_t HEART_BEAT_PERIOD = 2000;
	static constexpr uint32_t CONNECTION_TIMEOUT = HEART_BEAT_PERIOD * 5;
}
