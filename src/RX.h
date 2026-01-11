#pragma once

#include <uhd/exception.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <cmath>
#include <csignal>
#include <fstream>
#include <functional>
#include <iostream>
#include <thread>
#include "utils/queue.h"
#include "configs/rx_config.h"

//#define BLOCK_SIZE 1000
#define REQ_SAMPLES 0 //doing continous
#define SAMP_TYPE 0
#define TIMEOUT 0

class RX
{
public:
//spawns main RX thread according to passes parameters
static std::thread rx_thread(const std::string& cpu_format,
	const std::string& wire_format,
        const std::string& file,
	const std::string&  rx_channels,
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
	MutexFIFO<std::vector<rx_cpu_format>>& data_fifo);


static void rx_worker
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

);

};

