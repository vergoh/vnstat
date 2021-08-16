#!/usr/bin/perl -w

# vnstat.cgi -- example cgi for vnStat image output
# copyright (c) 2008-2021 Teemu Toivola <tst at iki dot fi>
#
# based on mailgraph.cgi
# copyright (c) 2000-2007 ETH Zurich
# copyright (c) 2000-2007 David Schweikert <dws@ee.ethz.ch>
# released under the GNU General Public License


# server name in page title
# fill to set, otherwise "hostname" command output is used
my $servername = '';

# temporary directory where to store the images
my $tmp_dir = '/tmp/vnstatcgi';

# location of "vnstat" binary
my $vnstat_cmd = '/usr/bin/vnstat';

# location of "vnstati" binary
my $vnstati_cmd = '/usr/bin/vnstati';

# image cache time in minutes, set 0 to disable
my $cachetime = '0';

# shown interfaces, interface specific pages can be accessed directly
# by using /interfacename as suffix for the cgi if the httpd supports PATH_INFO
# for static list, uncomment and update the list
#my @interfaces = ('eth0', 'eth1');

# center images on page instead of left alignment, set 0 to disable
my $aligncenter = '1';

# use large fonts, set 1 to enable
my $largefonts = '0';

# page background color
my $bgcolor = "white";

# page auto refresh interval in seconds, set 0 to disable
my $pagerefresh = '0';

# cgi script file name for httpd
# fill to override automatic detection
my $scriptname = '';

################


my $VERSION = "1.14";
my $cssbody = "body { background-color: $bgcolor; }";
my $csscommonstyle = <<CSS;
a { text-decoration: underline; }
a:link { color: #b0b0b0; }
a:visited { color: #b0b0b0; }
a:hover { color: #000000; }
small { font-size: 8px; color: #cbcbcb; }
img { border: 0; vertical-align: top; }
table { border: 0; }
table td { vertical-align: top; }
small { display: block; }
CSS
my $metarefresh = "";

sub graph($$$)
{
	my ($interface, $file, $param) = @_;

	my $fontparam = '--small';
	if ($largefonts == '1') {
		$fontparam = '--large';
	}

	if (defined $interface and defined $file and defined $param) {
		my $result = `"$vnstati_cmd" -i "$interface" -c $cachetime $param $fontparam -o "$file"`;
	} else {
		show_error("ERROR: invalid input");
	}
}

sub print_interface_list_html()
{
	print "Content-Type: text/html\n\n";

	print <<HEADER;
<!DOCTYPE html>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">$metarefresh
<meta name="generator" content="vnstat.cgi $VERSION">
<title>Traffic Statistics for $servername</title>
<style>
<!--
$csscommonstyle
$cssbody
-->
</style>
</head>
HEADER

	for my $i (0..$#interfaces) {
		print "<p><a href=\"${scriptname}?${i}-f\"><img src=\"${scriptname}?${i}-hs\" alt=\"$interfaces[${i}] summary\"></a></p>\n";
	}

	print <<FOOTER;
<small style=\"padding: 4px 4px\">Images generated using <a href="https://humdi.net/vnstat/">vnStat</a> image output.</small>
<br><br>
</body>
</html>
FOOTER
}

sub print_single_interface_html($)
{
	my ($interface) = @_;

	print "Content-Type: text/html\n\n";

	print <<HEADER;
<!DOCTYPE html>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">$metarefresh
<meta name="generator" content="vnstat.cgi $VERSION">
<title>Traffic Statistics for $servername - $interfaces[${interface}]</title>
<style>
<!--
$csscommonstyle
$cssbody
-->
</style>
</head>
HEADER

	print "<table><tr><td>\n";
	print "<img src=\"${scriptname}?${interface}-s\" alt=\"$interfaces[${interface}] summary\"><br>\n";
	print "<a href=\"${scriptname}?s-${interface}-d-l\"><img src=\"${scriptname}?${interface}-d\" alt=\"$interfaces[${interface}] daily\" style=\"margin: 4px 0px\"></a><br>\n";
	print "<a href=\"${scriptname}?s-${interface}-t-l\"><img src=\"${scriptname}?${interface}-t\" alt=\"$interfaces[${interface}] top 10\"></a><br>\n";
	print "</td><td>\n";
	print "<a href=\"${scriptname}?s-${interface}-h\"><img src=\"${scriptname}?${interface}-hg\" alt=\"$interfaces[${interface}] hourly\"></a><br>\n";
	print "<a href=\"${scriptname}?s-${interface}-5\"><img src=\"${scriptname}?${interface}-5g\" alt=\"$interfaces[${interface}] 5 minute\" style=\"margin: 4px 0px\"></a><br>\n";
	print "<a href=\"${scriptname}?s-${interface}-m-l\"><img src=\"${scriptname}?${interface}-m\" alt=\"$interfaces[${interface}] monthly\"></a><br>\n";
	print "<a href=\"${scriptname}?s-${interface}-y-l\"><img src=\"${scriptname}?${interface}-y\" alt=\"$interfaces[${interface}] yearly\" style=\"margin: 4px 0px\"></a><br>\n";
	print "</td></tr>\n</table>\n";

	print <<FOOTER;
<small style=\"padding: 12px 4px\">Images generated using <a href="https://humdi.net/vnstat/">vnStat</a> image output.</small>
<br><br>
</body>
</html>
FOOTER
}

sub print_single_image_html($)
{
	my ($image) = @_;
	my $interface = "-1";
	my $content = "";

	if ($image =~ /^(\d+)-/) {
		$interface = $1;
	} else {
		show_error("ERROR: invalid query");
	}

	if ($image =~ /^\d+-5/) {
		$content = "5 Minute";
	} elsif ($image =~ /^\d+-h/) {
		$content = "Hourly";
	} elsif ($image =~ /^\d+-d/) {
		$content = "Daily";
	} elsif ($image =~ /^\d+-m/) {
		$content = "Monthly";
	} elsif ($image =~ /^\d+-y/) {
		$content = "Yearly";
	} elsif ($image =~ /^\d+-t/) {
		$content = "Daily Top";
	} else {
		show_error("ERROR: invalid query type");
	}

	print "Content-Type: text/html\n\n";

	print <<HEADER;
<!DOCTYPE html>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">$metarefresh
<meta name="generator" content="vnstat.cgi $VERSION">
<title>$content Traffic Statistics for $servername - $interfaces[${interface}]</title>
<style>
<!--
$csscommonstyle
$cssbody
-->
</style>
</head>
HEADER

	print "<table><tr><td>\n";
	print "<img src=\"${scriptname}?${image}\" alt=\"$interfaces[${interface}] ", lc($content), "\">\n";
	print "</td></tr>\n</table>\n";

	print <<FOOTER;
<small style=\"padding: 12px 4px\">Image generated using <a href="https://humdi.net/vnstat/">vnStat</a> image output.</small>
<br><br>
</body>
</html>
FOOTER
}

sub send_image($)
{
	my ($file) = @_;

	-r $file or do {
		show_error("ERROR: can't find $file");
	};

	print "Content-type: image/png\n";
	print "Content-length: ".((stat($file))[7])."\n";
	print "\n";
	open(IMG, $file) or die;
	my $data;
	print $data while read(IMG, $data, 16384)>0;
}

sub show_error($)
{
	my ($error_msg) = @_;
	print "Content-type: text/plain\n\n$error_msg\n";
	exit 1;
}

sub main()
{
	if (length($scriptname) == 0) {
		if (defined $ENV{REQUEST_URI}) {
			($scriptname) = split(/\?/, $ENV{REQUEST_URI});
		} else {
			($scriptname) = $ENV{SCRIPT_NAME} =~ /([^\/]*)$/;
		}
		if ($scriptname =~ /\/$/) {
			$scriptname = '';
		}
	}

	if (not defined $interfaces) {
		our @interfaces = `$vnstat_cmd --dbiflist 1`;
	}
	chomp @interfaces;

	if (length($servername) == 0) {
		$servername = `hostname`;
		chomp $servername;
	}

	if ($aligncenter != '0') {
		$cssbody = "html { display: table; width: 100%; }\nbody { background-color: $bgcolor; display: table-cell; text-align: center; vertical-align: middle; }\ntable { margin-left: auto; margin-right: auto; margin-top: 10px; }";
	}

	if ($pagerefresh != '0') {
		$metarefresh = "\n<meta http-equiv=\"refresh\" content=\"$pagerefresh\">";
	}

	mkdir $tmp_dir, 0755 unless -d $tmp_dir;

	my $query = $ENV{QUERY_STRING};
	if (defined $query and $query =~ /\S/) {
		if ($query =~ /^(\d+)-s$/) {
			my $file = "$tmp_dir/vnstat_$1.png";
			graph($interfaces[$1], $file, "-s");
			send_image($file);
		}
		elsif ($query =~ /^(\d+)-hs$/) {
			my $file = "$tmp_dir/vnstat_$1_hs.png";
			graph($interfaces[$1], $file, "-hs");
			send_image($file);
		}
		elsif ($query =~ /^(\d+)-hsh$/) {
			my $file = "$tmp_dir/vnstat_$1_hsh.png";
			graph($interfaces[$1], $file, "-hs 0");
			send_image($file);
		}
		elsif ($query =~ /^(\d+)-hs5$/) {
			my $file = "$tmp_dir/vnstat_$1_hs5.png";
			graph($interfaces[$1], $file, "-hs 1");
			send_image($file);
		}
		elsif ($query =~ /^(\d+)-vs$/) {
			my $file = "$tmp_dir/vnstat_$1_vs.png";
			graph($interfaces[$1], $file, "-vs");
			send_image($file);
		}
		elsif ($query =~ /^(\d+)-vsh$/) {
			my $file = "$tmp_dir/vnstat_$1_vsh.png";
			graph($interfaces[$1], $file, "-vs 0");
			send_image($file);
		}
		elsif ($query =~ /^(\d+)-vs5$/) {
			my $file = "$tmp_dir/vnstat_$1_vs5.png";
			graph($interfaces[$1], $file, "-vs 1");
			send_image($file);
		}
		elsif ($query =~ /^(\d+)-d$/) {
			my $file = "$tmp_dir/vnstat_$1_d.png";
			graph($interfaces[$1], $file, "-d 30");
			send_image($file);
		}
		elsif ($query =~ /^(\d+)-d-l$/) {
			my $file = "$tmp_dir/vnstat_$1_d_l.png";
			graph($interfaces[$1], $file, "-d 60");
			send_image($file);
		}
		elsif ($query =~ /^(\d+)-m$/) {
			my $file = "$tmp_dir/vnstat_$1_m.png";
			graph($interfaces[$1], $file, "-m 12");
			send_image($file);
		}
		elsif ($query =~ /^(\d+)-m-l$/) {
			my $file = "$tmp_dir/vnstat_$1_m_l.png";
			graph($interfaces[$1], $file, "-m 24");
			send_image($file);
		}
		elsif ($query =~ /^(\d+)-t$/) {
			my $file = "$tmp_dir/vnstat_$1_t.png";
			graph($interfaces[$1], $file, "-t 10");
			send_image($file);
		}
		elsif ($query =~ /^(\d+)-t-l$/) {
			my $file = "$tmp_dir/vnstat_$1_t_l.png";
			graph($interfaces[$1], $file, "-t 20");
			send_image($file);
		}
		elsif ($query =~ /^(\d+)-h$/) {
			my $file = "$tmp_dir/vnstat_$1_h.png";
			graph($interfaces[$1], $file, "-h 48");
			send_image($file);
		}
		elsif ($query =~ /^(\d+)-hg$/) {
			my $file = "$tmp_dir/vnstat_$1_hg.png";
			graph($interfaces[$1], $file, "-hg");
			send_image($file);
		}
		elsif ($query =~ /^(\d+)-5$/) {
			my $file = "$tmp_dir/vnstat_$1_5.png";
			graph($interfaces[$1], $file, "-5 60");
			send_image($file);
		}
		elsif ($query =~ /^(\d+)-5g$/) {
			my $file = "$tmp_dir/vnstat_$1_5g.png";
			if ($largefonts == '1') {
				graph($interfaces[$1], $file, "-5g 576 300");
			} else {
				graph($interfaces[$1], $file, "-5g 422 250");
			}
			send_image($file);
		}
		elsif ($query =~ /^(\d+)-y$/) {
			my $file = "$tmp_dir/vnstat_$1_y.png";
			graph($interfaces[$1], $file, "-y 5");
			send_image($file);
		}
		elsif ($query =~ /^(\d+)-y-l$/) {
			my $file = "$tmp_dir/vnstat_$1_y_l.png";
			graph($interfaces[$1], $file, "-y 0");
			send_image($file);
		}
		elsif ($query =~ /^(\d+)-f$/) {
			print_single_interface_html($1);
		}
		elsif ($query =~ /^s-(.+)/) {
			print_single_image_html($1);
		}
		else {
			show_error("ERROR: invalid argument");
		}
	}
	else {
		my $html_shown = 0;
		if (defined $ENV{PATH_INFO}) {
			my @fields = split(/\//, $ENV{PATH_INFO});
			my $interface = $fields[-1];
			for my $i (0..$#interfaces) {
				if ($interfaces[${i}] eq $interface) {
					print_single_interface_html($i);
					$html_shown = 1;
					last;
				}
			}
			if ($html_shown == 0) {
				show_error("ERROR: no such interface: $interface");
			}
		}

		if ($html_shown == 0 and scalar @interfaces == 1) {
			print_single_interface_html(0);
		}
		elsif ($html_shown == 0) {
			print_interface_list_html();
		}
	}
}

main();
