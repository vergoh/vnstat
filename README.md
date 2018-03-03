# vnStat

vnStat is a console-based network traffic monitor that uses the network interface statistics provided by the kernel as information source. This means that vnStat won't actually be sniffing any traffic and also ensures light use of system resources.

Traffic statistics are stored on a hourly level for the last 24 hours, on a daily level for the last 30 days and on a monthly level for the last 12 months. Total seen traffic and a top 10 days listing is also provided. Optional png image output is available in systems with the gd library installed.

See the official webpage http://humdi.net/vnstat/ for additional details and output examples.


## Getting started

vnStat works best when installed. It's possible to either use the latest stable release or get the current development version from git.

Stable version
  1. ``wget http://humdi.net/vnstat/vnstat-latest.tar.gz``
  2. optional steps for verifying the file signature
     1. ``wget http://humdi.net/vnstat/vnstat-latest.tar.gz.asc``
     2. ``gpg --keyserver pgp.mit.edu --recv-key 0xDAFE84E63D140114``
     3. ``gpg --verify vnstat-latest.tar.gz.asc vnstat-latest.tar.gz``
     4. the signature is correct if the output shows "Good signature from Teemu Toivola"
  3. ``tar zxvf vnstat-latest.tar.gz``
  4. ``cd vnstat-*``

Development version
  1. ``git clone https://github.com/vergoh/vnstat``
  2. ``cd vnstat``

In both cases, continue with instructions from the INSTALL or INSTALL_BSD file depending on used operating system. Experimental instructions for OS X are available in the INSTALL_OSX file. Instructions for upgrading from a previous version are included in the UPGRADE file.

## Contacting the author

**email:** Teemu Toivola <tst at iki dot fi>
**irc:** Vergo (IRCNet)
**git:** https://github.com/vergoh/vnstat

Bug reports, improvement ideas, feature requests and pull requests should be done using the matching features on GitHub as those are harder to miss or forget.
