# What / Why

This is the development branch for vnStat 2.0 that uses a sqlite database
for storing data instead of a C structure dump in a file. A better database
format is needed for enabling longer duration statistics with user
configurable durations. Yearly and 5 minute resolution statistics are also planned.

# Status

##### Overall status

  * alpha version with working daemon implementation
    * some sanity checks may be missing or disabled
  * vnstat (console output) lacks most database features
    * `-l` / `--live` and `-tr` / `--traffic` are available
    * implemented database dependent features
      * `--create`
      * `--delete`
      * summary output
  * vnstati (image output) is disabled
  * not ready to replace vnStat 1.x due to lack of most outputs

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

##### TODO

  * `grep TODO src/* tests/*`
  * continue daemon refactoring
  * add missing sanity checks to daemon
  * most outputs (text and image)
    * use of 5 minute resolution statistics
  * feature configurability
  * freeze database structure
    * plan ahead and figure out how to migrate data to new structure if necessary?
  * documentation
    * especially feature comparison with 1.x versions is needed as some features may be left out
  * remember what else has been forgotten from this list
