#!/usr/bin/env python
# -*- mode: python; coding: utf-8; fill-column: 80; -*-
#
# record-pages.py
# Created by William Sentosa
#

import sys
import os
import subprocess
from urlparse import urlparse

# CONSTANTS
BROWSER_WAIT_MARK = "load"
WEB_PAGE_LOAD_SCRIPT = "web_page_load.js"

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
    success = 0
    failed = 0
    already = 0
    
    for url in urls:
    	domain = urlparse(url).netloc.replace(".", "_")
    	domain_folder = out_path + "/" + domain + "/"
    	record_folder = domain_folder + "record/"
    	record_data_folder = record_folder + "data/"
    	har_file = record_folder + domain + ".har"
    	if (os.path.isdir(domain_folder)):
    		print("Path to domain " + domain + " already exists.")
    		already += 1
    	else:
    		# create folder for each domain
	    	os.mkdir(domain_folder)
	    	os.mkdir(record_folder)
	    	# Call mahimahi and script to launch chrome
	    	mahimahi_record = ["mm-webrecord", record_data_folder]
	    	web_page_load = ["nodejs", WEB_PAGE_LOAD_SCRIPT, url, har_file, BROWSER_WAIT_MARK]
	    	subprocess.call(mahimahi_record + web_page_load)
	    	# Check whether recording success by finding the har file
	    	if (os.path.exists(har_file)):
	    		print(url + " record success")
	    		success += 1
	    	else:
	    		print(url + " record failed")
	    		failed += 1

	# Print statistics
	print("****************")
	print("SUCCESS: " + str(success))
	print("FAILED: " + str(failed))
	print("ALREADY: " + str(already))
	print("****************")

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

