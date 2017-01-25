# What / Why

This is the development branch for vnStat 2.0 that uses a sqlite database
for storing data instead of a C structure dump in a file. A better database
format is needed for enabling longer duration statistics with user
configurable durations. Yearly and 5 minute resolution statistics are now included.

# Status

##### Overall status

  * alpha version with working daemon implementation
    * some sanity checks may be missing or disabled
  * vnstat (console output) has most features implemented
  * vnstati (image output) is disabled
  * getting closer to replace vnStat 1.x

##### Done

  * database creation and handling
  * support for multiple interfaces
  * dynamic data buffering in daemon, SaveInterval is honored
  * 5 minute, hourly, daily, monthly, yearly and total traffic recorded to database
  * legacy database files are read only during first startup for data import
    * write support is no longer included in code
  * full data import from vnStat 1.x database format including reconstructed yearly data
  * legacy database is not kept in memory for each interface during daemon runtime
  * new configuration options for data retention durations
    * features can be disabled
  * old data cleanup
    * executed during startup and then once every hour
  * most vnstat (console output) features

##### Removed features

  * database import
    * most likely better to do directly via sqlite
  * merge of data from multiple interfaces
  * weekly ouput
  * `MonthRotate`
  * kernel test
  * `--update` / `-u` using vnstat command

##### TODO

  * `grep TODO src/* tests/*`
  * continue daemon refactoring
  * add missing sanity checks to daemon
  * console outputs
    * export?
  * image outputs
    * all
  * use of 5 minute resolution statistics
  * feature configurability
  * freeze database structure
    * plan ahead and figure out how to migrate data to new structure if necessary?
  * documentation
    * especially feature comparison with 1.x versions is needed as some features may be left out
  * remember what else has been forgotten from this list
