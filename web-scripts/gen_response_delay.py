#!/usr/bin/env python
# -*- mode: python; coding: utf-8; fill-column: 80; -*-
#
# gen_packet_delay_rules.py
#

"""
gen_packet_delay_rules.py
Generate delay rules for adv-delay-shell within Mahimahi.
"""

import io
import math
import os
import sys

def load_delay(delay_path):
    delay = open(delay_path, "r")
    result = dict()
    line = delay.readline()
    while line:
        splits = line.split("\t")
        key = splits[0] + "\t" + splits[1]
        result[key] = int(splits[2])
        line = delay.readline()
    delay.close()
    return result

def generate_response_delay(recorded_delay, replayed_delay):
    for key in replayed_delay.keys():
        if (key in recorded_delay):
            response_delay = recorded_delay[key] - replayed_delay[key]
        else:
            response_delay = 0
        print(key + "\t" + str(response_delay))


def main(args):
    recorded_delay_path, replayed_delay_path = args
    recorded_delay = load_delay(recorded_delay_path)
    replayed_delay = load_delay(replayed_delay_path)
    generate_response_delay(recorded_delay, replayed_delay)

if __name__ == '__main__':
    prog = sys.argv[0]
    args = sys.argv[1:]
    num_args = len(args)
    if num_args != 2:
        sys.stderr.write((u"Usage: %s" +
                          u" <recorded-delay> <replayed-delay>\n") %
                         (prog))
        sys.exit(1)

    main(args)
