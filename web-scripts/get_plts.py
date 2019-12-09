#!/usr/bin/env python
# -*- mode: python; coding: utf-8; fill-column: 80; -*-
#

"""
get_plts.py
Retrieve page load times from HAR files.
"""

import argparse
import codecs
import gzip
import io
import json
import numpy as np
import os
import sys
from urllib2 import urlparse as urlp


def load_sites(urls_file, data_dir):
    """Load URLs used for testing along with the path to the data directory.
    """
    sites = {}
    urls_fpath = os.path.abspath(urls_file)
    for url, dname in (line.strip().split() for line in
                       io.open(urls_file, 'r', encoding='utf-8')):
        sites[url] = os.path.join(os.path.abspath(data_dir), dname)

    return sites

def get_page_timings(harf):
    """Retrieve `pageTimings` from HAR file.
    """
    file = open(harf, 'r')
    har   = json.loads(codecs.decode(file.read(), 'utf-8'))
    log   = har[u'log']
    pages = log[u'pages']

    if len(pages) > 1:
        raise ValueError("More than one page found in `{}`".format(harf))

    if len(pages) == 0:
        sys.stderr.write("Warn: no pages found in `{}`\n".format(harf))
        return None

    page = pages[0]
    if 'pageTimings' not in page:
        return None

    return page['pageTimings']


def get_onload_time(harf):
    """Retrieve `onload` time from HAR file.
    """
    t = get_page_timings(harf)
    if t is None:
        return None
    # return float(t['onContentLoad'])
    return float(t['onLoad'])


def load_timings(url, sites, path, num_trials):
    """Load timings of the site corresponding to the given hostname from the
    path specified.
    """
    timings = []
    netloc  = urlp.urlparse(url).netloc
    name = netloc.replace('.', '_')

    if (num_trials < 0):
        fpath = os.path.join(path, "{}.har".format(name))
        if os.path.exists(fpath):
            t = get_onload_time(fpath)
            if t is not None:
                timings.append(t)

    for n in range(1, num_trials+1):
        tpath = os.path.join(path, str(n))
        fpath = os.path.join(tpath, "{}.har".format(name))
        if not os.path.exists(fpath):
            continue

        try:
            t = get_onload_time(fpath)
            if t is None:
                continue
            timings.append(t)
        except KeyError:
            continue

    N   = len(timings)
    if not N:
        return (N, -1, -1)

    p25, p50, p75, p90, p95 = [np.percentile(timings, p)
                               for p in (25, 50, 75, 90, 95)]
    plts_str = ','.join(str(v) for v in timings)
    mean = np.mean(timings)
    std = np.std(timings)
    return (N, mean)
    # return (N, mean, p25, p50, p75, plts_str)
    # return (N, mean, p25, p50, p75, p90, p95, plts_str)


def main(urls_file, num_trials, data_dir, exps):
    sites = load_sites(urls_file, data_dir)

    for s in sorted(sites.keys()):
        res = ''
        for exp in exps:
            exp_path = os.path.join(sites[s], exp)
            if ("record" in exp):
                num_trials = -1
            exp_res  = ' '.join(str(t) for t in
                                load_timings(s, sites[s], exp_path, num_trials))
            res  = ' '.join((res, exp_res))
        print("{} {}".format(s, res))


if __name__ == '__main__':
    desc = 'Utility to retrieve page load times from HAR files.'
    parser = argparse.ArgumentParser(description=desc)

    parser.add_argument('--urls-file', '-u',
                        type=str,
                        help='File containing URLs and record paths')
    parser.add_argument('--num-trials', '-c',
                        required=True,
                        type=int,
                        help='Number of trials')
    parser.add_argument('--data-dir', '-d',
                        default='.',
                        type=str,
                        help='Path containing the experiment output data')
    parser.add_argument('exps',
                        nargs='+',
                        type=str,
                        help='List of experiments')
    args = parser.parse_args()
    main(args.urls_file, args.num_trials, args.data_dir, args.exps)
