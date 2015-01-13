<?php

/* vnstat-json.php -- example php for vnStat json output */
/* copyright (c) 2015 Teemu Toivola <tst at iki dot fi> */
/* released under the GNU General Public License */


/* list of available interfaces, edit as necessary */
$interfaces = array("eth0", "eth1", "ethX");

/* location of vnstat binary */
$vnstat_cmd = "/usr/bin/vnstat";


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
