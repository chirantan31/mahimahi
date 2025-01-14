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
# List of Rep Config: without-delay, network-delay, base, base-network-0.33, base-server-0.5, train, test
REPLAY_CONFIG = "base"
NUM_TRIALS = 5
DEFAULT_DELAY = "0"
REPLAYSERVER_LOG_NAME = "network-delay"

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
			os.mkdir(replay_out_folder)
			for i in range(1, NUM_TRIALS + 1):
				output_folder = replay_out_folder + str(i) + "/"
				traffic_folder = output_folder + "traffic/"
				traffic_fpath = traffic_folder + domain
				packet_rule_fpath = output_folder + "packet_rule"
				log_folder = replay_out_folder + "/../"
				os.mkdir(output_folder)
				os.mkdir(traffic_folder)

				if (os.path.exists(log_folder + REPLAYSERVER_LOG_NAME)):
					shutil.rm(log_folder + REPLAYSERVER_LOG_NAME)

				# Generate delay rule file
				gen_rule = ["python", GEN_PACKET_RULE_SCRIPT, url, record_folder, REPLAY_CONFIG, packet_rule_fpath]
				subprocess.call(gen_rule)
				har_file = output_folder + domain + ".har"
				stdout_file_path = output_folder + "mahimahi-stdout"
				stdout_file = open(stdout_file_path, "w")
				
				# Call mahimahi and script to launch chrome
				mahimahi_replay = ["mm-webreplay", record_data_folder]
				mahimahi_replay_delay = ["mm-webreplay-delay", record_data_folder]
				mahimahi_network_delay = ["mm-adv-delay", DEFAULT_DELAY, packet_rule_fpath, traffic_fpath]
				web_page_load = ["nodejs", WEB_PAGE_LOAD_SCRIPT, url, har_file, BROWSER_WAIT_MARK]
				
				commands = []
				if (REPLAY_CONFIG == "without-delay"):
					commands = mahimahi_replay + web_page_load
				elif (REPLAY_CONFIG == "network-delay"):
					commands = mahimahi_replay + mahimahi_network_delay + web_page_load
				elif (REPLAY_CONFIG == "base"):
					commands = mahimahi_replay_delay + mahimahi_network_delay + web_page_load
				elif (REPLAY_CONFIG == "base-network-0.33"):
					commands = mahimahi_replay_delay + mahimahi_network_delay + web_page_load
				elif (REPLAY_CONFIG == "base-server-0.5"):
					commands = mahimahi_replay_delay + mahimahi_network_delay + web_page_load
				else:
					print("REPLAY_CONFIG " + REPLAY_CONFIG + " not found, default to base")
					commands = mahimahi_replay_delay + mahimahi_network_delay + web_page_load

				subprocess.call(commands, stdout=stdout_file)
				stdout_file.close()
				
				# Move replayserver log file
				if (os.path.exists(log_folder + REPLAYSERVER_LOG_NAME)):
					shutil.move(log_folder + REPLAYSERVER_LOG_NAME, output_folder + REPLAYSERVER_LOG_NAME)

				# Check whether recording success by finding the har file
				if (os.path.exists(har_file)):
					print("Trial " + str(i) + ": " + url + " replay success")
				else:
					print("Trial " + str(i) + ": " + url + " replay FAILED")

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

