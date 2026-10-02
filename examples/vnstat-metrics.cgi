#!/usr/bin/perl -w

# vnstat-metrics.cgi -- Prometheus compatible metrics endpoint output from vnStat data
# copyright (c) 2022 Teemu Toivola <tst at iki dot fi>
# released under the GNU General Public License

use strict;
use JSON::PP;
use Time::Local qw(timelocal_nocheck);

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

sub prom_escape
{
	my ($value) = @_;
	$value = '' unless defined $value;
	$value =~ s/\\/\\\\/g;
	$value =~ s/"/\\"/g;
	$value =~ s/\n/\\n/g;
	return $value;
}

sub prom_interface_labels
{
	my ($name, $alias) = @_;
	return 'interface="' . prom_escape($name) . '",alias="' . prom_escape($alias) . '"';
}

sub print_totals
{
	my ($data) = @_;

	print "\n# HELP vnstat_interface_total_received_bytes All time total received (rx) bytes\n";
	print "# TYPE vnstat_interface_total_received_bytes counter\n";

	foreach my $interface ( @{ $data->{'interfaces'} } ) {
		my $interface_alias = get_interface_alias($interface);
		print "vnstat_interface_total_received_bytes{" . prom_interface_labels($interface->{'name'}, $interface_alias) . "} $interface->{'traffic'}{'total'}{'rx'}\n";
	}

	print "\n# HELP vnstat_interface_total_transmitted_bytes All time total transmitted (tx) bytes\n";
	print "# TYPE vnstat_interface_total_transmitted_bytes counter\n";

	foreach my $interface ( @{ $data->{'interfaces'} } ) {
		my $interface_alias = get_interface_alias($interface);
		print "vnstat_interface_total_transmitted_bytes{" . prom_interface_labels($interface->{'name'}, $interface_alias) . "} $interface->{'traffic'}{'total'}{'tx'}\n";
	}
}

sub print_updated
{
	my ($data) = @_;

	print "\n# HELP vnstat_interface_updated_timestamp_seconds Unix time when the interface data was last updated\n";
	print "# TYPE vnstat_interface_updated_timestamp_seconds gauge\n";

	foreach my $interface ( @{ $data->{'interfaces'} } ) {
		my $interface_alias = get_interface_alias($interface);
		print "vnstat_interface_updated_timestamp_seconds{" . prom_interface_labels($interface->{'name'}, $interface_alias) . "} $interface->{'updated'}{'timestamp'}\n";
	}
}

sub normalized_monthrotate
{
	my ($monthrotate) = @_;
	if (!defined $monthrotate || $monthrotate !~ /^\d+$/ || $monthrotate < 1 || $monthrotate > 28) {
		return 1;
	}
	return int($monthrotate);
}

# JSON true only. false, a missing field, and an integer leave the year unshifted.
sub monthrotate_affects_years
{
	my ($value) = @_;
	return ref($value) eq 'JSON::PP::Boolean' && $value ? 1 : 0;
}

# Bucket label vnStat stores for $when. Five-minute labels use round(minute/5)*5,
# which can sit ahead of the clock; flooring to 300 seconds would drop a live row.
# timelocal_nocheck normalizes minute 60 and a day shifted before the 1st.
# The year label uses that day shift only when monthrotateaffectsyears is JSON true.
sub current_bucket_timestamp
{
	my ($resolution, $monthrotate, $monthrotateyears, $when) = @_;
	$when = time() unless defined $when;
	my ($sec, $min, $hour, $mday, $mon, $year) = localtime($when);

	if ($resolution eq 'fiveminute') {
		$min = int($min / 5 + 0.5) * 5;
		return timelocal_nocheck(0, $min, $hour, $mday, $mon, $year);
	}
	if ($resolution eq 'hour') {
		return timelocal_nocheck(0, 0, $hour, $mday, $mon, $year);
	}
	if ($resolution eq 'day') {
		return timelocal_nocheck(0, 0, 0, $mday, $mon, $year);
	}
	if ($resolution eq 'month') {
		my $shift = normalized_monthrotate($monthrotate) - 1;
		if ($shift) {
			($sec, $min, $hour, $mday, $mon, $year) = localtime(timelocal_nocheck($sec, $min, $hour, $mday - $shift, $mon, $year));
		}
		return timelocal_nocheck(0, 0, 0, 1, $mon, $year);
	}
	if ($resolution eq 'year') {
		my $shift = 0;
		if (monthrotate_affects_years($monthrotateyears)) {
			$shift = normalized_monthrotate($monthrotate) - 1;
		}
		if ($shift) {
			($sec, $min, $hour, $mday, $mon, $year) = localtime(timelocal_nocheck($sec, $min, $hour, $mday - $shift, $mon, $year));
		}
		return timelocal_nocheck(0, 0, 0, 1, 0, $year);
	}
	return undef;
}

sub current_resolution_value
{
	my ($interface, $resolution, $field) = @_;
	my $rows = $interface->{'traffic'}{$resolution};
	return undef unless ref($rows) eq 'ARRAY' && @{$rows};
	my $row = $rows->[0];
	return undef unless ref($row) eq 'HASH';
	return undef unless defined $row->{$field};
	my $timestamp = $row->{'timestamp'};
	return undef unless defined $timestamp && $timestamp =~ /^-?\d+$/;
	my $bucket = current_bucket_timestamp($resolution, $interface->{'monthrotate'}, $interface->{'monthrotateaffectsyears'});
	return undef unless defined $bucket && $timestamp == $bucket;
	return $row->{$field};
}

sub print_data_resolution
{
	my ($resolution, $data) = @_;
	my $output_count = 0;

	print "\n# HELP vnstat_interface_".$resolution."_received_bytes Received (rx) bytes for current $resolution\n";
	print "# TYPE vnstat_interface_".$resolution."_received_bytes gauge\n";

	$output_count = 0;
	foreach my $interface ( @{ $data->{'interfaces'} } ) {
		my $rx = current_resolution_value($interface, $resolution, 'rx');
		if (defined $rx) {
			my $interface_alias = get_interface_alias($interface);
			print "vnstat_interface_".$resolution."_received_bytes{" . prom_interface_labels($interface->{'name'}, $interface_alias) . "} $rx\n";
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
		my $tx = current_resolution_value($interface, $resolution, 'tx');
		if (defined $tx) {
			my $interface_alias = get_interface_alias($interface);
			print "vnstat_interface_".$resolution."_transmitted_bytes{" . prom_interface_labels($interface->{'name'}, $interface_alias) . "} $tx\n";
			$output_count++;
		}
	}
	if ($output_count == 0) {
		print "# no data\n";
	}
}

sub run_command
{
	my @cmd = @_;
	my $stdout = '';
	my $stderr = '';

	pipe(my $err_read, my $err_write) or return ('', $!, -1);
	my $pid = open(my $out, "-|");
	if (!defined $pid) {
		close $err_read;
		close $err_write;
		return ('', $!, -1);
	}
	if ($pid == 0) {
		close $err_read;
		open(STDERR, ">&", $err_write) or exit 127;
		close $err_write;
		exec {$cmd[0]} @cmd or exit 127;
	}
	close $err_write;
	binmode $out;
	binmode $err_read;
	{
		local $/;
		$stdout = <$out>;
		$stderr = <$err_read>;
	}
	$stdout = '' unless defined $stdout;
	$stderr = '' unless defined $stderr;
	close $out;
	my $status = $?;
	close $err_read;
	return ($stdout, $stderr, $status);
}

sub plain_response
{
	my ($status, $message) = @_;

	if (defined $status) {
		print "Status: $status\n";
	}
	print "Content-Type: text/plain\n";
	print "Cache-Control: private, no-cache\n\n$message\n";
	exit 0;
}

my @data_resolutions = ('fiveminute', 'hour', 'day', 'month', 'year');

my ($json_data, $command_stderr, $command_status) = run_command($vnstat_cmd, "--json", "s", "1");
print STDERR $command_stderr if length($command_stderr);

if ($command_status != 0) {
	plain_response("500 Internal Server Error", "Failed to read vnStat data.");
}

my $data = "";
# Keep rx and tx as the decimal digits from the JSON text.
$json_data =~ s/("(?:rx|tx)":)(\d+)/$1"$2"/g;
eval { $data = decode_json($json_data) };
if ($@) {
	plain_response("500 Internal Server Error", "Invalid command output.");
}

if (ref($data) ne 'HASH' or not defined $data->{'vnstatversion'}) {
	plain_response("500 Internal Server Error", "Expected content from command output missing.");
}

if (ref($data->{'interfaces'}) ne 'ARRAY' or not defined $data->{'interfaces'}[0]) {
	plain_response("500 Internal Server Error", "No interfaces found in command output.");
}

my $first_interface = $data->{'interfaces'}[0];
if (ref($first_interface) ne 'HASH'
	or ref($first_interface->{'created'}) ne 'HASH'
	or not defined $first_interface->{'created'}{'timestamp'}) {
	plain_response("500 Internal Server Error", "Incompatible vnStat version used.");
}

print "Content-Type: text/plain; version=0.0.4; charset=utf-8\n";
print "Cache-Control: private, no-cache\n\n";

print "# vnStat version: ".$data->{'vnstatversion'}."\n";

print_totals($data);
print_updated($data);

foreach my $data_resolution ( @data_resolutions ) {
	print_data_resolution($data_resolution, $data);
}
