
# tl;dr

    ./configure --prefix=/usr --sysconfdir=/etc && make && make install


# Prerequisites

 - make
 - C compiler
 - user with access to kernel interface statistics
   - usually available by default but can be restricted for example
     by grsecurity and similar security enhancement suites or settings
 - sqlite3     (library and development files)
 - libgd       (optional, image output)
 - check       (optional, test suite)
 - pkg-config  (optional, for check detection)
 - autotools   (optional, for recreating configure and makefiles)


# Compiling the binaries

This source package contains the required source files for vnStat including
the daemon (`vnstatd`) and image output (`vnstati`). Executing

    ./configure --prefix=/usr --sysconfdir=/etc && make

will compile `vnstat` and `vnstatd`. The optional image output capable binary
`vnstati` will also be compiled if the required additional libgd library is
found to be available and `--disable-image-output` isn't given as parameter
for `./configure`.

An example cgi (`vnstat.cgi`) to be used with a http server with the image
output support has been provided in the `examples` directory. Configuration
options for the cgi are in the beginning of the file. Additional examples
for using the json output are also available in the same directory.

For executing the optional test suite, see the appendix at the end of this
file.


# Installing as root

Log in as root and run the following command:

    make install

If there were no errors, vnStat binaries, man pages and a configuration
file should now be installed. The configuration file will be upgraded using
previously configured values if it is found already to exist. A backup
of the previous configuration file will be named as `vnstat.conf.old` in the
current directory.

The configuration file `/etc/vnstat.conf` should be checked at this point.
See the `vnstat.conf` man page for documentation about available options.

Finally, make vnStat monitor available interfaces. Configure init scripts
so that the following command is executed once during system startup:

    vnstatd -d

The `examples` directory contains suitable files for most commonly used
service managers. Refer to your operating system / distribution
documentation if unsure which service manager is being used.

 * systemd

   * option 1: hardened - requires a more recent systemd version
    ~~~
    cp -v examples/systemd/vnstat.service /etc/systemd/system/
    systemctl enable vnstat
    systemctl start vnstat
    ~~~

   * option 2: simple - works also with older systemd versions
    ~~~
    cp -v examples/systemd/simple/vnstat.service /etc/systemd/system/
    systemctl enable vnstat
    systemctl start vnstat
    ~~~

 * init.d

   * Debian
    ~~~
    cp -v examples/init.d/debian/vnstat /etc/init.d/
    update-rc.d vnstat defaults
    service vnstat start
    ~~~

   * Red Hat / CentOS
    ~~~
    cp -v examples/init.d/redhat/vnstat /etc/init.d/
    chkconfig vnstat on
    service vnstat start
    ~~~

 * upstart
    ~~~
    cp -v examples/upstart/vnstat.conf /etc/init/
    initctl start vnstat
    ~~~

An alternative method is to add the command to an already existing
script that gets executed during system startup. In many distributions
`/etc/rc.local` can be used if nothing else suitable can be found. Note
that the full path to the executable may need to be included instead of
only the command itself.

During first startup, the daemon (`vnstatd`) should list and add all
available interfaces for monitoring. Depending on configuration, it may
take some minutes for the `vnstat` command to begin showing results as
the entries in the database aren't updated constantly.

Monitoring of unwanted interfaces can be stopped with:

    vnstat --remove -i ethunwanted


# Installing without root access

Copy all needed binaries to some directory included in your PATH
(`~/bin/` is used here as an example) and create the database directory.

    cp -v vnstat vnstatd vnstati ~/bin/
    cp -v cfg/vnstat.conf ~/.vnstatrc
    mkdir ~/.vnstat

Check that the binaries got installed to a suitable location and are of the
correct version:

    vnstat --version

If this gives a `command not found` error or a different than expected
version then check the content of the PATH variable and try again.

Next open the configuration file `~/.vnstatrc` with your favorite text editor
and locate the following line:

    DatabaseDir "/var/lib/vnstat"

and replace it with

    DatabaseDir "/pathtomyhomedir/.vnstat"

Next, locate the following lines:

    UseLogging 2
    LogFile "/var/log/vnstat/vnstat.log"
    PidFile "/var/run/vnstat/vnstat.pid"

and replace them with

    UseLogging 1
    LogFile "/pathtomyhomedir/.vnstat/.log"
    PidFile "/pathtomyhomedir/.vnstat/.pid"

Finally, save the file. If you are unsure about your home directory path, execute

    cd ; pwd

The ouput should tell your home directory.

Now it's time to add a crontab entry for vnStat in order to get the daemon
running automatically after a system startup. Do that by executing the
command `crontab -e` and add the following line (without leading spaces,
remember to change the path):

    @reboot ~/bin/vnstatd -d

If you found yourself using a strange editor then `man vi` may help.

Make sure the configuration file (`~/.vnstatrc`) has the log option either
disabled or set to a file that is located in a place where you have write
permissions, such as your home dir. Then try starting the daemon with

    vnstatd -d

After that wait for (or generate) at least 1024 bytes of network traffic
(and 5 min for the next database file save).

    vnstat

Now you should get some stats about your network usage. See the config
file `~/.vnstatrc` for interface and other settings.


# Appendix: Running the test suite

This step isn't mandatory for using vnStat.

The source package includes a test suite for validating many of the
functionalities provided and used by the executables. The test suite requires
the Check unit testing framework ( https://libcheck.github.io/check/ ) to be
installed and available. Depending on the used distribution, the necessary
package to be installed is usually called `check` and may also require
`check-devel` to be installed if available. After the `./configure` script has
been executed the test suite can be executed with:

    make check

The output should show a non-zero number of tests executed if all the
necessary packages were available. A more detailed list of executed tests
can be seen from the `check_vnstat.log` file after execution.
