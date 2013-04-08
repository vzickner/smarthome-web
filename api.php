<?php
$device = "/dev/ttyAMA0";

function execute($command) {
	return exec($command);
}

if (isset($_GET['setAerationLevel']) || isset($_GET['getAerationLevel'])) {
	if (isset($_GET['setAerationLevel']) && in_array($_GET['setAerationLevel'], range(0,3))) {
		$aerationLevel = -1;
		for ($i=0;$i<10 && $aerationLevel != $_GET['setAerationLevel']; $i++) {
			execute("./smarthome/smarthome -t ".$device." -a a -w ".((int)$_GET['setAerationLevel']));
			$aerationLevel = execute("./smarthome/smarthome -t ".$device." -a a -r");
		}
	} else {
		$aerationLevel = execute("./smarthome/smarthome -t ".$device." -a a -r");
	}

	echo json_encode(array('aerationLevel' => $aerationLevel));
}
?>
