#!/usr/bin/perl -w

# vnstat.cgi -- example cgi for vnStat image output
# copyright (c) 2008-2011 Teemu Toivola <tst at iki dot fi>
#
# based on mailgraph.cgi
# copyright (c) 2000-2007 ETH Zurich
# copyright (c) 2000-2007 David Schweikert <dws@ee.ethz.ch>
# released under the GNU General Public License


my $host = 'Some Server';
my $scriptname = 'vnstat.cgi';

# temporary directory where to store the images
my $tmp_dir = '/tmp/vnstatcgi';

# location of vnstati
my $vnstati_cmd = '/usr/bin/vnstati';

# cache time in minutes, set 0 to disable
my $cachetime = '15';

# shown interfaces, remove unnecessary lines
my @graphs = (
        { interface => 'eth0' },
        { interface => 'eth1' },
);


################


my $VERSION = "1.3";

sub graph($$$)
{
	my ($interface, $file, $param) = @_;
	my $result = `"$vnstati_cmd" -i "$interface" -c $cachetime $param -o "$file"`;
}


sub print_html()
{
	print "Content-Type: text/html\n\n";

	print <<HEADER;
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<meta name="Generator" content="vnstat.cgi $VERSION">
<title>Traffic Statistics for $host</title>
<style type="text/css">
<!--
a { text-decoration: underline; }
a:link { color: #b0b0b0; }
a:visited { color: #b0b0b0; }
a:hover { color: #000000; }
small { font-size: 8px; color: #cbcbcb; }
-->
</style>
</head>
<body bgcolor="#ffffff">
HEADER

	for my $n (0..$#graphs) {
		print "<p><a href=\"$scriptname?${n}-f\"><img src=\"$scriptname?${n}-hs\" border=\"0\" alt=\"$graphs[$n]{interface} summary\"></a></p>\n";
	}

	print <<FOOTER;
<small>Images generated using <a href="http://humdi.net/vnstat/">vnStat</a> image output.</small>
</body>
</html>
FOOTER
}

sub print_fullhtml($)
{
	my ($interface) = @_;

	print "Content-Type: text/html\n\n";

	print <<HEADER;
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<meta name="Generator" content="vnstat.cgi $VERSION">
<title>Traffic Statistics for $host</title>
<style type="text/css">
<!--
a { text-decoration: underline; }
a:link { color: #b0b0b0; }
a:visited { color: #b0b0b0; }
a:hover { color: #000000; }
small { font-size: 8px; color: #cbcbcb; }
-->
</style>
</head>
<body bgcolor="#ffffff">
HEADER

	print "<table border=\"0\"><tr><td>\n";
	print "<img src=\"$scriptname?${interface}-s\" border=\"0\" alt=\"${interface} summary\">";
	print "</td><td>\n";
	print "<img src=\"$scriptname?${interface}-h\" border=\"0\" alt=\"${interface} hourly\">";
	print "</td></tr><tr><td valign=\"top\">\n";
	print "<img src=\"$scriptname?${interface}-d\" border=\"0\" alt=\"${interface} daily\">";
	print "</td><td valign=\"top\">\n";
	print "<img src=\"$scriptname?${interface}-t\" border=\"0\" alt=\"${interface} top 10\"><br>\n";
	print "<img src=\"$scriptname?${interface}-m\" border=\"0\" alt=\"${interface} monthly\" vspace=\"4\">";
	print "</td></tr>\n</table>\n";

	print <<FOOTER;
<small><br>&nbsp;Images generated using <a href="http://humdi.net/vnstat/">vnStat</a> image output.</small>
</body>
</html>
FOOTER
}

sub send_image($)
{
	my ($file)= @_;

	-r $file or do {
		print "Content-type: text/plain\n\nERROR: can't find $file\n";
		exit 1;
	};

	print "Content-type: image/png\n";
	print "Content-length: ".((stat($file))[7])."\n";
	print "\n";
	open(IMG, $file) or die;
	my $data;
	print $data while read(IMG, $data, 16384)>0;
}

sub main()
{
	mkdir $tmp_dir, 0755 unless -d $tmp_dir;

	my $img = $ENV{QUERY_STRING};
	if(defined $img and $img =~ /\S/) {
		if($img =~ /^(\d+)-s$/) {
			my $file = "$tmp_dir/vnstat_$1.png";
			graph($graphs[$1]{interface}, $file, "-s");
			send_image($file);
		}
		elsif($img =~ /^(\d+)-hs$/) {
			my $file = "$tmp_dir/vnstat_$1_hs.png";
			graph($graphs[$1]{interface}, $file, "-hs");
			send_image($file);
		}
		elsif($img =~ /^(\d+)-d$/) {
			my $file = "$tmp_dir/vnstat_$1_d.png";
			graph($graphs[$1]{interface}, $file, "-d");
			send_image($file);
		}
		elsif($img =~ /^(\d+)-m$/) {
			my $file = "$tmp_dir/vnstat_$1_m.png";
			graph($graphs[$1]{interface}, $file, "-m");
			send_image($file);
		}
		elsif($img =~ /^(\d+)-t$/) {
			my $file = "$tmp_dir/vnstat_$1_t.png";
			graph($graphs[$1]{interface}, $file, "-t");
			send_image($file);
		}
		elsif($img =~ /^(\d+)-h$/) {
			my $file = "$tmp_dir/vnstat_$1_h.png";
			graph($graphs[$1]{interface}, $file, "-h");
			send_image($file);
		}
		elsif($img =~ /^(\d+)-f$/) {
			print_fullhtml($1);
		}
		else {
			die "ERROR: invalid argument\n";
		}
	}
	else {
		if ($#graphs == 0) {
			print_fullhtml(0);
		} else {
			print_html();
		}
	}
}

main();
