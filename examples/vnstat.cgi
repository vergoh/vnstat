#!/usr/bin/perl -w

# vnstat.cgi -- example cgi for vnStat image output
# copyright (c) 2008-2026 Teemu Toivola <tst at iki dot fi>
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

# follow browser or system light/dark theme, set 0 to disable,
# set 1 to enable: dark theme keeps $darkmode when it is 1 and otherwise uses 2,
# light theme forces $darkmode to 0
my $autodarkmode = '1';

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

# use configuration file defined list lengths instead of hardcoded values on single image pages, set 1 to enable
my $usecfglengthonsingleimagepages = '0';

# cgi script file name for httpd
# fill to override automatic detection
my $scriptname = '';


################ no user configurable settings below this line ################


my $VERSION = "1.22";
my $cssbody = "html { background-color: $bgcolor; color-scheme: light; }\nbody { background-color: $bgcolor; text-align: left; display: block; }";
my $csscommonstyle = "a { text-decoration: underline; }\ntable { border: 0px; border-spacing: 0px; display: inline; }\ntd { vertical-align: top; padding: 0px; }\nimg { border: 0px; vertical-align: top; margin: 4px 4px; }";
my $csscolors = "a:link { color: #b0b0b0; }\na:visited { color: #b0b0b0; }\na:hover { color: #000000; }\nsmall { display: inline; font-size: 8px; color: #cbcbcb; padding: 0px 4px; }";
my $cssthemeswitch = "";
my $metarefresh = "";
my $themeheaders = "";
my $themecookiescript = "";
my $themetogglescript = "\n<script>\n"
	. "document.addEventListener('DOMContentLoaded', function () {\n"
	. "\tvar sw = document.getElementById('theme-switch');\n"
	. "\tif (!sw) {\n"
	. "\t\treturn;\n"
	. "\t}\n"
	. "\tsw.addEventListener('click', function () {\n"
	. "\t\tvar next = sw.getAttribute('aria-checked') === 'true' ? 'light' : 'dark';\n"
	. "\t\tdocument.cookie = 'vnstat_theme=' + next + '; Path=/; Max-Age=31536000; SameSite=Lax';\n"
	. "\t\tlocation.reload();\n"
	. "\t});\n"
	. "});\n"
	. "</script>";

sub client_color_scheme
{
	if (defined $ENV{HTTP_SEC_CH_PREFERS_COLOR_SCHEME}) {
		my $hint = lc($ENV{HTTP_SEC_CH_PREFERS_COLOR_SCHEME});
		if ($hint eq 'dark' or $hint eq 'light') {
			return ($hint, 1);
		}
	}
	if (defined $ENV{HTTP_COOKIE} and $ENV{HTTP_COOKIE} =~ /(?:^|;\s*)vnstat_color_scheme=(dark|light)(?:;|$)/) {
		return ($1, 0);
	}
	return ('', 0);
}

my $configured_darkmode = $darkmode;
my $themeoverride = '';
if (defined $ENV{HTTP_COOKIE} and $ENV{HTTP_COOKIE} =~ /(?:^|;\s*)vnstat_theme=(dark|light)(?:;|$)/) {
	$themeoverride = $1;
}

if ($themeoverride eq 'dark') {
	if ($configured_darkmode != '1') {
		$darkmode = '2';
	}
} elsif ($themeoverride eq 'light') {
	$darkmode = '0';
} elsif ($autodarkmode == '1') {
	my ($colorscheme, $colorschemefromhint) = client_color_scheme();
	if ($colorscheme eq 'dark') {
		if ($darkmode != '1') {
			$darkmode = '2';
		}
	} elsif ($colorscheme eq 'light') {
		$darkmode = '0';
	}

	if ($colorschemefromhint == 0) {
		$themecookiescript = "\n<script>\n"
			. "(function () {\n"
			. "\tvar want = window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light';\n"
			. "\tvar detected = '$colorscheme';\n"
			. "\tif (detected !== want) {\n"
			. "\t\tdocument.cookie = 'vnstat_color_scheme=' + want + '; Path=/; SameSite=Lax';\n"
			. "\t\tif (document.cookie.indexOf('vnstat_color_scheme=' + want) !== -1) {\n"
			. "\t\t\tlocation.reload();\n"
			. "\t\t}\n"
			. "\t}\n"
			. "\twindow.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', function (e) {\n"
			. "\t\tdocument.cookie = 'vnstat_color_scheme=' + (e.matches ? 'dark' : 'light') + '; Path=/; SameSite=Lax';\n"
			. "\t\tlocation.reload();\n"
			. "\t});\n"
			. "})();\n"
			. "</script>";
	}
}

if ($autodarkmode == '1') {
	$themeheaders = "Accept-CH: Sec-CH-Prefers-Color-Scheme\nCritical-CH: Sec-CH-Prefers-Color-Scheme\nVary: Sec-CH-Prefers-Color-Scheme\n";
}

my $switch_track = "#e6e6e6";
my $switch_border = "#c8c8c8";
my $switch_thumb = "#ffffff";
my $switch_icon = "#1a1a1a";
my $switch_focus = "#1a1a1a";
my $switch_shadow = "0 1px 2px rgba(0, 0, 0, 0.28)";

if ($darkmode == '1' or $darkmode == '2') {
	$bgcolor = "black";
	$cssbody = "html { background-color: $bgcolor; color-scheme: dark; }\nbody { background-color: $bgcolor; text-align: left; display: block; }";
	$csscolors = "a:link { color: #707070; }\na:visited { color: #707070; }\na:hover { color: #ffffff; }\nsmall { display: inline; font-size: 8px; color: #606060; padding: 0px 4px; }";
	$switch_track = "#242424";
	$switch_border = "#3a3a3a";
	$switch_thumb = "#3c3c3c";
	$switch_icon = "#c8c8c8";
	$switch_focus = "#a0a0a0";
	$switch_shadow = "none";
}

$cssthemeswitch = "button.theme-switch { position: fixed; top: 10px; right: 10px; z-index: 2; width: 52px; height: 28px; margin: 0; padding: 0; border: 1px solid $switch_border; border-radius: 999px; background: $switch_track; color: $switch_icon; cursor: pointer; box-sizing: border-box; appearance: none; -webkit-appearance: none; }\n"
	. "button.theme-switch:focus-visible { outline: 2px solid $switch_focus; outline-offset: 2px; }\n"
	. "button.theme-switch .theme-switch-thumb { position: absolute; top: 2px; left: 2px; z-index: 0; width: 22px; height: 22px; border-radius: 50%; background: $switch_thumb; box-shadow: $switch_shadow; }\n"
	. "button.theme-switch[aria-checked=true] .theme-switch-thumb { left: 26px; }\n"
	. "button.theme-switch .theme-switch-sun, button.theme-switch .theme-switch-moon { position: absolute; top: 6px; z-index: 1; width: 14px; height: 14px; pointer-events: none; }\n"
	. "button.theme-switch .theme-switch-sun { left: 6px; }\n"
	. "button.theme-switch .theme-switch-moon { left: 30px; }\n"
	. "button.theme-switch[aria-checked=false] .theme-switch-moon, button.theme-switch[aria-checked=true] .theme-switch-sun { display: none; }\n"
	. "button.theme-switch svg { display: block; width: 14px; height: 14px; }\n";

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

sub one_line
{
	my ($text) = @_;
	return '' unless defined $text;
	$text =~ s/\s+/ /g;
	$text =~ s/^ //;
	$text =~ s/ $//;
	return $text;
}

sub graph
{
	my ($interface, $file, $param) = @_;

	my $fontparam = '--small';
	if ($largefonts == '1') {
		$fontparam = '--large';
	}
	$fontparam .= ' --common-width';

	if (defined $interface and defined $file and defined $param) {
		my @args = (
			$vnstati_cmd, "-i", $interface, "-c", $cachetime,
			split(/\s+/, $param),
			split(/\s+/, $fontparam),
			"--invert-colors", $darkmode, "-o", $file
		);
		return run_command(@args);
	}
	show_error("ERROR: invalid input");
}

sub send_image
{
	my ($file, $output, $stderr, $status) = @_;

	if ($file ne '-') {
		open(my $IMG_FILE, "<", $file) or show_error("ERROR: can't find $file");

		print "Content-type: image/png\n";
		print "Content-length: ".((stat($IMG_FILE))[7])."\n";
		print $themeheaders;
		print "\n";
		my $data;
		print $data while read($IMG_FILE, $data, 16384)>0;
		close $IMG_FILE;
	} else {
		if ($status != 0) {
			my $detail = one_line($stderr);
			if (length($detail)) {
				show_error("ERROR: command failed: $detail");
			}
			show_error("ERROR: command failed");
		}
		if (length($output) < 1000) {
			show_error("ERROR: command failed: $output");
		}

		print "Content-type: image/png\n";
		print "Content-length: ".(length($output))."\n";
		print $themeheaders;
		print "\n";
		print $output;
	}
}

sub handle_image
{
	my ($interface, $file, $param) = @_;

	if ($cachetime == '0') {
		$file = '-';
	} else {
		$file =~ s/\.png$/_dm$darkmode.png/;
	}

	my ($output, $stderr, $status) = graph($interface, $file, $param);
	send_image($file, $output, $stderr, $status);
}

sub show_error
{
	my ($error_msg, $status) = @_;
	$status = "500 Internal Server Error" unless defined $status;
	plain_response($status, $error_msg);
}

sub plain_response
{
	my ($status, $message) = @_;

	if (defined $status) {
		print "Status: $status\n";
	}
	print "Content-Type: text/plain\n\n$message\n";
	exit 0;
}

sub load_interface_list
{
	open(my $iflist, "-|", $vnstati_cmd, "--dbiflist", "1")
		or plain_response("500 Internal Server Error", "Failed to list interfaces.");
	my @lines = <$iflist>;
	close $iflist;
	my $failed = ($? != 0);
	chomp @lines;
	@lines = grep { length $_ } @lines;
	if ($failed or grep { /^Error:/ } @lines) {
		plain_response("500 Internal Server Error", "Failed to list interfaces.");
	}
	return @lines;
}

sub print_empty_database_html
{
	print_html_headers();

	print <<HEADER;
<!DOCTYPE html>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">$metarefresh
<meta name="generator" content="vnstat.cgi $VERSION">$themecookiescript$themetogglescript
<title>Traffic Statistics for $servername</title>
<style>
<!--
$csscommonstyle
$csscolors
$cssbody
$cssthemeswitch
-->
</style>
</head>
HEADER
	print "<body>\n";
	print theme_switch_html();
	print "<br>\n";
	print "Database is empty.\n";
	print "</body>\n</html>\n";
}

sub print_html_headers
{
	print "Content-Type: text/html\n";
	print $themeheaders;
	print "Vary: Cookie\n";
	print "Cache-Control: private, no-cache\n";
	print "\n";
}

sub image_query
{
	my ($query) = @_;

	return "${scriptname}?${query}&dm=${darkmode}";
}

sub theme_switch_html
{
	my $checked = 'false';
	if ($darkmode == '1' or $darkmode == '2') {
		$checked = 'true';
	}

	my $sun = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" aria-hidden="true"><circle cx="12" cy="12" r="4"></circle><path d="M12 2v2M12 20v2M4.93 4.93l1.41 1.41M17.66 17.66l1.41 1.41M2 12h2M20 12h2M4.93 19.07l1.41-1.41M17.66 6.34l1.41-1.41"></path></svg>';
	my $moon = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z"></path></svg>';

	return "<button type=\"button\" id=\"theme-switch\" class=\"theme-switch\" role=\"switch\" aria-checked=\"$checked\" aria-label=\"Dark mode\">"
		. "<span class=\"theme-switch-sun\" aria-hidden=\"true\">$sun</span>"
		. "<span class=\"theme-switch-moon\" aria-hidden=\"true\">$moon</span>"
		. "<span class=\"theme-switch-thumb\" aria-hidden=\"true\"></span>"
		. "</button>\n";
}

sub print_interface_list_html
{
	my @interfaces = @vnStatCGI::interfaces;

	print_html_headers();

	print <<HEADER;
<!DOCTYPE html>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">$metarefresh
<meta name="generator" content="vnstat.cgi $VERSION">$themecookiescript$themetogglescript
<title>Traffic Statistics for $servername</title>
<style>
<!--
$csscommonstyle
$csscolors
$cssbody
$cssthemeswitch
-->
</style>
</head>
HEADER
	print "<body>\n";
	print theme_switch_html();
	print "<br>\n";
	my $interfacesshown = 0;
	my $lineended = 0;
	for my $i (0..$#interfaces) {
		if (length($indexshowninterfaces) > 0 && $interfaces[${i}] !~ /$indexshowninterfaces/) {
			next;
		}
		if (length($indexhiddeninterfaces) > 0 && $interfaces[${i}] =~ /$indexhiddeninterfaces/) {
			next;
		}
		print "<a href=\"${scriptname}?${i}-f\"><img src=\"" . image_query("${i}-$indeximageoutput") . "\" alt=\"$interfaces[${i}]\"></a>";
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

	print_html_headers();

	print <<HEADER;
<!DOCTYPE html>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">$metarefresh
<meta name="generator" content="vnstat.cgi $VERSION">$themecookiescript$themetogglescript
<title>Traffic Statistics for $servername - $interfaces[${interface}]</title>
<style>
<!--
$csscommonstyle
$csscolors
$cssbody
$cssthemeswitch
-->
</style>
</head>
HEADER
	print "<body>\n";
	print theme_switch_html();
	print "<br>\n";
	print "<table>\n<tr><td>\n";
	print "<img src=\"" . image_query("${interface}-s") . "\" alt=\"$interfaces[${interface}] summary\"><br>\n";
	print "<a href=\"${scriptname}?s-${interface}-d-l\"><img src=\"" . image_query("${interface}-d") . "\" alt=\"$interfaces[${interface}] daily\"></a><br>\n";
	print "<a href=\"${scriptname}?s-${interface}-t-l\"><img src=\"" . image_query("${interface}-t") . "\" alt=\"$interfaces[${interface}] top 10\"></a><br>\n";
	print "</td><td>\n";
	print "<a href=\"${scriptname}?s-${interface}-h\"><img src=\"" . image_query("${interface}-hg") . "\" alt=\"$interfaces[${interface}] hourly\"></a><br>\n";
	print "<a href=\"${scriptname}?s-${interface}-5\"><img src=\"" . image_query("${interface}-5g") . "\" alt=\"$interfaces[${interface}] 5 minute\"></a><br>\n";
	print "<a href=\"${scriptname}?s-${interface}-m-l\"><img src=\"" . image_query("${interface}-m") . "\" alt=\"$interfaces[${interface}] monthly\"></a><br>\n";
	print "<a href=\"${scriptname}?s-${interface}-y-l\"><img src=\"" . image_query("${interface}-y") . "\" alt=\"$interfaces[${interface}] yearly\"></a><br>\n";
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
		show_error("ERROR: invalid query", "400 Bad Request");
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
		show_error("ERROR: invalid query type", "400 Bad Request");
	}

	print_html_headers();

	print <<HEADER;
<!DOCTYPE html>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">$metarefresh
<meta name="generator" content="vnstat.cgi $VERSION">$themecookiescript$themetogglescript
<title>$content Traffic Statistics for $servername - $interfaces[${interface}]</title>
<style>
<!--
$csscommonstyle
$csscolors
$cssbody
$cssthemeswitch
-->
</style>
</head>
HEADER
	print "<body>\n";
	print theme_switch_html();
	print "<br>\n";
	print "<table>\n<tr><td>\n";
	print "<img src=\"" . image_query($image) . "\" alt=\"$interfaces[${interface}] ", lc($content), "\">\n";
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
		our @interfaces = load_interface_list();
	}
	chomp @vnStatCGI::interfaces;
	my @interfaces = @vnStatCGI::interfaces;

	if (length($servername) == 0) {
		$servername = `hostname`;
		chomp $servername;
	}

	if ($aligncenter != '0') {
		my $cssscheme = "light";
		if ($darkmode == '1' or $darkmode == '2') {
			$cssscheme = "dark";
		}
		$cssbody = "html { background-color: $bgcolor; color-scheme: $cssscheme; }\nbody { background-color: $bgcolor; text-align: center; display: block; }";
	}

	if ($pagerefresh != '0') {
		$metarefresh = "\n<meta http-equiv=\"refresh\" content=\"$pagerefresh\">";
	}

	if ($cachetime != '0') {
		mkdir $tmp_dir, 0755 unless -d $tmp_dir;
	}

	if (scalar @interfaces == 0) {
		print_empty_database_html();
		return;
	}

	my $query = $ENV{QUERY_STRING};
	my $listlength = '';
	if (defined $query and $query =~ /\S/) {
		if ($query =~ s/&dm=([012])$//) {
			$darkmode = $1;
		}
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
			if ($usecfglengthonsingleimagepages == '0') {
				$listlength = '60';
			}
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_d_l.png", "-d $listlength");
		}
		elsif ($query =~ /^(\d+)-m$/) {
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_m.png", "-m 12");
		}
		elsif ($query =~ /^(\d+)-m-l$/) {
			if ($usecfglengthonsingleimagepages == '0') {
				$listlength = '24';
			}
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_m_l.png", "-m $listlength");
		}
		elsif ($query =~ /^(\d+)-t$/) {
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_t.png", "-t 10");
		}
		elsif ($query =~ /^(\d+)-t-l$/) {
			if ($usecfglengthonsingleimagepages == '0') {
				$listlength = '20';
			}
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_t_l.png", "-t $listlength");
		}
		elsif ($query =~ /^(\d+)-h$/) {
			if ($usecfglengthonsingleimagepages == '0') {
				$listlength = '48';
			}
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_h.png", "-h $listlength");
		}
		elsif ($query =~ /^(\d+)-hg$/) {
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_hg.png", "-hg");
		}
		elsif ($query =~ /^(\d+)-5$/) {
			if ($usecfglengthonsingleimagepages == '0') {
				$listlength = '60';
			}
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_5.png", "-5 $listlength");
		}
		elsif ($query =~ /^(\d+)-5g$/) {
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_5g.png", "-5g");
		}
		elsif ($query =~ /^(\d+)-y$/) {
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_y.png", "-y 5");
		}
		elsif ($query =~ /^(\d+)-y-l$/) {
			if ($usecfglengthonsingleimagepages == '0') {
				$listlength = '0';
			}
			handle_image($interfaces[$1], "$tmp_dir/vnstat_$1_y_l.png", "-y $listlength");
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
			show_error("ERROR: invalid argument", "400 Bad Request");
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
				show_error("ERROR: no such interface: $interface", "404 Not Found");
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
