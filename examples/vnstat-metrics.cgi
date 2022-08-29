#!/usr/bin/perl -w

# vnstat-metrics.cgi -- Prometheus compatible metrics endpoint output from vnStat data
# copyright (c) 2022 Teemu Toivola <tst at iki dot fi>
# released under the GNU General Public License

use strict;
use JSON::PP;

# location of vnstat binary
my $vnstat_cmd = '/usr/bin/vnstat';


################


sub get_interface_alias
{
	my ($interface) = @_;
	my $interface_alias = $interface->{'alias'};
	if (length($interface_alias) == 0) {
		$interface_alias = $interface->{'name'};
	}
	return $interface_alias;
}

sub print_totals
{
	my ($data) = @_;

	print "\n# HELP vnstat_interface_total_received_bytes All time total received (rx) bytes\n";
	print "# TYPE vnstat_interface_total_received_bytes counter\n";

	foreach my $interface ( @{ $data->{'interfaces'} } ) {
		my $interface_alias = get_interface_alias($interface);
		print "vnstat_interface_total_received_bytes{interface=\"$interface->{'name'}\",alias=\"$interface_alias\"} $interface->{'traffic'}{'total'}{'rx'} $interface->{'updated'}{'timestamp'}000\n";
	}

	print "\n# HELP vnstat_interface_total_transmitted_bytes All time total transmitted (tx) bytes\n";
	print "# TYPE vnstat_interface_total_transmitted_bytes counter\n";

	foreach my $interface ( @{ $data->{'interfaces'} } ) {
		my $interface_alias = get_interface_alias($interface);
		print "vnstat_interface_total_transmitted_bytes{interface=\"$interface->{'name'}\",alias=\"$interface_alias\"} $interface->{'traffic'}{'total'}{'tx'} $interface->{'updated'}{'timestamp'}000\n";
	}
}

sub print_data_resolution
{
	my ($resolution, $data) = @_;
	my $output_count = 0;

	print "\n# HELP vnstat_interface_".$resolution."_received_bytes Received (rx) bytes for current $resolution\n";
	print "# TYPE vnstat_interface_".$resolution."_received_bytes gauge\n";

	$output_count = 0;
	foreach my $interface ( @{ $data->{'interfaces'} } ) {
		my $interface_alias = get_interface_alias($interface);
		if (defined $interface->{'traffic'}{$resolution}) {
			print "vnstat_interface_".$resolution."_received_bytes{interface=\"$interface->{'name'}\",alias=\"$interface_alias\"} $interface->{'traffic'}{$resolution}[0]{'rx'} $interface->{'updated'}{'timestamp'}000\n";
			$output_count++;
		}
	}
	if ($output_count == 0) {
		print "# no data\n";
	}

	print "\n# HELP vnstat_interface_".$resolution."_transmitted_bytes Transmitted (tx) bytes for current $resolution\n";
	print "# TYPE vnstat_interface_".$resolution."_transmitted_bytes gauge\n";

	$output_count = 0;
	foreach my $interface ( @{ $data->{'interfaces'} } ) {
		my $interface_alias = get_interface_alias($interface);
		if (defined $interface->{'traffic'}{$resolution}) {
			print "vnstat_interface_".$resolution."_transmitted_bytes{interface=\"$interface->{'name'}\",alias=\"$interface_alias\"} $interface->{'traffic'}{$resolution}[0]{'tx'} $interface->{'updated'}{'timestamp'}000\n";
			$output_count++;
		}
	}
	if ($output_count == 0) {
		print "# no data\n";
	}
}

my @data_resolutions = ('fiveminute', 'hour', 'day', 'month', 'year');

print "Content-Type: text/plain\n\n";

my $json_data = `$vnstat_cmd --json s 1`;

my $data = "";
eval { $data = decode_json($json_data) };
if ($@) {
	print "# Error: Invalid command output: $json_data\n";
	exit 1;
}

if (not defined $data->{'vnstatversion'}) {
	print "# Error: Expected content from command output missing\n";
	exit 1;
}

if (not defined $data->{'interfaces'}[0]) {
	print "# Error: No interfaces found in command output\n";
	exit 1;
}

if (not defined $data->{'interfaces'}[0]{'created'}{'timestamp'}) {
	print "# Error: Incompatible vnStat version used\n";
	exit 1;
}

print "# vnStat version: ".$data->{'vnstatversion'}."\n";

print_totals($data);

foreach my $data_resolution ( @data_resolutions ) {
	print_data_resolution($data_resolution, $data);
}
