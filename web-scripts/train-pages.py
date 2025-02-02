#!/usr/bin/env python
# -*- mode: python; coding: utf-8; fill-column: 80; -*-
#
# replay-pages.py
# Created by William Sentosa
#

import sys
import os
import subprocess
from urlparse import urlparse
import shutil

# CONSTANTS
BROWSER_WAIT_MARK = "load"
WEB_PAGE_LOAD_SCRIPT = "web_page_load.js"
GEN_PACKET_RULE_SCRIPT = "gen_packet_delay_rules.py"
GEN_RESPONSE_DELAY_SCRIPT = "gen_response_delay.py"
REPLAY_CONFIG = "train"
NUM_TRIALS = 1
DEFAULT_DELAY = "0"
REQUEST_DELAYS_NAME = "request_delays.txt"
SERVER_DELAYS_NAME = "server-delays.txt"
RECORDED_DELAYS_NAME = "request_delays.txt"

# Load list of urls from file
# File format:
# https://www.google.com
# https://www.yahoo.com
def load_urls(filename):
	result = []
	with open(filename) as file:
		line = file.readline()
		while (line):
			result.append(line.strip())
			line = file.readline()
	return result

def main(args):
    url_file_path = args[0]
    out_path = args[1]
    urls = load_urls(url_file_path)
    for url in urls:
    	domain = urlparse(url).netloc.replace(".", "_")
    	domain_folder = out_path + "/" + domain + "/"
    	record_folder = domain_folder + "record/"
    	record_data_folder = record_folder + "data/"
    	replay_out_folder = domain_folder + "replay--" + REPLAY_CONFIG + "/"
    	if (os.path.isdir(replay_out_folder)):
    		print("Folder " + replay_out_folder + " is already there! Skipping the replay")
    	else:
    		output_folder = replay_out_folder
    		traffic_folder = output_folder + "traffic/"
    		traffic_fpath = traffic_folder + domain
    		packet_rule_fpath = output_folder + "packet_rule"
    		log_folder = replay_out_folder + "/../"
    		os.mkdir(output_folder)
	    	os.mkdir(traffic_folder)

	    	# Generate delay rule file
	    	gen_rule = ["python", GEN_PACKET_RULE_SCRIPT, url, record_folder, REPLAY_CONFIG, packet_rule_fpath]
	    	subprocess.call(gen_rule)
	    	har_file = output_folder + domain + ".har"
	    	stdout_file_path = output_folder + "mahimahi-stdout"
	    	stdout_file = open(stdout_file_path, "w")
	    	# Call mahimahi and script to launch chrome
	    	mahimahi_replay = ["mm-webreplay", record_data_folder]
	    	mahimahi_delay = ["mm-adv-delay", DEFAULT_DELAY, packet_rule_fpath, traffic_fpath]
	    	mahimahi_proxy = ["mm-proxy", output_folder]
	    	web_page_load = ["nodejs", WEB_PAGE_LOAD_SCRIPT, url, har_file, BROWSER_WAIT_MARK]
	    	commands = mahimahi_replay + mahimahi_delay + mahimahi_proxy + web_page_load

	    	subprocess.call(commands, stdout=stdout_file)
	    	stdout_file.close()

	    	# Check whether recording success by finding the har file
	    	if (os.path.exists(har_file)):
	    		server_delays_path = record_folder + SERVER_DELAYS_NAME
	    		server_delay_file = open(server_delays_path, "w")
	    		replayed_delays_path = output_folder + REQUEST_DELAYS_NAME
	    		recorded_delays_path = record_data_folder + RECORDED_DELAYS_NAME
	    		gen_response_delay = ["python", GEN_RESPONSE_DELAY_SCRIPT, recorded_delays_path, replayed_delays_path]
	    		subprocess.call(gen_response_delay, stdout=server_delay_file)
	    		server_delay_file.close()
	    		print("Train success! Created server delay file: " + server_delays_path)
	    	else:
	    		print("Train FAILED!")

if __name__ == '__main__':
    prog = sys.argv[0]
    args = sys.argv[1:]
    num_args = len(args)

    if ( num_args < 2 ) :
        sys.stderr.write((u"Usage: %s" +
                          u" <urls-file> <out-path>\n") %
                         (prog))
        sys.exit(1)

    main(args)

