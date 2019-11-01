

#include <vector>
#include <numeric>
#include <iostream>
#include <chrono>
#include <unordered_map>
#include <string>
#include <tuple>

using namespace std;
using namespace std::chrono;

class TimeLogger
{
public:
    static unordered_map<std::string, time_point<system_clock>> starts;
    static vector<tuple<std::string, std::string, std::string, float>> finals;
    static int testInt;
    // static void start(string addr);
    // static void end(string addr);
    // static void printFinals();
    static void test()
    {
        std::cout << testInt;
    }

    static void start(std::string addr)
    {
        starts[addr] = system_clock::now();
    }

    static void end(std::string addr, std::string request)
    {
        float value = (system_clock::now() - starts[addr]) / milliseconds(1);
        int firstSpace = request.find(" ", 0);
        int secondSpace = request.find(" ", firstSpace + 1);
        std::string methodType = request.substr(0, firstSpace);
        std::string requestUrl = request.substr(firstSpace + 1, secondSpace);
        finals.push_back(make_tuple(addr, methodType, requestUrl, value));
    }

    static void printFinals()
    {
        for (auto element : TimeLogger::finals)
        {
            std::cout << get<0>(element) << "::" << get<1>(element) << "::" << get<2>(element) << get<3>(element) << std::endl;
        }
    }
};