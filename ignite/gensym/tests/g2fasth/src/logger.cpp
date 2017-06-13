#include <iostream>
#include <time.h>
#include "logger.hpp"

using namespace std;
using namespace g2::fasth;

logger::logger(g2::fasth::log_level logLevel)
    :d_loglevel(logLevel)
{
}

void logger::add_output_stream(std::ostream &output_stream, log_level level)
{
    add_output_stream(&output_stream, level);
}

void logger::add_output_stream(std::ostream *output_stream, log_level level)
{
    stream_info stream_info(output_stream, level);
    tthread::lock_guard<tthread::mutex> lg(d_mutex);
    d_outputStreams.push_back(stream_info);
}

void logger::log(log_level log_level, std::string text)
{
    tthread::lock_guard<tthread::mutex> lg(d_mutex);
    // If the log level passed is greater then permissible log level of suite, quit.
    if (log_level > d_loglevel)
    {
        return;
    }

    for (auto iter = d_outputStreams.begin(); iter < d_outputStreams.end(); ++iter)
    {
        // If log level of iterator is greater then permissible log level of suite, quit.
        if (iter->level > d_loglevel)
        {
            continue;
        }
        // If log level passed is greater then permissible log level of iterator, quit.
        else if (log_level > iter->level)
        {
            continue;
        }
        auto ptr_stream = iter->pStream;
        auto written = write(text.c_str(), false, ptr_stream);
        if (written)
        {
            (*ptr_stream) << endl;
            ptr_stream->flush();
        }
    }
}

bool logger::write(const char* data, bool written, std::ostream* stream)
{
    if (written == true)
    {
        (*stream) << " ";
    }
    (*stream) << data;
    return true;
}