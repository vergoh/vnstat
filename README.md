# What / Why

This is the development branch for vnStat 2.0 that uses a sqlite database
for storing data instead of a C structure dump in a file. A better database
format was needed for enabling longer-duration statistics with user-configurable
data retention durations. Yearly and five-minute resolution statistics are now included.

# Status

##### Overall status

  * beta version
    * likely to be more beta than seen with recent 1.x releases
    * has been tested so far only in 24/7 running servers
  * vnstatd (daemon) has all features implemented
    * some sanity checks may be missing or disabled
  * vnstat (console output) has most features implemented
  * vnstati (image output) has all intended original features implemented
  * test coverage is better than in current master branch
    * some unit conversion issues have been found and fixed
  * getting closer to replace vnStat 1.x

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
  * continue daemon refactoring
  * testing in more diverse environments
  * extend sanity checks in daemon
  * output of 5 minute resolution statistics (vnstat and vnstati)
    * possibly not included in first 2.x release
  * query of specific time range date
    * most likely not included in first 2.x release
    * extending the length of the current outputs is however already supported
      * hourly output is the only exception
  * feature configurability
  * freeze database structure
    * plan ahead and figure out how to migrate data to new structure if necessary?
  * documentation
    * will be updated once merged to master from branch
    * especially feature comparison with 1.x versions is needed as some features may be left out
  * remember what else has been forgotten from this list
