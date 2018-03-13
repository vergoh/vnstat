# vnStat

vnStat is a console-based network traffic monitor that uses the network
interface statistics provided by the kernel as information source. This
means that vnStat won't actually be sniffing any traffic and also ensures
light use of system resources.

Traffic statistics are stored on a hourly level for the last 24 hours, on
a daily level for the last 30 days and on a monthly level for the last 12
months. Total seen traffic and a top 10 days listing is also provided.
Optional png image output is available in systems with the gd library
installed.

See the official webpage http://humdi.net/vnstat/ for additional details
and output examples.

## Getting started

vnStat works best when installed. It's possible to either use the latest
stable release or get the current development version from git.

Stable version
  1. ``wget http://humdi.net/vnstat/vnstat-latest.tar.gz``
  2. optional steps for verifying the file signature
     1. ``wget http://humdi.net/vnstat/vnstat-latest.tar.gz.asc``
     2. ``gpg --keyserver pgp.mit.edu --recv-key 0xDAFE84E63D140114``
     3. ``gpg --verify vnstat-latest.tar.gz.asc vnstat-latest.tar.gz``
     4. the signature is correct if the output shows "Good signature from Teemu Toivola"
  3. ``tar zxvf vnstat-latest.tar.gz``
  4. ``cd vnstat-*``

Development version (not recommended currently, documentation not up to date)
  1. ``git clone https://github.com/vergoh/vnstat``
  2. ``cd vnstat``

In both cases, continue with instructions from the INSTALL or INSTALL_BSD file
depending on used operating system. Experimental instructions for OS X are
available in the INSTALL_OSX file. Instructions for upgrading from a previous
version are included in the UPGRADE file.

## Current development status

Unlike version 1.18, the current development version uses a sqlite database
for storing data instead of a C structure dump in a file. This change enables
having longer-duration statistics with user-configurable data retention
durations. Yearly and five-minute resolution statistics are now included.

##### Overall status

  * beta version
    * possibly more beta than seen with recent 1.x releases
    * has been tested so far mainly in 24/7 running servers
  * vnstatd (daemon) has all features implemented
    * some sanity checks may be missing or disabled
  * vnstat (console output) has most features implemented
  * vnstati (image output) has all intended original features implemented
  * 5 minute resolution outputs available in both text and image formats
  * data migration verified to work automatically from 1.x version databases
  * test coverage is better than with 1.x releases
    * some unit conversion issues have been found and fixed

##### Done

  * vnstatd (daemon)
    * database creation and handling
    * support for multiple interfaces
    * dynamic data buffering, SaveInterval is honored
    * 5 minute, hourly, daily, monthly, yearly and total traffic recorded to database
    * legacy database files are read only during first startup for data import
      * write support is no longer included in code
      * legacy database files aren't removed after first time import to new database
    * full data import from vnStat 1.x database format including reconstructed yearly data
    * legacy database is not kept in memory for each interface during daemon runtime
    * new configuration options for data retention durations
      * features can be disabled
    * old data cleanup
      * executed during startup and then once every hour
    * logging and handling of possible database access errors
      * only fatal errors will cause the daemon to exit directly
  * most vnstat (console output) features
  * all vnstati (image output) features
  * pebibyte support

##### Removed features

  * text format database import
    * most likely better to do directly via sqlite cli or using some script language
  * merge of data from multiple interfaces
  * weekly ouput
  * `MonthRotate` configuration option
  * kernel test
    * provided some use mostly with 2.0 and 2.2 kernels
  * `--update` / `-u` using vnstat command
  * old style (default in versions up to 1.7) summary layout in image output

##### TODO

  * `grep TODO src/* tests/*`
  * query of specific time range date
    * possibly not included in first 2.x release
    * extending the length of the current outputs is however already supported
  * feature configurability
    * is something still missing?
  * freeze database structure
    * plan ahead and figure out how to migrate data to new structure if necessary?
  * decide if output type parameters should be renamed
    * currently -h results in a graph style output when everything else gives a list
    * -h to output list and have -hg (or similar) for the hourly graph?
  * documentation
    * needs to be updated to match 2.0 feature set and dependency requirements
    * especially feature comparison with 1.x versions is needed as some features have been left out
  * remember what else has been forgotten from this list

## Contacting the author

**email:** Teemu Toivola <tst at iki dot fi>
**irc:** Vergo (IRCNet)
**git:** https://github.com/vergoh/vnstat

Bug reports, improvement ideas, feature requests and pull requests should be
sent using the matching features on GitHub as those are harder to miss or
forget.
