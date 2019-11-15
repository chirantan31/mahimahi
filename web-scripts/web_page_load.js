// web_page_load.js

const fs                  = require('fs');
const { harFromMessages } = require('chrome-har');
const os                  = require('os');
const path                = require('path');
const { promisify }       = require('util');
const puppeteer           = require('puppeteer');
var sleep = require('sleep');

// Experiment options.
const opts = {
    browser: {
        // Whether to run browser in `headless` mode.
        headless: false,
        // Time (in ms) to wait for browser launch to complete.
        timeout: 50000,
        // Whether to ignore HTTPS errors during navigation.
        ignoreHTTPSErrors: true,
        args: [ '--enable-features=NetworkService',
            '--ignore-certificate-errors',
            ],
    },

    nav: {
        // Maximum time to complete page navigation.
        timeout: 1000000,
    },

    // Event types to observe.
    eventsToObserve: [
        'Page.loadEventFired',
        'Page.domContentEventFired',
        'Page.frameStartedLoading',
        'Page.frameAttached',
        'Network.requestWillBeSent',
        'Network.requestServedFromCache',
        'Network.dataReceived',
        'Network.responseReceived',
        'Network.resourceChangedPriority',
        'Network.loadingFinished',
        'Network.loadingFailed',
    ],

    // Temporary `User Data` directory.
    tmpUsrDataDir: path.join(os.tmpdir(), 'dejavu'),
};


(() => {
    // Sleep for 2 second before launching the browser so that mahi2 has finished setup properly.
    sleep.sleep(2);
    const EXIT_FAIL = 1;

    function showUsage(err) {
        console.warn("Usage: %s <URL> <output-file> " +
                     "[<load|domcontentloaded|networkidle0|networkidle2>]",
                     process.argv[1]);
        process.exit(err);
    };

    const args = process.argv.slice(2);
    if (args.length < 2 || args.length > 3) {
        showUsage(EXIT_FAIL);
    }

    var wait_mark = undefined;
    if (args.length == 3) {
        wait_mark = args[2];
        if (wait_mark !== 'load'             &&
            wait_mark !== 'domcontentloaded' &&
            wait_mark !== 'networkidle0'     &&
            wait_mark !== 'networkidle2') {
            console.error("Unknown event `%s` for wait_mark!", wait_mark);
            showUsage(EXIT_FAIL);
        }
    }

    // Path to `User Data`.
    // NOTE: Just to be sure that each request is as _new_ as it can be.
    opts.browser['userDataDir'] = opts.tmpUsrDataDir;

    (async (site_url, out_fpath, wait_mark) => {
        // By default, wait until 'networkidle2'.
        var wait_mark = typeof wait_mark !== 'undefined' ?
                wait_mark: 'networkidle2';

        const browser = await puppeteer.launch(opts.browser);
        const page    = await browser.newPage();
        await page.setCacheEnabled();

        // Register events listeners.
        const client = await page.target().createCDPSession();
        await client.send('Page.enable');
        await client.send('Network.enable');

        // List of events for converting to HAR.
        const events = [];

        opts.eventsToObserve.forEach(method => {
            client.on(method, params => {
                events.push({ method, params });
            });
        });

        
        // Load the Web site.
        await page.goto(site_url, {timeout: opts.nav.timeout,
                               waitUntil: wait_mark});
        await browser.close();
        

        // Generate the HAR file.
        const har = harFromMessages(events);
        await promisify(fs.writeFile)(out_fpath, JSON.stringify(har, null, 2));
    })(args[0], args[1], wait_mark).catch(err => {
        console.error(err);
        process.exit(EXIT_FAIL);
    });
})();
