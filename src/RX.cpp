#include "RX.h"



extern bool stop_signal_called;

std::thread RX::rx_thread
(	const std::string& cpu_format,
	const std::string& wire_format,
	const std::string& file,
	const std::string& rx_channels,
	const std::string& rx_args,
	std::string& ref,
	double& rx_rate,
	double& rx_freq,
	bool& set_gain,
	double& rx_gain,
	bool& bw_set,
	double& rx_bw,
	bool& ant_set,
	std::string& rx_ant,
	double& settling_time,
	size_t& sblocks,
	MutexFIFO<std::vector<rx_cpu_format>>& data_fifo
)
{

//Create the initial rx_usrp object from passed boost parameters
std::cout << boost::format("Creating the receive usrp device with: %s...") % rx_args
	<< std::endl;
uhd::usrp::multi_usrp::sptr rx_usrp = uhd::usrp::multi_usrp::make(rx_args);

std::cout << std::endl;
auto ants = rx_usrp->get_rx_antennas(0);
std::cout << "Available RX antennas:\n";
for (const auto& ant : ants) {
    std::cout << "  " << ant << '\n';
}
std::cout << std::endl;


//Verifying passed RX channels are valid:
//rx_channel_nums holds all valid channel numbers at the end
std::vector<std::string> rx_channel_strings;
std::vector<size_t> rx_channel_nums;     
boost::split(rx_channel_strings, rx_channels, boost::is_any_of("\"',"));
for (size_t ch = 0; ch < rx_channel_strings.size(); ch++) 
{         
	size_t chan = std::stoi(rx_channel_strings[ch]);
        if (chan >= rx_usrp->get_rx_num_channels()) 
	{             
		throw std::runtime_error("Invalid RX channel(s) specified.");         
	}
	 else            
	{
		 rx_channel_nums.push_back(std::stoi(rx_channel_strings[ch]));
	}
}


//Locking mboard clock for rx
if(ref == "internal")
{
	std::cout << "setting mboard clock only for RX with parameter " << ref;
}
rx_usrp->set_clock_source(ref);    


//More verbose output:
std::cout << "Using RX Device: " << rx_usrp->get_pp_string();


//setting RX rate
std::cout << boost::format("Setting RX Rate: %f Msps...") % (rx_rate / 1e6)  << std::endl;
rx_usrp->set_rx_rate(rx_rate);
std::cout << boost::format("Actual RX Rate: %f Msps...")  % (rx_usrp->get_rx_rate() / 1e6)
<< std::endl               
<< std::endl;


//setting RX center frequency
for (size_t ch = 0; ch < rx_channel_nums.size(); ch++) 
{         
	//size_t channel = rx_channel_nums[ch];         
	//if (rx_channel_nums.size() > 1) 
	//{             
	std::cout << "Configuring RX Channel " << rx_channel_nums[0] << " ONLY" << std::endl;
	//}
}
size_t channel = rx_channel_nums[0];
std::cout << boost::format("Setting RX Freq: %f MHz...") % (rx_freq / 1e6)  << std::endl;
uhd::tune_request_t rx_tune_request(rx_freq);
//if (vm.count("rx-int-n"))             
//	rx_tune_request.args = uhd::device_addr_t("mode_n=integer");
rx_usrp->set_rx_freq(rx_tune_request, channel);
std::cout << boost::format("Actual RX Freq: %f MHz...")   % (rx_usrp->get_rx_freq(channel) / 1e6) 
 << std::endl
 << std::endl;

//setting RX gain
if(set_gain)
{
	std::cout << boost::format("Setting RX Gain: %f dB...") % rx_gain
        << std::endl;
        rx_usrp->set_rx_gain(rx_gain, channel);
        std::cout << boost::format("Actual RX Gain: %f dB...")  % rx_usrp->get_rx_gain(channel)
        << std::endl
        << std::endl;
}

// set the receive analog frontend filter bandwidth
if (bw_set) 
{             
	std::cout << boost::format("Setting RX Bandwidth: %f MHz...") % (rx_bw / 1e6)
        << std::endl;
        rx_usrp->set_rx_bandwidth(rx_bw, channel);
        std::cout << boost::format("Actual RX Bandwidth: %f MHz...")  % (rx_usrp->get_rx_bandwidth(channel) / 1e6)
        << std::endl
        << std::endl;         
}         
// set the receive antenna
if (ant_set)
    rx_usrp->set_rx_antenna(rx_ant, channel);

// enable RX DC offset correction
//std::cout << "Enabling RX DC offset correction..." << std::endl;
//rx_usrp->set_rx_dc_offset(true, channel);

//check LO locked
std::vector<std::string> rx_sensor_names;
rx_sensor_names = rx_usrp->get_rx_sensor_names(0);
if (std::find(rx_sensor_names.begin(), rx_sensor_names.end(), "lo_locked")  != rx_sensor_names.end()) 
{
         uhd::sensor_value_t lo_locked = rx_usrp->get_rx_sensor("lo_locked", 0);
         std::cout << boost::format("Checking RX: %s ...") % lo_locked.to_pp_string()
                   << std::endl;         
	bool is_locked = false;
	for(int i=0; i<20;i++)
	{
		if(lo_locked.to_bool())
		{
			is_locked = true;
			break;
		}
		std::cout << "checking LO; iteration " << i  << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		lo_locked = rx_usrp->get_rx_sensor("lo_locked",0);
		//UHD_ASSERT_THROW(lo_locked.to_bool());
	}
	if(!is_locked)
	{
		std::cerr << "Warning: RX LO FAILED AFTER 20 RETRIES";
	}
	else
	{
		std::cout << "\nRX LO locked successfully" << std::endl;
	}
} 
std::cout << "RX worker thread expects std::complex<float> in, be aware of that" << std::endl;

std::thread worker(&RX::rx_worker,
                       //this,
                       rx_usrp,
                       cpu_format,
                       wire_format,
                       file,
                       //samps_per_buff,
		       sblocks,
                       //num_requested_samples,
		       REQ_SAMPLES,
                       settling_time,
                       rx_channel_nums,
                       std::ref(data_fifo)
                       );

return worker;

}







void RX::rx_worker
(
uhd::usrp::multi_usrp::sptr rx_usrp,
const std::string& cpu_format,
const std::string& wire_format,
const std::string& file,
size_t samps_per_buff,
int num_requested_samples,
double settling_time,
std::vector<size_t> rx_channel_nums,
MutexFIFO<std::vector<rx_cpu_format>>& data_fifo //may need to be changed
)
{

std::cout << "Worker thread actually instantiated";

uhd::set_thread_priority_safe(1.0, true);


bool overflow_message = true; //for error checking
// create a receive streamer
uhd::stream_args_t stream_args(cpu_format, wire_format);
stream_args.channels             = rx_channel_nums;
//stream_args.spp			 = samps_per_buff;
uhd::rx_streamer::sptr rx_stream = rx_usrp->get_rx_stream(stream_args);

uhd::rx_metadata_t md;
std::vector<std::complex<float>> buff(samps_per_buff, std::complex<float>(0.0f, 0.0f));
uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
stream_cmd.stream_now = true; //this is fine for now but if we add more initialization may want to stall
rx_stream->issue_stream_cmd(stream_cmd);
double timeout = settling_time + 0.5f;
size_t num_block = 0;

//START OF HACK ---------------------------------
//There is a persistant transient issue - perhaps my hardware isn't settling or the DC offset isn't being
//managed? Either way this is my solution. I don't like it - but it works. I have no explanation except
//for some transient hardware effect is dominating the signal for the first X samples.

// FLUSH STARTUP TRANSIENT - Discard first several buffers
    // Empirically observed: first ~20-30 buffers contain transient behavior
    std::cout << "Flushing startup transient (discarding first buffers)..." << std::endl;
    const int NUM_FLUSH_BUFFERS = 30;  // Adjust based on your observations
    for(int i = 0; i < NUM_FLUSH_BUFFERS; i++) {
        rx_stream->recv(&buff.front(), samps_per_buff, md, timeout);
        timeout = 0.1f;
        if(i % 10 == 0) {
            std::cout << "  Flushed " << i << " buffers..." << std::endl;
        }
    }
    std::cout << "Startup flush complete (" << NUM_FLUSH_BUFFERS << " buffers discarded)" << std::endl;
// END OF HACK ---------------------------------


while(not stop_signal_called)
{
	//buff[0] = std::complex<float>(num_block,0.0f);
	rx_stream->recv(&buff.front(),samps_per_buff, md, timeout);
	timeout		= 0.1f;
	if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
            std::cout << "Timeout while streaming" << std::endl;
            break;
        }

        if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW) {
            if (overflow_message) {
                overflow_message = false;
                std::cerr
                    << boost::format(
                           "Got an overflow indication. Please consider the following:\n"
                           "  Your write medium must sustain a rate of %fMB/s.\n"
                           "  Dropped samples will not be written to the file.\n"
                           "  Please modify this example for your purposes.\n"
                       "  This message will not appear again.\n")
                           % (rx_usrp->get_rx_rate() * sizeof(std::complex<float>) / 1e6);
            }
            continue;
        }
        if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
            throw std::runtime_error("Receiver error " + md.strerror());
        }
	
	//std::this_thread::sleep_for(std::chrono::milliseconds(100));
        //data_fifo.push(buff); //This makes a copy which we want to re-use the buffer
	//num_block++;
        //then are happy to fill with new samples and repeat this process
}
//clean exit code
stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
rx_stream->issue_stream_cmd(stream_cmd);
std::cout << "RX thread exited cleanly" << std::endl;

}

