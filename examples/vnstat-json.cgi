#!/usr/bin/perl -w

# vnstat-json.cgi -- example cgi for vnStat json output
# copyright (c) 2015-2021 Teemu Toivola <tst at iki dot fi>
# released under the GNU General Public License


# location of vnstat binary
my $vnstat_cmd = '/usr/bin/vnstat';

# individually accessible interfaces with ?interface=N
# for static list, uncomment first line below, update the list and comment out second line
#my @interfaces = ('eth0', 'eth1');
my @interfaces = `$vnstat_cmd --dbiflist 1`;


################


my $iface = "";
chomp @interfaces;

if (defined $ENV{'QUERY_STRING'}) {
	my $getiface = "";
	my @values = split(/&/, $ENV{'QUERY_STRING'});
	foreach $i (@values) {
		($varname, $varvalue) = split(/=/, $i);
		if ($varname eq 'interface' && $varvalue =~ /^(\d+)$/) {
			$getiface = $varvalue;
		}
	}

	if (length($getiface) > 0 && $getiface >= 0 && $getiface <= $#interfaces) {
		$iface = "-i @interfaces[int($getiface)]";
	}
}

print "Content-Type: application/json\n\n";
exec("$vnstat_cmd --json $iface");
