/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef ADV_DELAY_QUEUE_HH
#define ADV_DELAY_QUEUE_HH

#include <cstdint>
#include <queue>
#include <string>
#include <vector>

#include "delay_rule.hh"
#include "file_descriptor.hh"

class AdvDelayQueue
{
    // NOTE: Cannot inherit from DelayQueue since the packet queue is private.
private:
    uint64_t delay_ms_;

    typedef std::pair<uint64_t, std::string> DelayPktPair_t;

    class DelayPktPairCmp final
    {
    public:
        bool operator() (const DelayPktPair_t& lhs, const DelayPktPair_t& rhs)
        {
            // NOTE: Packets are in **ascending order** of release times.
            return lhs.first > rhs.first;
        }
    };

    // Release timestamp, contents.
    std::priority_queue<DelayPktPair_t,
                        std::vector<DelayPktPair_t>,
                        DelayPktPairCmp> packet_queue_;

    // Packet delay rules.
    const DelayRules_t rules_;

    typedef std::map<uint16_t, uint64_t> PerPortBytes_t;
    typedef std::map<uint32_t, PerPortBytes_t> PerConnBytes_t;
    // Bytes transferred per TCP connection (uplink or downlink).
    PerConnBytes_t byte_counters_;

    typedef std::map<uint16_t, uint64_t> PerPortPkts_t;
    typedef std::map<uint32_t, PerPortPkts_t> PerConnPkts_t;
    // Packets transferred per TCP connection (uplink or downlink).
    PerConnBytes_t pkt_counters_;

    // True, if queue is used for uplink; false, otherwise.
    const bool uplink_;

    // Number of bytes transferred using the queue.
    uint64_t num_bytes_;

    // Output path prefix.
    const std::string path_prefix_;

    void save_byte_counters(void);
    void save_byte_per_conn_counters(void);
    void save_pkt_per_conn_counters(void);

protected:
    uint64_t get_delay_for_tcp(const PortRules_t& port_rules,
                               const struct tcphdr *h,
                               const std::string::size_type& sz);
    uint64_t get_delay_for_udp(const PortRules_t& port_rules,
                               const struct udphdr *h);
    uint64_t get_delay_for_ipv4(const struct iphdr *h,
                                const std::string::size_type& sz);
    uint64_t get_delay_for_ipv6(const struct iphdr *h,
                                const std::string::size_type& sz);
    uint64_t get_delay_for(const char *h,
                           const std::string::size_type& sz);

public:
    AdvDelayQueue(const uint64_t& delay_ms, const DelayRules_t& rules,
                  const bool& uplink, const std::string& path_prefix)
        : delay_ms_(delay_ms), packet_queue_(),
          rules_(rules), byte_counters_(), pkt_counters_(),
          uplink_(uplink), num_bytes_(0),
          path_prefix_(path_prefix)
    {
        for (auto const &proto_rules : rules_) {
            for (auto const &ip_rules : proto_rules.second) {
                for (auto const &port_rules : ip_rules.second) {
                    byte_counters_[ip_rules.first][port_rules.first] = 0;
                }
            }
        }
    }

    ~AdvDelayQueue();

    void read_packet(const std::string & contents);

    void write_packets(FileDescriptor & fd);

    unsigned int wait_time(void) const;

    bool pending_output(void) const { return wait_time() <= 0; }

    static bool finished(void) { return false; }
};

#endif /* ADV_DELAY_QUEUE_HH */