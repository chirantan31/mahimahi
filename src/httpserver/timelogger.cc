#include <timelogger.hh>

int TimeLogger::testInt;
unordered_map<std::string, time_point<system_clock>> TimeLogger::rttStart;
unordered_map<std::string, time_point<system_clock>> TimeLogger::objectLoadStart;
vector<tuple<std::string, std::string, float>> TimeLogger::finals;
unordered_map<std::string, vector<float>> TimeLogger::rtts;
    