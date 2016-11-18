<?php

$data[0][0] = "date";
$data[0][1] = "number";

$data[1][0] = "2016-01-01 10:27:37";
$data[1][1] = "100.0";

$rlib =	rlib_init();
rlib_version();
rlib_add_datasource_array($rlib, "local_array");
rlib_add_query_as($rlib, "local_array", "data", "data");

$output_format = 'txt';

if (isset($argv[1]))
	$output_format = $argv[1];

rlib_add_report($rlib, "formatstring.xml");
rlib_set_output_format_from_text($rlib, $output_format);
rlib_set_locale($rlib, "en_US");
rlib_execute($rlib);
rlib_spool($rlib);
rlib_free($rlib);
	
?>
