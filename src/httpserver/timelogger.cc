#include <timelogger.hh>

int TimeLogger::testInt;
unordered_map<std::string, time_point<system_clock>> TimeLogger::starts;
vector<tuple<std::string, std::string, std::string, float>> TimeLogger::finals;
    