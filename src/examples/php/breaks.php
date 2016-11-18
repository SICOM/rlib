<?php
$data[0] = array('company', 'region', 'district', 'store', 'sales');

$data[1] = array('Foo, Inc.', 'Region A','District X', 'Store #1', 1500.00);
$data[2] = array('Foo, Inc.', 'Region A','District X', 'Store #2', 1300.00);
$data[3] = array('Foo, Inc.', 'Region A','District Y', 'Store #3', 2000.00);
$data[4] = array('Foo, Inc.', 'Region A','District Y', 'Store #4', 1800.00);
$data[5] = array('Foo, Inc.', 'Region B','District M', 'Store #11', 2500.00);
$data[6] = array('Foo, Inc.', 'Region B','District M', 'Store #12', 2300.00);
$data[7] = array('Foo, Inc.', 'Region B','District N', 'Store #13', 3000.00);
$data[8] = array('Foo, Inc.', 'Region B','District N', 'Store #14', 2800.00);

$rlib =	rlib_init();
rlib_version();
rlib_add_datasource_array($rlib, "local_array");
rlib_add_query_as($rlib, "local_array", "data", "data");

$output_format = 'txt';

if (isset($argv[1]))
	$output_format = $argv[1];

rlib_add_report($rlib, "breaks.xml");
rlib_set_output_format_from_text($rlib, $output_format);
rlib_set_output_parameter($rlib, "debugging", "yes");
rlib_set_output_parameter($rlib, "only_quote_strings", "yes");
rlib_set_locale($rlib, "en_US");
rlib_execute($rlib);
// header(rlib_get_content_type($rlib));
rlib_spool($rlib);
rlib_free($rlib);
	
?>
