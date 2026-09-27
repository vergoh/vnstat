#!/usr/bin/perl -w

# vnstat-json.cgi -- example cgi for vnStat json output
# copyright (c) 2015-2026 Teemu Toivola <tst at iki dot fi>
# released under the GNU General Public License

use strict;

# location of vnstat binary
my $vnstat_cmd = '/usr/bin/vnstat';

# individually accessible interfaces with ?interface=N or /interfacename suffix
# for static list, uncomment and update the list
#our @interfaces = ('eth0', 'eth1');


################


{
	if (!@main::interfaces) {
		if (open(my $iflist, "-|", $vnstat_cmd, "--dbiflist", "1")) {
			@main::interfaces = <$iflist>;
			close $iflist;
		}
	}

	my @interfaces = @main::interfaces;
	chomp @interfaces;

	my $iface;

	if (defined $ENV{PATH_INFO}) {
		my @fields = split(/\//, $ENV{PATH_INFO});
		my $interface = $fields[-1];
		for my $i (0..$#interfaces) {
			if ($interfaces[${i}] eq $interface) {
				$iface = $interface;
				last;
			}
		}
	}

	if (!defined $iface and defined $ENV{QUERY_STRING}) {
		my $getiface = "";
		my @values = split(/&/, $ENV{QUERY_STRING});
		foreach my $i (@values) {
			my ($varname, $varvalue) = split(/=/, $i);
			if ($varname eq 'interface' && $varvalue =~ /^(\d+)$/) {
				$getiface = $varvalue;
			}
		}

		if (length($getiface) > 0 && $getiface >= 0 && $getiface <= $#interfaces) {
			$iface = $interfaces[int($getiface)];
		}
	}

	my @command = ($vnstat_cmd, "--json");
	if (defined $iface) {
		push @command, "-i", $iface;
	}

	print "Content-Type: application/json\n\n";
	exec @command;
}
