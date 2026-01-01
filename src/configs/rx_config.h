#pragma once
struct RxConfig {
	//The size of the blocks pulled from the RX thread and passed "upstream"
	size_t samples_per_recv = 4096;
};

