/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef DELAY_RULE_HH
#define DELAY_RULE_HH

#include<limits>
#include<map>

class DelayRule
{
    uint8_t _proto;
    uint32_t _ip;  // NOTE: Currently, only IPv4 addresses are supported!
    uint16_t _port;
    uint64_t _fwd_delay;
    uint64_t _rev_delay;
    uint64_t _fwd_lim;
    uint64_t _fwd_alt_delay;
    uint64_t _rev_lim;
    uint64_t _rev_alt_delay;

public:
    DelayRule()
        :_proto(0), _ip(0), _port(0), _fwd_delay(0), _rev_delay(0),
         _fwd_lim(std::numeric_limits<uint64_t>::max()), _fwd_alt_delay(0),
         _rev_lim(std::numeric_limits<uint64_t>::max()), _rev_alt_delay(0)
    { /* -- empty -- */ }

    DelayRule(uint8_t proto, uint32_t ip, uint16_t port,
              uint64_t fwd_delay, uint64_t rev_delay)
        :_proto(proto), _ip(ip), _port(port),
         _fwd_delay(fwd_delay), _rev_delay(rev_delay),
         _fwd_lim(std::numeric_limits<uint64_t>::max()),
         _fwd_alt_delay(fwd_delay),
         _rev_lim(std::numeric_limits<uint64_t>::max()),
         _rev_alt_delay(rev_delay)
    { /* -- empty -- */ }

    DelayRule(uint8_t proto, uint32_t ip, uint16_t port,
              uint64_t fwd_delay, uint64_t rev_delay,
              uint64_t fwd_lim, uint64_t fwd_alt_delay)
        :_proto(proto), _ip(ip), _port(port),
         _fwd_delay(fwd_delay), _rev_delay(rev_delay),
         _fwd_lim(fwd_lim),
         _fwd_alt_delay(fwd_alt_delay),
         _rev_lim(std::numeric_limits<uint64_t>::max()),
         _rev_alt_delay(rev_delay)
    { /* -- empty -- */ }

    DelayRule(uint8_t proto, uint32_t ip, uint16_t port,
              uint64_t fwd_delay, uint64_t rev_delay,
              uint64_t fwd_lim, uint64_t fwd_alt_delay,
              uint64_t rev_lim, uint64_t rev_alt_delay)
        :_proto(proto), _ip(ip), _port(port),
         _fwd_delay(fwd_delay), _rev_delay(rev_delay),
         _fwd_lim(fwd_lim),
         _fwd_alt_delay(fwd_alt_delay),
         _rev_lim(rev_lim),
         _rev_alt_delay(rev_alt_delay)
    { /* -- empty -- */ }

    static const uint16_t WILDCARD_PORT = 0;

    uint8_t proto() const { return _port; };
    uint32_t ip() const { return _ip; };
    uint16_t port() const { return _port; };
    uint64_t fwd_delay() const { return _fwd_delay; };
    uint64_t rev_delay() const { return _rev_delay; };
    uint64_t fwd_lim() const { return _fwd_lim; };
    uint64_t fwd_alt_delay() const { return _fwd_alt_delay; };
    uint64_t rev_lim() const { return _rev_lim; };
    uint64_t rev_alt_delay() const { return _rev_alt_delay; };


    // True, if port number is zero; false, otherwise
    bool isPortReqd();
};

/**
 * Parse file containing delay rules.
 *
 * There should one rule per line in the file, and the rules are expected to be
 *  in the following format.
 *
 *  <proto> <IP> <port> <to-delay> <from-delay>
 *  where,
 *       <proto>: Standard protocol numbers as defined in [1].
 *          <IP>: IPv4 or IPv6 address.
 *        <port>: port number [0, 65535]
 *    <to-delay>: delay in milliseconds on the forward path.
 *  <from-delay>: delay in milliseconds on the reverse path.
 *
 * Use '0' for port number when the port numbers are not applicable to the
 *  concerned rule, viz., ICMP.
 * If a floating-point value is specified for delays, the number will be
 *  truncated to an integer.
 *
 * [1]: http://www.iana.org/assignments/protocol-numbers/protocol-numbers.xhtml
 */
typedef std::map<uint16_t, DelayRule> PortRules_t;
typedef std::map<uint32_t, PortRules_t> IPRules_t;
typedef std::map<uint8_t, IPRules_t> ProtoRules_t;
typedef ProtoRules_t DelayRules_t;
DelayRules_t parse_rules(const std::string & rules_file);

// Print the delay rules.
void show_rules(const DelayRules_t & rules);

#endif /* DELAY_RULE_HH */
