#pragma once

#include <cstdint>


enum class EChatTableID : uint16_t
{
	HEARTBEAT,
	MESSAGE_TABLE,
	GREETINGS_TABLE,
	ID_LIST_TABLE,
	MAX
};
