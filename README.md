# What / Why

This is the development branch for vnStat 2.0 that uses a sqlite database
for storing data instead of a C structure dump in a file. A better database
format was needed for enabling longer-duration statistics with user-configurable
durations. Yearly and five-minute resolution statistics are now included.

# Status

##### Overall status

  * alpha version
    * has been tested so far only in 24/7 running servers
  * vnstatd (daemon) has all features implemented
    * some sanity checks may be missing or disabled
  * vnstat (console output) has most features implemented
  * vnstati (image output) lacks rewrite of some features
  * getting closer to replace vnStat 1.x

##### Done

  * vnstatd (daemon)
    * database creation and handling
    * support for multiple interfaces
    * dynamic data buffering, SaveInterval is honored
    * 5 minute, hourly, daily, monthly, yearly and total traffic recorded to database
    * legacy database files are read only during first startup for data import
      * write support is no longer included in code
    * full data import from vnStat 1.x database format including reconstructed yearly data
    * legacy database is not kept in memory for each interface during daemon runtime
    * new configuration options for data retention durations
      * features can be disabled
    * old data cleanup
      * executed during startup and then once every hour
    * logging and handling of possible database access errors
      * only fatal errors will cause the daemon to exit directly
  * most vnstat (console output) features
  * many vnstati (image output) features

##### Removed features

  * database import
    * most likely better to do directly via sqlite cli
  * merge of data from multiple interfaces
  * weekly ouput
  * `MonthRotate` configuration option
  * kernel test
    * provided some use mostly with 2.0 and 2.2 kernels
  * `--update` / `-u` using vnstat command

##### TODO

  * `grep TODO src/* tests/*`
  * continue daemon refactoring
  * testing in more diverse environments
  * extend sanity checks in daemon
  * image outputs
    * summary and hourly + combinations
  * output of 5 minute resolution statistics
  * feature configurability
  * freeze database structure
    * plan ahead and figure out how to migrate data to new structure if necessary?
  * documentation
    * especially feature comparison with 1.x versions is needed as some features may be left out
  * remember what else has been forgotten from this list
