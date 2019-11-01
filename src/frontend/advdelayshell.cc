/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <vector>
#include <string>

#include "delay_rule.hh"
#include "adv_delay_queue.hh"
#include "util.hh"
#include "ezio.hh"
#include "packetshell.cc"

using namespace std;

int main(int argc, char *argv[])
{
    try {
        /* clear environment while running as root */
        char ** const user_environment = environ;

        environ = nullptr;

        check_requirements(argc, argv);

        if (argc < 4) {
            throw runtime_error("Usage: " + string(argv[0]) +
                                " delay-milliseconds" +
                                " rule-file" +
                                " out-path-prefix" +
                                " [command...]");
        }

        const uint64_t delay_ms = myatoi(argv[1]);
        string rules_file = argv[2];
        DelayRules_t rules = parse_rules(rules_file);
        string path_prefix = argv[3];

        vector<string> command;

        if (argc == 4) {
            command.push_back(shell_path());
        } else {
            for (int i = 4; i < argc; i++) {
                command.push_back(argv[i]);
            }
        }

        PacketShell<AdvDelayQueue> delay_shell_app("adv-delay", user_environment);
        const bool is_uplink = true;
        delay_shell_app.start_uplink("[adv-delay <def: " + to_string(delay_ms) + " ms>] ",
                                     command, delay_ms, rules, is_uplink, path_prefix);
        delay_shell_app.start_downlink(delay_ms, rules, !is_uplink, path_prefix);

        
        return delay_shell_app.wait_for_exit();
    } catch ( const exception & e ) {
        print_exception(e);
        return EXIT_FAILURE;
    }
}
