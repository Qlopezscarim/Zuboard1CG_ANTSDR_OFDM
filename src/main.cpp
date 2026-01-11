/*
//	Code written from reference 2.4.8 txrx_loopback_to_file.cpp (RX)
*/

#include <uhd/utils/safe_main.hpp>
#include <csignal>
#include "RX.h"
#include <cstdint>
#include <bitset>
#include "utils/queue.h"
#include "configs/rx_config.h"
#include "energy_calculator.h"
#include "energy_detector.h"

namespace po = boost::program_options;


// Interrupt signal handler
bool stop_signal_called = false;
void sig_int_handler(int) {
    stop_signal_called = true;
}


int UHD_SAFE_MAIN(int argc, char *argv[]) {
    //INPUT HANDLING WITH BOOST
    //TX paramters to be set
	std::string tx_args, wave_type, tx_ant, tx_subdev, ref, tx_channels;
	double tx_rate, tx_freq, tx_gain, wave_freq, tx_bw;
	float ampl;

    //RX parameters to be set
	std::string rx_args, file, type, rx_ant, rx_subdev, rx_channels;
	size_t total_num_samps, spb, sblocks;
	double rx_rate, rx_freq, rx_gain, rx_bw;
	double settling;

    // Communication parameters to be set
	bool is_sink;

    // setup the program options
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help", "help message")
	("is-sink", po::value<bool>(&is_sink)->default_value("false"),"Is sink or source")
        ("tx-args", po::value<std::string>(&tx_args)->default_value(""), "uhd transmit device address args")
        ("rx-args", po::value<std::string>(&rx_args)->default_value(""), "uhd receive device address args")
        ("file", po::value<std::string>(&file)->default_value("no"), "name of the file to write binary samples to (Do not name it no)")
        ("type", po::value<std::string>(&type)->default_value("short"), "sample type in file: double, float, or short")
        ("sblocks", po::value<size_t>(&sblocks)->default_value(10000), "total size of blocks for recieve")
        ("settling", po::value<double>(&settling)->default_value(double(0.2)), "settling time (seconds) before receiving")
        ("spb", po::value<size_t>(&spb)->default_value(0), "samples per buffer, 0 for default")
        ("tx-rate", po::value<double>(&tx_rate), "rate of transmit outgoing samples")
        ("rx-rate", po::value<double>(&rx_rate), "rate of receive incoming samples")
        ("tx-freq", po::value<double>(&tx_freq), "transmit RF center frequency in Hz")
        ("rx-freq", po::value<double>(&rx_freq), "receive RF center frequency in Hz")
        ("ampl", po::value<float>(&ampl)->default_value(float(0.3)), "amplitude of the waveform [0 to 0.7]")
        ("tx-gain", po::value<double>(&tx_gain), "gain for the transmit RF chain")
        ("rx-gain", po::value<double>(&rx_gain), "gain for the receive RF chain")
        ("tx-ant", po::value<std::string>(&tx_ant), "transmit antenna selection")
        ("rx-ant", po::value<std::string>(&rx_ant), "receive antenna selection")
        ("tx-subdev", po::value<std::string>(&tx_subdev), "transmit subdevice specification")
        ("rx-subdev", po::value<std::string>(&rx_subdev), "receive subdevice specification")
        ("tx-bw", po::value<double>(&tx_bw), "analog transmit filter bandwidth in Hz")
        ("rx-bw", po::value<double>(&rx_bw), "analog receive filter bandwidth in Hz")
        ("wave-type", po::value<std::string>(&wave_type)->default_value("CONST"), "waveform type (CONST, SQUARE, RAMP, SINE)")
        ("wave-freq", po::value<double>(&wave_freq)->default_value(0), "waveform frequency in Hz")
        ("ref", po::value<std::string>(&ref)->default_value("internal"), "clock reference (internal, external, mimo)")
//        ("otw", po::value<std::string>(&otw)->default_value("sc16"), "specify the over-the-wire sample mode")
        ("tx-channels", po::value<std::string>(&tx_channels)->default_value("0"), "which TX channel(s) to use (specify \"0\", \"1\", \"0,1\", etc)")
        ("rx-channels", po::value<std::string>(&rx_channels)->default_value("0"), "which RX channel(s) to use (specify \"0\", \"1\", \"0,1\", etc)")
        ("tx-int-n", "tune USRP TX with integer-N tuning")
        ("rx-int-n", "tune USRP RX with integer-N tuning")
    ;
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    //END OF INPUT HANDLING WITH BOOST

    //Print out help message if neccessary/asked for 
    if (vm.count("help")) {
        std::cout << "Lab1 " << desc << std::endl;
        return ~0;
    }

    // Set the signal handler
    std::signal(SIGINT, &sig_int_handler);
    // Keep running until CTRL-C issued

    // Parameter checking
    bool gain_set	= false;
    bool bw_set 	= false;
    bool ant_set	= false;
    // RX_RATE check
    if (not vm.count("rx-rate") && not vm.count("tx-rate")) 
    {         
	std::cerr << "Please specify the sample rate with --rx-rate" << std::endl;
	std::cerr << "or please specify the sample rate with --tx-rate" << std::endl;\
	return ~0;     
    }
   if (not vm.count("rx-freq") && not vm.count("tx-freq"))
   {
	std::cerr << "Please specify the center frequency with --rx-freq" <<
	"or please specify the center frequency with --tx-freq"
	<< std::endl;
	return ~0;
   }
   std::cout << "NOTE NO IMPLEMENTATION FOR rx-int-n CURRENTLY" << std::endl;
   if(vm.count("rx-gain") || vm.count("tx-gain"))
   {
   gain_set = true;
   }
   if(vm.count("rx-bw") || vm.count("tx-bw"))
   {
   bw_set = true;
   }
   if(vm.count("rx-ant") || vm.count("tx-ant"))
   {
   ant_set = true;
   }


/***********************START OF RX CODE*************************************************/

//instantiating FIFO from RX to energyDetectorThread
MutexFIFO<std::vector<rx_cpu_format>> data_fifo;

//instantiating FIFO from energyDetectorThread to XXX thread
MutexFIFO<std::vector<rx_cpu_format>> data_fifo_2;


RxConfig rx_config;

//instantiating packet size variable for ref passing - relic of old power thread
size_t pack_size = rx_config.samples_per_recv;
//size_t grabbed_block_size = rx_config.energy_detector_grabbed_samples;


//instantiating energy calculator thread
//Used for initial debugging an antenna checking - now depreciated
/*std::thread energy_calculator_object(ENERGYCALCULATOR::energyCalculatorThread,
        std::ref(data_fifo),
	std::ref(pack_size)
       // std::ref(grabbed_block_size)
);*/

//instantiating energy detector thread
std::thread energy_detector_object(DETECTOR::energyDetectorThread,
        std::ref(data_fifo),
        std::ref(data_fifo_2)
       // std::ref(grabbed_block_size)
);


//Setting options for RX cpu data types depending on compile options:
#if defined(RXFC64)
	std::string cpu_format = "fc64";
#elif defined(RXFC32)
	std::string cpu_format = "fc32";
#elif defined(RXSC16)
	std::string cpu_format = "sc16";
#elif defined(RXSC8)
	std::string cpu_format = "sc8";
#else
	#error "Define a RX CPU type in the compile options"
#endif

//Setting options for RX OTW data types depending on compile options
#if defined(RXOTWSC16)
	std::string otw = "sc16";
#elif defined(RXOTWSC12)
	std::string otw = "sc12";
#elif defined(RXOTWSC8)
        std::string otw	= "sc8";
#else
        #error "Define a RX OTW type in the compile options"
#endif


//instantiating RX thread:
std::thread rx_thread_object = RX::rx_thread(
        cpu_format,
        otw,
        "",
        rx_channels,
        rx_args,
        ref,
        rx_rate,
        rx_freq,
        gain_set,
        rx_gain,
        bw_set,
        rx_bw,
        ant_set,
        rx_ant,
        settling, //time
        std::ref(sblocks), //size of blocks
        std::ref(data_fifo));


    size_t count = 0;
    while(not stop_signal_called)
    {
	std::this_thread::sleep_for(std::chrono::milliseconds(1)); // sleep for 1ms to make sure can catch CTRL-C
	count++;
	if(count % 3000 == 0)
	{

#ifdef VERBOSETX
		std::cout << "FIFO from filter_thread to TX_thread:\t\t\t\t" << data_fifo_TX.size() << std::endl;
                std::cout << "FIFO from modulator_thread to filter_thread:\t\t\t" << data_fifo2_TX.size()<< std::endl;
                std::cout << "FIFO from producer_thread to modulator_thread:\t\t\t" << data_fifo3_TX.size()<< std::endl;
#endif

#ifdef VERBOSERX
		std::cout << "\t\t\t\t\t\t\t\t\t\t\t\t\t|FIFO 1:\t" << data_fifo.size() << std::endl;
		std::cout << "\t\t\t\t\t\t\t\t\t\t\t\t\t|FIFO 2:\t" << data_fifo_2.size()<< std::endl;
		//std::cout << "\t\t\t\t\t\t\t\t\t\t\t\t\t|FIFO 3:\t" << data_fifo3.size()<< std::endl;
		//std::cout << "\t\t\t\t\t\t\t\t\t\t\t\t\t|FIFO Correlator to demodulator " <<  Correlator_to_Demodulator.size() << std::endl;
#endif

	}
    }
    //want other threads to exit cleanly before exiting program:
    std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // sleep for 1ms to make
    
    rx_thread_object.join();
    energy_detector_object.join();
//    energy_calculator_object.join();



    std::cout << "Clean program exit; all threads joined back forcefully" << std::endl;
    return EXIT_SUCCESS;

}

