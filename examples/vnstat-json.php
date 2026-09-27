<?php

/* vnstat-json.php -- example php for vnStat json output */
/* copyright (c) 2015-2026 Teemu Toivola <tst at iki dot fi> */
/* released under the GNU General Public License */


/* location of vnstat binary */
$vnstat_cmd = "/usr/bin/vnstat";

/* individually accessible interfaces with ?interface=N */
/* for static list, uncomment and update the list */
//$interfaces = array("eth0", "eth1");

/* no editing should be needed below this line */

function vnstat_run($command)
{
	$pipes = array();
	$proc = proc_open($command, array(
		1 => array("pipe", "w"),
		2 => array("pipe", "w")
	), $pipes);
	if (!is_resource($proc)) {
		return array("", "", -1);
	}
	$stdout = stream_get_contents($pipes[1]);
	$stderr = stream_get_contents($pipes[2]);
	fclose($pipes[1]);
	fclose($pipes[2]);
	$status = proc_close($proc);
	if ($stdout === false) {
		$stdout = "";
	}
	if ($stderr === false) {
		$stderr = "";
	}
	if (strlen($stderr) > 0) {
		fwrite(STDERR, $stderr);
	}
	return array($stdout, $stderr, $status);
}

if (!isset($interfaces) || count($interfaces) == 0) {
	list($list_out, $list_err, $list_status) = vnstat_run(array($vnstat_cmd, "--dbiflist", "1"));
	$interfaces = explode("\n", trim($list_out));
}

$iface = "";
$selected = false;
$getiface = "";
if (isset($_GET['interface']) && ctype_digit($_GET['interface'])) {
	$getiface = $_GET['interface'];
}

if (strlen($getiface) > 0 && $getiface >= 0 && $getiface < count($interfaces)) {
	$selected = true;
	$iface = $interfaces[$getiface];
}

$command = array($vnstat_cmd, "--json");
if ($selected) {
	array_push($command, "-i", $iface);
}

header("Content-Type: application/json");
list($json, $json_err, $json_status) = vnstat_run($command);
echo $json;
?>
