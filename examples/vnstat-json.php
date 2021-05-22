<?php

/* vnstat-json.php -- example php for vnStat json output */
/* copyright (c) 2015-2021 Teemu Toivola <tst at iki dot fi> */
/* released under the GNU General Public License */


/* location of vnstat binary */
$vnstat_cmd = "/usr/bin/vnstat";

/* individually accessible interfaces with ?interface=N */
/* for static list, uncomment first line below, update the list and comment out second line */
//$interfaces = array("eth0", "eth1");
$interfaces = explode("\n", trim(shell_exec("$vnstat_cmd --dbiflist 1")));

/* no editing should be needed below this line */

$iface = "";
$getiface = "";
if (isset($_GET['interface']) && ctype_digit($_GET['interface'])) {
	$getiface = $_GET['interface'];
}

if (strlen($getiface) > 0 && $getiface >= 0 && $getiface < count($interfaces)) {
	$iface = " -i ".$interfaces[$getiface];
}

header("Content-Type: application/json");
passthru($vnstat_cmd." --json".$iface);
?>
