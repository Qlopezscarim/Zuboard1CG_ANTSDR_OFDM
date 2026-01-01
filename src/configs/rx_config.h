#pragma once

struct RxConfig {
	//The size of the blocks pulled from the RX thread and passed "upstream"
	size_t samples_per_recv = 4096;
	//The size of the blocks captured from the energy detector and passed along
	size_t energy_detector_grabbed_samples = 1000;
};

