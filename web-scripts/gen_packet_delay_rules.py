#!/usr/bin/env python
# -*- mode: python; coding: utf-8; fill-column: 80; -*-
#
# gen_packet_delay_rules.py
#

"""
gen_packet_delay_rules.py
Generate delay rules for adv-delay-shell within Mahimahi.
"""

from collections import namedtuple
import io
import math
import os
import re
import socket
import sys
from urlparse import urlparse


SPACE = u' '
WHITESPACE = re.compile(r'\s+')

# RTT file name.
# RTTs to origin/edge servers.
RTT_FILE = "rtts.txt"

# RTT data.
RecRTT = namedtuple('RecRTT',
                    ('proto', 'ip', 'port', 'rtt'))

# Link delay data.
LinkDelay = namedtuple('LinkDelay',
                       ('rtt_original', 'rtt_adjusted', 'up', 'down'))


def get_link_delays(rtt, mult=1.0):
    """Compute the uplink and downlink delays given the RTT.
    """
    _rtt = math.ceil(rtt)*mult
    up = _rtt * 0.5
    down = _rtt * 0.5
    return LinkDelay(rtt, _rtt, up, down)


RULEBOOK = {
    'base'                :
    lambda rtt: ld_to_str(get_link_delays(rtt)),
    'without-delay'                :
    lambda rtt: ld_to_str(get_link_delays(rtt, 0)),
    'network-delay'                :
    lambda rtt: ld_to_str(get_link_delays(rtt)),
    'base'                :
    lambda rtt: ld_to_str(get_link_delays(rtt)),
    'base-network-0.33'                :
    lambda rtt: ld_to_str(get_link_delays(rtt, 0.33)),
    'base-server-0.5'                :
    lambda rtt: ld_to_str(get_link_delays(rtt)),
    'train'                :
    lambda rtt: ld_to_str(get_link_delays(rtt)),
}


def srv_rtts(dir_path, domain):
    """Retrieve RTTs to origin/edge servers.
    """
    rtts = []
    rtts_file = dir_path + "/data/" + RTT_FILE;
    file = open(rtts_file, "r")
    # Skip the first file
    line = file.readline()
    line = file.readline()
    while (line):
        data = line.strip().split();
        rtt = RecRTT(socket.IPPROTO_TCP, data[0], int(data[1]), float(data[4]))
        rtts.append(rtt)
        line = file.readline()
    return rtts


def get_rtts(dir_path, domain):
    """Retrieve the RTTs measured on all connections made to fetch the content for the corresponding page.
    NOTE: Website domains uniquely identify the Webpages that were fetched.
    """
    rtt_recs = []
    for rec in srv_rtts(dir_path, domain):
        rtt_recs.append(rec)
    return rtt_recs


def write_rules(out, rtt_recs, gen):
    """Write the delay rules to a file.
    """
    # <protocol> <ip> <port> <fwd-delay> <rev-delay> \
    #   <fwd-lim> <fwd-alt-delay> \
    #   <rev-lim> <rev-alt-delay>
    for rec in rtt_recs:
        out.write(u"%d %s %d %s\n" %
                  (rec.proto, rec.ip, rec.port, gen(rec.rtt)))

def ld_to_str(ld):
    """Convert link-delay data to string.
    """
    return SPACE.join(("%.0f" % d) for d in (ld.up, ld.down))

def main(args):
    page_url, rec_path, config, out_file = args
    domain = urlparse(page_url).netloc
    file = open(out_file, "w")
    write_rules(file, get_rtts(rec_path, domain), RULEBOOK[config])
    file.close()


if __name__ == '__main__':
    prog = sys.argv[0]
    args = sys.argv[1:]
    num_args = len(args)
    if num_args != 4:
        sys.stderr.write((u"Usage: %s" +
                          u" <URL> <record-path> <config> <out-file>\n") %
                         (prog))
        sys.exit(1)

    main(args)
