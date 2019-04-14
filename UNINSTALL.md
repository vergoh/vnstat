# Uninstall

 1. Stop the daemon (`vnstatd`) if it is still running
 2. Remove any manually installed service files or file entries
    used to start the daemon
 3. Run `make uninstall` and follow the instructions
 4. Remove the database directory if no longer needed, the
    command of the previous step will not touch any collected data
