#include "energy_detector.h"

extern bool stop_signal_called;

void ENERGYDETECTOR::energyDetectorThread
(
MutexFIFO<std::vector<std::complex<float>>>& data_fifo,
size_t& sblocks
)

{
	std::vector<std::complex<float>> fifo_output;

	while(not stop_signal_called)
	{
		while( not data_fifo.pop(fifo_output) ) //keep trying to pop until the FIFO has data
		{
			if( stop_signal_called )
			{
			std::cout << "Graceful exit of energyDetectorThread" << std::endl;
			return;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1));

		}
		float total_power = 0;
		//non-optimized no SIMD code
		for(size_t i = 0; i<sblocks ; i++)
		{
			//pass for now
			total_power = total_power + std::norm(fifo_output[i+1]);
		}
		total_power = total_power/sblocks;
		std::cout << "The detected average block power was: " << total_power << std::endl;
	}

	std::cout << "Graceful exit of energyDetectorThread" << std::endl;

}
