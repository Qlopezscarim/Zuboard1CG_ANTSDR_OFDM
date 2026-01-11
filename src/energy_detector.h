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
#include "RX.h"

//This is determined by a transmit rate of TX of 1MHz, a symbol rate of 800 Ksps, a packet size of 1047 symbols, and a RX of 1MHz
#define SIZE_BLOCKS_TO_STORE 1500 //when power detector is happy - this is prior to upsample!

class DETECTOR
{
public:
//This is terrible but an assumption of only one POWER_THREAD makes it okay - should be changed later though
static float 	alpha;
static float 	power_detector;
static float 	power_threshold;
static bool 	block_stored;
static size_t	block_index;
static size_t	packet_num;
//spawns main RX thread according to passes parameters
static void energyDetectorThread(
        MutexFIFO<std::vector<rx_cpu_format>>& data_fifo,
	MutexFIFO<std::vector<rx_cpu_format>>& data_fifo2
//	size_t& sblocks,
//	SharedPrinter& printer
	);
};
