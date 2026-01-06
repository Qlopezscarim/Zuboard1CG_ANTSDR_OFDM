#include "energy_detector.h"
  
extern bool stop_signal_called;

float	DETECTOR::alpha 		= 0.850f;
float	DETECTOR::power_detector 	= 0.0f;
float	DETECTOR::power_threshold 	= 0.00000005f;
bool	DETECTOR::block_stored 		= false;
size_t	DETECTOR::block_index 		= 0;
size_t	DETECTOR::packet_num		= 0;

void DETECTOR::energyDetectorThread
(MutexFIFO<std::vector<std::complex<float>>>& data_fifo,
MutexFIFO<std::vector<std::complex<float>>>& data_fifo2
//size_t& sblocks,
//SharedPrinter& printer
)
{

	std::vector<std::complex<float>> fifo_output;
	std::vector<std::complex<float>> buff (SIZE_BLOCKS_TO_STORE, std::complex<float>(0.0f, 0.0f));

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

		//non-optimized no SIMD code
		//for simplicity we are going to assume only one power thread exist
		//otherwise blocks must be accessed in order from the FIFO and the actual
		//IIR filter will need to also be accessed in order in which case 
		//we just have pending threads - sure the math for one block can happen in the background
		//and then it's a slight save but overall seems like a minimal save so 
		//for the more or less serial sequence we will use one thread

		//The state machine is as follows:
		//Collect power samples with IIR filter result in power_detector
		//if power_detector > threshold store the next block samples and keep
		//doing IIR filter in background
		//Collect the next 1000 samples - stop collecting ONLY once those 1000 are collected
		//start re-collecting after threshold passes again
		for(size_t i = 0; i < fifo_output.size() ; i++)
		{
			power_detector = alpha*power_detector + (1-alpha)*std::norm(fifo_output[i]);
			//IIR filter

			std::cout << "Current power reading is: " << power_detector << std::endl;

			if(power_detector > power_threshold || block_stored == true)
			{
				block_stored = true;
				if(block_index < SIZE_BLOCKS_TO_STORE)
				{
					//store to fifo
					buff[block_index] = fifo_output[i];
					block_index = block_index + 1;
				}
				else
				{
					//std::cout << "Power Thread:\t "<<block_index << std::endl;
					//Implementing AGC before pushing packet:
					float agc_sum = 0;
					for (int i = 0; i<SIZE_BLOCKS_TO_STORE ; i++)
					{
						agc_sum = agc_sum +  std::norm(buff[i]);
					}
					float rms = std::sqrt(agc_sum / SIZE_BLOCKS_TO_STORE);
					for (int i = 0; i<SIZE_BLOCKS_TO_STORE ; i++)
                                        {
                                                buff[i] = buff[i]/rms;
                                        }

					//want to store which "packet" we are on
					//buff[0] = std::complex<float>(packet_num,0.0f);

					//want to push the buffer to FIFO so we can reuse buffer
					data_fifo2.push(buff);

					//for debugging
					//std::cout << "Packet detected and pushed to averaging" << "\n";
					
					//reset conditions
					//may want to make power_threshold 0 for false alarms?
					block_stored	= false;
					block_index	= 0;
					//packet_num	= packet_num + 1;
				}
			}
			else
			{
				/*if(packet_num == 0)
				{
					std::cout << "No packet detected: " << power_detector << std::endl;
					std::cout << "Input number: \t" << std::norm(fifo_output[i]) << std::endl;
				}*/
			}
		}
		//total_power = total_power/sblocks;
		//thread safe implementation
		//printer.print("block number: \t" + std::to_string(fifo_output[0].real()) + " Total power: \t" + std::to_string(total_power));
		//non-thread safe
		//std::cout << "\t" << power_detector << "\n";
		//std::cout << "block number: \t" << fifo_output[0].real() << " Total power: \t" << total_power << std::endl;
	}

	std::cout << "Graceful exit of energyDetectorThread" << std::endl;

}
