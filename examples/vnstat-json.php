<?php

/* vnstat-json.php -- example php for vnStat json output */
/* copyright (c) 2015-2026 Teemu Toivola <tst at iki dot fi> */
/* released under the GNU General Public License */


/* location of vnstat binary */
$vnstat_cmd = "/usr/bin/vnstat";

/* individually accessible interfaces with ?interface=N or /interfacename */
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

function plain_response($status, $message)
{
	if ($status !== null) {
		header("Status: ".$status);
	}
	header("Content-Type: text/plain");
	header("Cache-Control: private, no-cache");
	echo $message, "\n";
	exit(0);
}

if (!isset($interfaces) || count($interfaces) == 0) {
	list($list_out, $list_err, $list_status) = vnstat_run(array($vnstat_cmd, "--dbiflist", "1"));
	if (!is_string($list_out)) {
		$list_out = "";
	}
	if ($list_status !== 0) {
		plain_response("500 Internal Server Error", "Failed to list interfaces.");
	}
	$list_out = trim($list_out);
	if ($list_out === "") {
		$interfaces = array();
	} else {
		$names = array();
		foreach (explode("\n", $list_out) as $name) {
			if ($name === "") {
				continue;
			}
			if (strncmp($name, "Error:", 6) === 0) {
				plain_response("500 Internal Server Error", "Failed to list interfaces.");
			}
			$names[] = $name;
		}
		$interfaces = $names;
	}
}

if (count($interfaces) == 0) {
	plain_response(null, "Database is empty.");
}

$iface = "";
$selected = false;
if (isset($_SERVER['PATH_INFO']) && is_string($_SERVER['PATH_INFO'])) {
	$fields = explode('/', $_SERVER['PATH_INFO']);
	$interface = $fields[count($fields) - 1];
	if ($interface !== '') {
		$found = false;
		foreach ($interfaces as $name) {
			if ($name === $interface) {
				$iface = $interface;
				$selected = true;
				$found = true;
				break;
			}
		}
		if (!$found) {
			plain_response("404 Not Found", "Unknown interface.");
		}
	}
}
if (!$selected && isset($_GET['interface'])) {
	$raw = $_GET['interface'];
	if (!is_string($raw) || !ctype_digit($raw) || (int)$raw >= count($interfaces)) {
		plain_response("400 Bad Request", "Invalid interface selector.");
	}
	$selected = true;
	$iface = $interfaces[(int)$raw];
}

$command = array($vnstat_cmd, "--json");
if ($selected) {
	array_push($command, "-i", $iface);
}

list($json, $json_err, $json_status) = vnstat_run($command);
if ($json_status !== 0) {
	plain_response("500 Internal Server Error", "Failed to read vnStat data.");
}
json_decode($json);
if (json_last_error() !== JSON_ERROR_NONE) {
	plain_response("500 Internal Server Error", "Invalid command output.");
}
header("Content-Type: application/json");
header("Cache-Control: private, no-cache");
echo $json;
?>
