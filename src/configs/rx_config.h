#pragma once

struct RxConfig {
	//The size of the blocks pulled from the RX thread and passed "upstream"
	size_t samples_per_recv = 4096;
	//The size of the blocks captured from the energy detector and passed along
	size_t energy_detector_grabbed_samples = 1000;
};


#include <complex>


//Handling the CPU format -- this actually changes things on the fly so we modify it with a define

#if defined(RXFC64)
	using rx_cpu_format = std::complex<double>;
#elif defined(RXFC32)
	using rx_cpu_format = std::complex<float>;
#elif defined(RXSC16)
	using rx_cpu_format = std::complex<int16_t>;
#elif defined(RXSC8)
	using rx_cpu_format = std::complex<int8_t>;
#else
	#error "No sample type defined"
#endif
