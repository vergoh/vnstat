#!/usr/bin/perl -w

# vnstat-json.cgi -- example cgi for vnStat json output
# copyright (c) 2015-2026 Teemu Toivola <tst at iki dot fi>
# released under the GNU General Public License

use strict;
use JSON::PP;

# location of vnstat binary
my $vnstat_cmd = '/usr/bin/vnstat';

# individually accessible interfaces with ?interface=N or /interfacename suffix
# for static list, uncomment and update the list
#our @interfaces = ('eth0', 'eth1');


################


sub plain_response
{
	my ($status, $message) = @_;

	if (defined $status) {
		print "Status: $status\n";
	}
	print "Content-Type: text/plain\n\n$message\n";
	exit 0;
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

sub load_interfaces
{
	open(my $iflist, "-|", $vnstat_cmd, "--dbiflist", "1")
		or plain_response("500 Internal Server Error", "Failed to list interfaces.");
	my @lines = <$iflist>;
	close $iflist;
	my $failed = ($? != 0);
	chomp @lines;
	@lines = grep { length $_ } @lines;
	if ($failed or grep { /^Error:/ } @lines) {
		plain_response("500 Internal Server Error", "Failed to list interfaces.");
	}
	if (!@lines) {
		plain_response(undef, "Database is empty.");
	}
	return @lines;
}

{
	if (!@main::interfaces) {
		@main::interfaces = load_interfaces();
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

	my ($json_data, $command_stderr, $command_status) = run_command(@command);
	print STDERR $command_stderr if length($command_stderr);

	if ($command_status != 0) {
		plain_response("500 Internal Server Error", "Failed to read vnStat data.");
	}

	eval { decode_json($json_data) };
	if ($@) {
		plain_response("500 Internal Server Error", "Invalid command output.");
	}

	print "Content-Type: application/json\n\n";
	print $json_data;
}
