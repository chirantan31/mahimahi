

#include <vector>
#include <numeric>
#include <iostream>
#include <chrono>
#include <unordered_map>
#include <string>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <tuple>
#include <mutex>

using namespace std;
using namespace std::chrono;

class TimeLogger
{
public:
    static unordered_map<std::string, time_point<system_clock>> rttStart;
    static unordered_map<std::string, time_point<system_clock>> objectLoadStart;
    static unordered_map<std::string, vector<float>> rtts;
    static vector<tuple<std::string, std::string, float>> finals;
    static int testInt;
    static std::mutex start_rtt_mutex;
    static std::mutex stop_rtt_mutex;
    static std::mutex start_obj_timer_mutex;
    static std::mutex stop_obj_timer_mutex;
    
    static void test()
    {
        std::cout << testInt;
    }

    static void startRttTimer(std::string addr)
    {
        std::lock_guard<std::mutex> guard(start_rtt_mutex);
        rttStart[addr] = system_clock::now();
    }

    static void stopRttTimer(std::string addr)
    {
        float value = (system_clock::now() - rttStart[addr]) / milliseconds(1);
        // cout << value << std::endl;
        std::lock_guard<std::mutex> guard(stop_rtt_mutex);
        rtts[addr].push_back(value);
    }

    static void startObjLoadTimer(std::string request)
    {
        int firstSpace = request.find(' ', 0);
        int secondSpace = request.find(' ', firstSpace + 1);
        std::string methodType = request.substr(0, firstSpace);
        std::string requestUrl = request.substr(firstSpace + 1, secondSpace - firstSpace - 1);
        std::lock_guard<std::mutex> guard(start_obj_timer_mutex);
        objectLoadStart[requestUrl] = system_clock::now();
    }

    static void stopObjLoadTimer(std::string request)
    {
        int firstSpace = request.find(' ', 0);
        int secondSpace = request.find(' ', firstSpace + 1);
        std::string methodType = request.substr(0, firstSpace);
        std::string requestUrl = request.substr(firstSpace + 1, secondSpace - firstSpace - 1);
        float value = (system_clock::now() - objectLoadStart[requestUrl]) / milliseconds(1);
        std::lock_guard<std::mutex> guard(stop_obj_timer_mutex);
        finals.push_back(make_tuple(methodType, requestUrl, value));
    }

    static void printFinals()
    {
        for (auto element : TimeLogger::finals)
        {
            std::cout << get<0>(element) << "::" << get<1>(element) << "::" << get<2>(element) << std::endl;
        }
    }

    static void print_map(std::string directory)
    {
        ofstream file(directory + "/request_delays.txt");
        if (file.is_open())
        {
            for (auto element : TimeLogger::finals)
            {
                file << get<0>(element) << "\t" << get<1>(element) << "\t" << get<2>(element) << std::endl;
            }
            file.close();
        }
    }

    static void save_rtt(std::string directory)
    {
        ofstream file(directory + "/rtts.txt");
        if (file.is_open())
        {
            file << "IP\tPort\tMin\tMax\tMean\tStdDev" << endl;
            for (auto element : TimeLogger::rtts)
            {
                std::vector<float> v = element.second;
                
                double sum = std::accumulate(v.begin(), v.end(), 0.0);
                double mean = sum / v.size();

                std::vector<double> diff(v.size());
                std::transform(v.begin(), v.end(), diff.begin(),
                               std::bind2nd(std::minus<double>(), mean));
                double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
                double stdev = std::sqrt(sq_sum / v.size());
                float max = *max_element(std::begin(v), std::end(v));
                float min = *min_element(std::begin(v), std::end(v));
                auto ip = element.first.substr(0, element.first.find_first_of(':'));
                auto port = element.first.substr(element.first.find_first_of(':') + 1);
                file << ip << "\t" << port << "\t" << min << "\t" << max << "\t" << mean << "\t" << stdev << endl;
            }
            file.close();
        }
    }
};