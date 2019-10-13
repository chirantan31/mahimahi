/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include<algorithm>
#include<arpa/inet.h>
#include<fstream>
#include<iomanip>
#include<iostream>
#include<iterator>
#include<sstream>
#include<vector>

#include "delay_rule.hh"

using namespace std;

bool DelayRule::isPortReqd()
{
    return _port == DelayRule::WILDCARD_PORT;
}

ProtoRules_t parse_rules(const string & rules_file)
{
  ifstream fd(rules_file.c_str());
  if (!fd.good()) {
    throw runtime_error("Rules file '" + rules_file + "' does not exist!");
  }

  ProtoRules_t rules;

  string line;
  uint32_t rule_num = 0;
  while (std::getline(fd, line)) {
      rule_num++;
      istringstream iss(line);

      vector<string> cols {istream_iterator<string>{iss},
              istream_iterator<string>{}};

      const vector<string>::size_type num_cols = cols.size();
      if (num_cols < 5 || num_cols > 9) {
          ostringstream err;
          err << "Failed to parse rule#" << rule_num << ":"
              << " Invalid number of fields!";
          throw runtime_error(err.str());
      }

      uint32_t proto = std::atoi(cols.at(0).c_str());
      if (proto > 255) {
          ostringstream err;
          err << "Invalid protocol number '" << proto << "'"
              << " in rule #" << rule_num << "!";
          throw runtime_error(err.str());
      }

      string addr = cols.at(1);
      // TODO: Add support for ipv6
      uint32_t ipv4;
      int e;
      if ((e = inet_pton(AF_INET, addr.c_str(), &ipv4)) <= 0) {
          ostringstream err;
          if (e == 0) {
              err << "IPv4 address '"
                  << addr
                  << "' not in presentation format!";
          } else {
              err << "'"
                  << addr
                  << "' is not a valid IPv4 address!";
          }
          throw runtime_error(err.str());
      }

      uint32_t port = std::atoi(cols.at(2).c_str());
      if (port > 65535) {
          ostringstream err;
          err << "Invalid port number '" << port << "'"
              << " in rule #" << rule_num << "!";
          throw runtime_error(err.str());
      }

      uint64_t fwd_delay = std::atol(cols.at(3).c_str());
      uint64_t rev_delay = std::atol(cols.at(4).c_str());

      if (num_cols >= 7) {
          // Threshold after which forward path delay should be switched to a
          //  different value.
          uint64_t fwd_lim = std::atol(cols.at(5).c_str());
          uint64_t fwd_alt_delay = std::atol(cols.at(6).c_str());

          if (num_cols == 9) {
              // Threshold after which reverse path delay should be switched to
              //  a different value.
              uint64_t rev_lim = std::atol(cols.at(5).c_str());
              uint64_t rev_alt_delay = std::atol(cols.at(6).c_str());

              rules[proto][ipv4][port] =
                  DelayRule(proto, ipv4, port, fwd_delay, rev_delay,
                            fwd_lim, fwd_alt_delay,
                            rev_lim, rev_alt_delay);
          } else {
              rules[proto][ipv4][port] =
                  DelayRule(proto, ipv4, port, fwd_delay, rev_delay,
                            fwd_lim, fwd_alt_delay);
          }
      } else {
          rules[proto][ipv4][port] =
              DelayRule(proto, ipv4, port, fwd_delay, rev_delay);
      }

  }
  return rules;
}

void show_rules(const DelayRules_t & rules)
{
    char ipv4[INET_ADDRSTRLEN];

    uint32_t rule_num = 0;
    for (auto const &proto_rules : rules) {
        for (auto const &ip_rules : proto_rules.second) {
            for (auto const &port_rules : ip_rules.second) {
                rule_num++;
                if (inet_ntop(AF_INET, &ip_rules.first, ipv4,
                              INET_ADDRSTRLEN) == NULL) {
                    throw runtime_error("Invalid rule encountered!");
                }

                const uint64_t MAX_LIM = std::numeric_limits<uint64_t>::max();

                const DelayRule& rule = port_rules.second;
                cout << "#- Rule#"
                     << setfill('0') << setw(3)
                     << rule_num << "  "
                     << setfill(' ') << setw(3)
                     << unsigned(proto_rules.first) << " "
                     << setw(15) << ipv4
                     << "(" << setw(10) << ip_rules.first << ")" << " "
                     << setw(5)
                     << (port_rules.first == 0 ? "*" :
                         to_string(port_rules.first)) << " "
                     << setw(5) << rule.fwd_delay() << "ms "
                     << setw(5) << rule.rev_delay() << "ms   "
                     << "F@" << setw(10)
                     << (rule.fwd_lim() == MAX_LIM ? "*" :
                         to_string(rule.fwd_lim())) << " "
                     << setw(5) << rule.fwd_alt_delay() << "ms   "
                     << "B@" << setw(10)
                     << (rule.rev_lim() == MAX_LIM ? "*" :
                         to_string(rule.rev_lim())) << " "
                     << setw(5) << rule.rev_alt_delay() << "ms "
                     << endl;
            }
        }
    }
}
