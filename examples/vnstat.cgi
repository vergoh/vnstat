#!/usr/bin/perl -w

# vnstat.cgi -- example cgi for vnStat image output
# copyright (c) 2008-2024 Teemu Toivola <tst at iki dot fi>
#
# based on mailgraph.cgi
# copyright (c) 2000-2007 ETH Zurich
# copyright (c) 2000-2007 David Schweikert <dws@ee.ethz.ch>
# released under the GNU General Public License

package vnStatCGI;
use strict;

# server name in page title
# fill to set, otherwise "hostname" command output is used
my $servername = '';

# temporary directory where to store the images
my $tmp_dir = '/tmp/vnstatcgi';

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

# use black background and invert image colors when enabled, set 0 to disable,
# set 1 to enable without inverting rx and tx color, set 2 to enable and invert all colors
my $darkmode = '0';

# page auto refresh interval in seconds, set 0 to disable
my $pagerefresh = '0';

# interfaces to be shown on the index page when more than one interface exists, regular expression, leave empty to disable filter
my $indexshowninterfaces = '';

# interfaces to be hidden from the index page when more than one interface exists, regular expression, leave empty to disable filter
my $indexhiddeninterfaces = '';

# number of images to show per row on the index page when more than one interface exists, set '0' for auto fit
my $indeximagesperrow = '1';

# image output to use on the index page when more than one interface exists
my $indeximageoutput = 'hs';

# cgi script file name for httpd
# fill to override automatic detection
my $scriptname = '';


################ no user configurable settings below this line ################


my $VERSION = "1.19";
my $cssbody = "body { background-color: $bgcolor; text-align: left; display: block; }";
my $csscommonstyle = "a { text-decoration: underline; }\ntable { border: 0px; border-spacing: 0px; display: inline; }\ntd { vertical-align: top; padding: 0px; }\nimg { border: 0px; vertical-align: top; margin: 4px 4px; }";
my $csscolors = "a:link { color: #b0b0b0; }\na:visited { color: #b0b0b0; }\na:hover { color: #000000; }\nsmall { display: inline; font-size: 8px; color: #cbcbcb; padding: 0px 4px; }";
my $metarefresh = "";

if ($darkmode == '1' or $darkmode == '2') {
	$bgcolor = "black";
	$cssbody = "body { background-color: $bgcolor; text-align: left; display: block; }";
	$csscolors = "a:link { color: #707070; }\na:visited { color: #707070; }\na:hover { color: #ffffff; }\nsmall { display: inline; font-size: 8px; color: #606060; padding: 0px 4px; }";
}

sub graph
{
	my ($interface, $file, $param) = @_;
	my $result = '';

	my $fontparam = '--small';
	if ($largefonts == '1') {
		$fontparam = '--large';
	}

	if (defined $interface and defined $file and defined $param) {
		$result = `"$vnstati_cmd" -i "$interface" -c $cachetime $param $fontparam --invert-colors $darkmode -o "$file"`;
	} else {
		show_error("ERROR: invalid input");
	}
	return $result;
}

sub send_image
{
	my ($file, $output) = @_;

	if ($file ne '-') {
		-r $file or do {
			show_error("ERROR: can't find $file");
		};

		print "Content-type: image/png\n";
		print "Content-length: ".((stat($file))[7])."\n";
		print "\n";
		open(my $IMG_FILE, "<", $file) or die;
		my $data;
		print $data while read($IMG_FILE, $data, 16384)>0;
	} else {
		if (length($output) < 1000) {
			show_error("ERROR: command failed: $output");
		}

		print "Content-type: image/png\n";
		print "Content-length: ".(length($output))."\n";
		print "\n";
		print $output;
	}
}

sub handle_image
{
	my ($interface, $file, $param) = @_;

	if ($cachetime == '0') {
		$file = '-';
	}

	my $output = graph($interface, $file, $param);
	send_image($file, $output);
}

sub show_error
{
	my ($error_msg) = @_;
	print "Content-type: text/plain\n\n$error_msg\n";
	exit 1;
}

sub print_interface_list_html
{
	my @interfaces = @vnStatCGI::interfaces;

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
$csscolors
$cssbody
-->
</style>
</head>
HEADER
	print "<body>\n<br>\n";
	my $interfacesshown = 0;
	my $lineended = 0;
	for my $i (0..$#interfaces) {
		if (length($indexshowninterfaces) > 0 && $interfaces[${i}] !~ /$indexshowninterfaces/) {
			next;
		}
		if (length($indexhiddeninterfaces) > 0 && $interfaces[${i}] =~ /$indexhiddeninterfaces/) {
			next;
		}
		print "<a href=\"${scriptname}?${i}-f\"><img src=\"${scriptname}?${i}-$indeximageoutput\" alt=\"$interfaces[${i}]\"></a>";
		$interfacesshown++;
		if ($indeximagesperrow > 0 && $interfacesshown % $indeximagesperrow == 0) {
			print "<br>\n";
			$lineended = 1;
		} else {
			$lineended = 0;
		}
	}
	if (!$lineended) {
		print "<br>\n";
	}

	print <<FOOTER;
<small>Images generated using <a href="https://humdi.net/vnstat/">vnStat</a> image output.</small><br>
</body>
</html>
FOOTER
}

sub print_single_interface_html
{
	my ($interface) = @_;
	my @interfaces = @vnStatCGI::interfaces;

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
$csscolors
$cssbody
-->
</style>
</head>
HEADER
	print "<body>\n<br>\n";
	print "<table>\n<tr><td>\n";
	print "<img src=\"${scriptname}?${interface}-s\" alt=\"$interfaces[${interface}] summary\"><br>\n";
	print "<a href=\"${scriptname}?s-${interface}-d-l\"><img src=\"${scriptname}?${interface}-d\" alt=\"$interfaces[${interface}] daily\"></a><br>\n";
	print "<a href=\"${scriptname}?s-${interface}-t-l\"><img src=\"${scriptname}?${interface}-t\" alt=\"$interfaces[${interface}] top 10\"></a><br>\n";
	print "</td><td>\n";
	print "<a href=\"${scriptname}?s-${interface}-h\"><img src=\"${scriptname}?${interface}-hg\" alt=\"$interfaces[${interface}] hourly\"></a><br>\n";
	print "<a href=\"${scriptname}?s-${interface}-5\"><img src=\"${scriptname}?${interface}-5g\" alt=\"$interfaces[${interface}] 5 minute\"></a><br>\n";
	print "<a href=\"${scriptname}?s-${interface}-m-l\"><img src=\"${scriptname}?${interface}-m\" alt=\"$interfaces[${interface}] monthly\"></a><br>\n";
	print "<a href=\"${scriptname}?s-${interface}-y-l\"><img src=\"${scriptname}?${interface}-y\" alt=\"$interfaces[${interface}] yearly\"></a><br>\n";
	print "</td></tr>\n</table>\n";

	print <<FOOTER;
<br>
<small>Images generated using <a href="https://humdi.net/vnstat/">vnStat</a> image output.</small><br>
</body>
</html>
FOOTER
}

sub print_single_image_html
{
	my ($image) = @_;
	my $interface = "-1";
	my $content = "";
	my @interfaces = @vnStatCGI::interfaces;

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
$csscolors
$cssbody
-->
</style>
</head>
HEADER
	print "<body>\n<br>\n";
	print "<table>\n<tr><td>\n";
	print "<img src=\"${scriptname}?${image}\" alt=\"$interfaces[${interface}] ", lc($content), "\">\n";
	print "</td></tr>\n</table>\n";

	print <<FOOTER;
<br>
<small>Image generated using <a href="https://humdi.net/vnstat/">vnStat</a> image output.</small><br>
</body>
</html>
FOOTER
}

sub main
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

	if (not defined $vnStatCGI::interfaces) {
		our @interfaces = `$vnstati_cmd --dbiflist 1`;
	}
	chomp @vnStatCGI::interfaces;
	my @interfaces = @vnStatCGI::interfaces;

	if (length($servername) == 0) {
		$servername = `hostname`;
		chomp $servername;
	}

	if ($aligncenter != '0') {
		$cssbody = "body { background-color: $bgcolor; text-align: center; display: block; }";
	}

	if ($pagerefresh != '0') {
		$metarefresh = "\n<meta http-equiv=\"refresh\" content=\"$pagerefresh\">";
	}

	if ($cachetime != '0') {
		mkdir $tmp_dir, 0755 unless -d $tmp_dir;
	}

	my $query = $ENV{QUERY_STRING};
	if (defined $query and $query =~ /\S/) {
		if ($query =~ /^(\d+)-s$/) {
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1.png", "-s");
		}
		elsif ($query =~ /^(\d+)-hs$/) {
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_hs.png", "-hs");
		}
		elsif ($query =~ /^(\d+)-hsh$/) {
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_hsh.png", "-hs 0");
		}
		elsif ($query =~ /^(\d+)-hs5$/) {
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_hs5.png", "-hs 1");
		}
		elsif ($query =~ /^(\d+)-vs$/) {
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_vs.png", "-vs");
		}
		elsif ($query =~ /^(\d+)-vsh$/) {
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_vsh.png", "-vs 0");
		}
		elsif ($query =~ /^(\d+)-vs5$/) {
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_vs5.png", "-vs 1");
		}
		elsif ($query =~ /^(\d+)-d$/) {
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_d.png", "-d 30");
		}
		elsif ($query =~ /^(\d+)-d-l$/) {
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_d_l.png", "-d 60");
		}
		elsif ($query =~ /^(\d+)-m$/) {
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_m.png", "-m 12");
		}
		elsif ($query =~ /^(\d+)-m-l$/) {
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_m_l.png", "-m 24");
		}
		elsif ($query =~ /^(\d+)-t$/) {
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_t.png", "-t 10");
		}
		elsif ($query =~ /^(\d+)-t-l$/) {
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_t_l.png", "-t 20");
		}
		elsif ($query =~ /^(\d+)-h$/) {
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_h.png", "-h 48");
		}
		elsif ($query =~ /^(\d+)-hg$/) {
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_hg.png", "-hg");
		}
		elsif ($query =~ /^(\d+)-5$/) {
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_5.png", "-5 60");
		}
		elsif ($query =~ /^(\d+)-5g$/) {
			if ($largefonts == '1') {
				handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_5g.png", "-5g 576 300");
			} else {
				handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_5g.png", "-5g 422 250");
			}
		}
		elsif ($query =~ /^(\d+)-y$/) {
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_y.png", "-y 5");
		}
		elsif ($query =~ /^(\d+)-y-l$/) {
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_y_l.png", "-y 0");
		}
		elsif ($query =~ /^(\d+)-95rx$/) {
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_95rx.png", "--95th 0");
		}
		elsif ($query =~ /^(\d+)-95tx$/) {
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_95tx.png", "--95th 1");
		}
		elsif ($query =~ /^(\d+)-95total$/) {
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_95total.png", "--95th 2");
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
