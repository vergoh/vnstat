# What / Why

This is the development branch for vnStat 2.0 that uses a sqlite database
for storing data instead of a C structure dump in a file. A better database
format is needed for enabling longer duration statistics with user
configurable durations. Everything is stored using UTC dates which should
help with possible timezone and daylight saving time related issues.
Yearly and 5 minute resolution statistics are also planned.

# Status

##### Overall status

  * prototype
  * not usable due to lack of outputs
  * don't even try with a user that has write access to any vnStat 1.x database files

##### Done

  * database creation with basic insert functionality
  * support for multiple interfaces

##### TODO

  * remove writing of old format database files
  * data buffering in daemon instead of writing to disk once every update cycle
  * data import from vnStat 1.x database format
  * all outputs (text and image)
  * old data cleanup, everything gets currently stored forever
  * top 10, missing from current database structure
    * should it still be only for days or also top 10 months?
  * feature configurability
  * freeze database structure
  * documentation
  * remember what else has been forgotten from this list
