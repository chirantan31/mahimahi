/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <arpa/inet.h>
#include <fstream>
#include <iostream>
#include <limits>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <sstream>
#include <string>

#include "adv_delay_queue.hh"
#include "delay_rule.hh"
#include "pktinfo.hh"
#include "timestamp.hh"

using namespace std;

void AdvDelayQueue::save_byte_counters(void)
{
    ofstream f;
    if (uplink_) {
        f.open(path_prefix_ + ".bytes_uplink");
    } else {
        f.open(path_prefix_ + ".bytes_downlink");
    }
    f << num_bytes_ << endl;
    f.close();
}

void AdvDelayQueue::save_byte_per_conn_counters(void)
{
    ofstream g;
    if (uplink_) {
        g.open(path_prefix_ + ".bytes_per_conn_uplink");
    } else {
        g.open(path_prefix_ + ".bytes_per_conn_downlink");
    }

    // FIXME: Add support for IPv6.
    char ipv4[INET_ADDRSTRLEN];
    for (auto const &per_conn : byte_counters_) {
        for (auto const &per_port : per_conn.second) {
            if (inet_ntop(AF_INET, &per_conn.first, ipv4,
                          INET_ADDRSTRLEN) == NULL) {
                throw runtime_error("Invalid IP address encountered!");
            }
            g << ipv4 << ":" << per_port.first
              << " " << per_port.second << endl;
        }
    }
    g.close();
}

void AdvDelayQueue::save_pkt_per_conn_counters(void)
{
    ofstream h;
    if (uplink_) {
        h.open(path_prefix_ + ".pkts_per_conn_uplink");
    } else {
        h.open(path_prefix_ + ".pkts_per_conn_downlink");
    }

    // FIXME: Add support for IPv6.
    char ipv4[INET_ADDRSTRLEN];
    for (auto const &per_conn : pkt_counters_) {
        for (auto const &per_port : per_conn.second) {
            if (inet_ntop(AF_INET, &per_conn.first, ipv4,
                          INET_ADDRSTRLEN) == NULL) {
                throw runtime_error("Invalid IP address encountered!");
            }
            h << ipv4 << ":" << per_port.first
              << " " << per_port.second << endl;
        }
    }
    h.close();
}

AdvDelayQueue::~AdvDelayQueue()
{
    save_byte_counters();
    save_byte_per_conn_counters();
    save_pkt_per_conn_counters();
}

uint64_t AdvDelayQueue::get_delay_for_tcp(const PortRules_t& port_rules,
                                          const struct tcphdr *h,
                                          const std::string::size_type& sz)
{    
    PortRules_t::const_iterator port_rules_iter =
        port_rules.find(uplink_ ? ntohs(h->th_dport) :
                        ntohs(h->th_sport));

    if (port_rules_iter == port_rules.end()) {
        // No matching rule.
        return delay_ms_;
    }

    const DelayRule& rule = (*port_rules_iter).second;

    // Bytes transferred on the link so far.
    const uint64_t bytes_sent = byte_counters_[rule.ip()][rule.port()];

    // Update byte counter with the size of the current packet.
    byte_counters_[rule.ip()][rule.port()] += sz;

    // Update packet counter
    pkt_counters_[rule.ip()][rule.port()] += 1;

    if (uplink_) {
        // Uplink

        if (bytes_sent < rule.fwd_lim()) {
            return rule.fwd_delay();
        } else {
            return rule.fwd_alt_delay();
        }
    }

    // Downlink
    if (bytes_sent < rule.rev_lim()) {
        return rule.rev_delay();
    } else {
        return rule.rev_alt_delay();
    }
}

uint64_t AdvDelayQueue::get_delay_for_udp(const PortRules_t& port_rules,
                                          const struct udphdr *h)
{
    PortRules_t::const_iterator port_rules_iter =
        port_rules.find(uplink_ ? ntohs(h->uh_dport) :
                        ntohs(h->uh_sport));

    if (port_rules_iter == port_rules.end()) {
        // No matching rule.
        return delay_ms_;
    }
    
    return uplink_ ? (*port_rules_iter).second.fwd_delay() :
        (*port_rules_iter).second.rev_delay();
}

uint64_t AdvDelayQueue::get_delay_for_ipv4(const struct iphdr *h,
                                           const std::string::size_type& sz)
{
    ProtoRules_t::const_iterator proto_rules_iter = rules_.find(h->protocol);
    if (proto_rules_iter == rules_.end()) {
        return delay_ms_;
    }

    const IPRules_t ip_rules = (*proto_rules_iter).second;
    IPRules_t::const_iterator ip_rules_iter =
        ip_rules.find(uplink_ ? h->daddr : h->saddr);
    
    if (ip_rules_iter == ip_rules.end()) {
        // No matching rule in the forward or reverse directions.
        return delay_ms_;
    }

    const PortRules_t port_rules = (*ip_rules_iter).second;
    // Check for wild card rule.
    PortRules_t::const_iterator port_rules_iter = port_rules.find(0);
    if (port_rules_iter != port_rules.end()) {
        return uplink_ ? (*port_rules_iter).second.fwd_delay() :
            (*port_rules_iter).second.rev_delay();
    }

    const char *pkt = (const char *) h;

    switch (h->protocol) {
    case IPPROTO_IP:
        // Protocol not supported.
        break;
    case IPPROTO_ICMP:
        // For ICMP, ports don't matter.
        return uplink_ ? port_rules.at(DelayRule::WILDCARD_PORT).fwd_delay() :
            port_rules.at(DelayRule::WILDCARD_PORT).rev_delay();
    case IPPROTO_IGMP:
    case IPPROTO_IPIP:
        // Protocol not supported.
        break;
    case IPPROTO_TCP: {
        const struct tcphdr *tcph = (struct tcphdr*) (pkt + h->ihl*4);
        const uint64_t delay = get_delay_for_tcp(port_rules, tcph, sz);
        // print_tcp_endpoints(h, delay);
        return delay;
    }
    case IPPROTO_EGP:
        // Protocol not supported.
        break;
    case IPPROTO_PUP:
        // Protocol not supported.
        break;
    case IPPROTO_UDP: {
        const struct udphdr *udph = (struct udphdr*) (pkt + h->ihl*4);
        const uint64_t delay = get_delay_for_udp(port_rules, udph);
        // print_udp_endpoints(h, delay);
        return delay;
    }
    case IPPROTO_IDP:
    case IPPROTO_TP:
    case IPPROTO_DCCP:
    case IPPROTO_IPV6:
    case IPPROTO_RSVP:
    case IPPROTO_GRE:
    case IPPROTO_ESP:
    case IPPROTO_AH:
    case IPPROTO_MTP:
    case IPPROTO_BEETPH:
    case IPPROTO_ENCAP:
    case IPPROTO_PIM:
    case IPPROTO_COMP:
    case IPPROTO_SCTP:
    case IPPROTO_UDPLITE:
    case IPPROTO_RAW:
default:
        // Protocol not supported.
        break;
    }
    
    return delay_ms_;
}

uint64_t AdvDelayQueue::get_delay_for_ipv6(const struct iphdr *h,
                                           const std::string::size_type& sz)
{
    if (h->protocol != 6) {
        ostringstream err;
        err << "Data (sz: " << sz << ") is not an IPv6 packet!";
        throw runtime_error(err.str());
    }
    // TODO: Add support for IPv6 addresses as well.
    return delay_ms_;
}

uint64_t AdvDelayQueue::get_delay_for(const char *pkt, const std::string::size_type& sz)
{
    // NOTE: IFF_NO_PI is unset, and hence, ignore first four bytes.
    //  If IFF_NO_PI is unset, first four bytes contain two flag bytes and two
    //   protocol bytes.
    // printf("#- bytes[0:4]- flag[%02x, %02x]  protocol[%02x, %02x]\n",
    //        *(pkt + 0), *(pkt + 1), *(pkt + 2), *(pkt + 3));

    const struct iphdr *h = (struct iphdr *)(pkt + 4);
    
    if (h->version != 4) {
        return get_delay_for_ipv6(h, sz);
    } else {
        return get_delay_for_ipv4(h, sz);
    }
}

void AdvDelayQueue::read_packet(const string& contents)
{
    num_bytes_ += contents.size();
    if (contents.size() < sizeof(struct iphdr)) {
        throw new runtime_error("Packet too small!");
    }
    // Perform deep packet inspection to identify which delay rule to apply.
    uint64_t pkt_delay = get_delay_for(contents.data(), contents.size());
    packet_queue_.emplace(timestamp() + pkt_delay, contents);
}

void AdvDelayQueue::write_packets(FileDescriptor& fd)
{
    while ((!packet_queue_.empty())
           && (packet_queue_.top().first <= timestamp())) {
        fd.write(packet_queue_.top().second);
        packet_queue_.pop();
    }
}

unsigned int AdvDelayQueue::wait_time( void ) const
{
    if (packet_queue_.empty()) {
        return numeric_limits<uint16_t>::max();
    }

    const auto now = timestamp();

    if (packet_queue_.top().first <= now) {
        return 0;
    } else {
        return packet_queue_.top().first - now;
    }
}
