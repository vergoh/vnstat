# What / Why

This is the development branch for vnStat 2.0 that uses a sqlite database
for storing data instead of a C structure dump in a file. A better database
format is needed for enabling longer duration statistics with user
configurable durations. Yearly and 5 minute resolution statistics are also planned.

# Status

##### Overall status

  * only daemon is being compiled
  * alpha version with minimal working daemon implementation
    * many sanity checks are missing or disabled
    * some daemon related configuration options aren't active
  * don't try with a user that has write access to any vnStat 1.x database files
    * just to be on the safe side

##### Done

  * database creation and handling
  * support for multiple interfaces
  * dynamic data buffering in daemon, SaveInterval is honored
  * 5 minute, hourly, daily, monthly, yearly and total traffic recorded to database
  * legacy database files are read only during first startup for data import
    * write support is no longer included in code
  * full data import from vnStat 1.x database format including reconstructed yearly data
  * legacy database is not kept in memory for each interface during daemon runtime

##### TODO

  * `grep TODO src/* tests/*`
  * continue daemon refactoring
  * add missing sanity checks to daemon
  * rewrite disabled tests
  * all outputs (text and image)
    * use of 5 minute resolution statistics
  * old data cleanup, everything gets currently stored forever
  * top 10, included in database schema but not populated
  * feature configurability
  * freeze database structure
  * documentation
  * remember what else has been forgotten from this list
