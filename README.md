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
  * don't even try with a user that has write access to any vnStat 1.x database files

##### Done

  * database creation and handling
  * support for multiple interfaces
  * dynamic data buffering in daemon, SaveInterval is honored
  * 5 minute, hourly, daily, monthly, yearly and total traffic recorded to database
  * legacy database files aren't being accessed

##### TODO

  * continue daemon refactoring
  * add missing sanity checks to daemon
  * rewrite disabled tests
  * data import from vnStat 1.x database format
  * all outputs (text and image)
    * use of 5 minute resolution statistics
  * old data cleanup, everything gets currently stored forever
  * top 10, included in database schema but not populated
  * feature configurability
  * freeze database structure
  * documentation
  * remember what else has been forgotten from this list
