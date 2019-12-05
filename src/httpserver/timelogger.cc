#include <timelogger.hh>

int TimeLogger::testInt;
unordered_map<std::string, time_point<system_clock>> TimeLogger::rttStart;
unordered_map<std::string, time_point<system_clock>> TimeLogger::objectLoadStart;
vector<tuple<std::string, std::string, float>> TimeLogger::finals;
unordered_map<std::string, vector<float>> TimeLogger::rtts;
std::mutex TimeLogger::start_rtt_mutex;
std::mutex TimeLogger::stop_rtt_mutex;
std::mutex TimeLogger::start_obj_timer_mutex;
std::mutex TimeLogger::stop_obj_timer_mutex;
    
    