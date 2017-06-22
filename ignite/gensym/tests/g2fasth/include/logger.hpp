#pragma once
#ifndef INC_LIBG2FASTH_LOGGER_H
#define INC_LIBG2FASTH_LOGGER_H

#include <iostream>
#include <vector>
#include "tinythread.h"
#include "g2fasth_enums.hpp"

namespace g2 {
namespace fasth {
/**
* This class is responsible for logging to collection of streams.
* The amount of data logged depends on log level set through the constructor.
*/
class logger {
private:
    struct stream_info {
        std::ostream* pStream;
        g2::fasth::log_level level;
        stream_info(std::ostream* pStream, log_level level)
            : pStream(pStream), level(level) {
        }
    };
public:
    /**
    * Accepts log level of application.
    */
    logger(g2::fasth::log_level);
    /**
    * This method accepts reference output stream instance and log level associated with it.
    */
    void add_output_stream(std::ostream &, log_level);
    /**
    * This method accepts address of output stream instance and log level associated with it.
    */
    void add_output_stream(std::ostream*, log_level);
    /**
    * This method logs data with desired log level.
    */
    void log(log_level, std::string);
private:
    tthread::mutex d_mutex;
    log_level d_loglevel;
    std::vector<stream_info> d_outputStreams;
    bool write(const char* data, bool written, std::ostream* stream);
};
}
}

#endif // !INC_LIBG2FASTH_LOGGER_H