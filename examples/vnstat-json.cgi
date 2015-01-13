#!/usr/bin/perl -w

# vnstat-json.cgi -- example cgi for vnStat json output
# copyright (c) 2015 Teemu Toivola <tst at iki dot fi>
# released under the GNU General Public License


# location of vnstat binary
my $vnstat_cmd = '/usr/bin/vnstat';

# shown interfaces, edit as necessary
my @interfaces = ('eth0', 'eth1', 'ethX');


################


my $iface = "";
my $getiface = "";
my @values = split(/&/,$ENV{'QUERY_STRING'});
foreach $i (@values) {
	($varname, $varvalue) = split(/=/,$i);
	if ($varname == 'interface' && $varvalue =~ /^(\d+)$/) {
		$getiface = $varvalue;
	}
}

if (length($getiface) > 0 && $getiface >= 0 && $getiface <= $#interfaces) {
	$iface = "-i @interfaces[$getiface]";
}

print "Content-Type: application/json\n\n";
exec("$vnstat_cmd --json $iface");
