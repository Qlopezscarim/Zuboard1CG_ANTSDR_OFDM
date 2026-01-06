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
#include "RX.h"

class FILE_GEN
{
public:
//spawns main RX thread according to passes parameters
static void fileThread(
        MutexFIFO<std::vector<std::complex<float>>>& data_fifo,
	MutexFIFO<std::vector<std::complex<float>>>& data_fifo2,
        std::string file_name
        );
};
