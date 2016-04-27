<?php
	$hostname = "localhost";
	$username = "rlib";
	$password = "rlib";
	$database = "rlib";	

	$rlib =	rlib_init();
	rlib_add_datasource_mysql($rlib, "local_mysql", $hostname, $username, $password, $database);
	rlib_add_query_as($rlib, "local_mysql", "select * FROM products", "products");
	rlib_add_report($rlib, "products.xml");
	rlib_set_output_format_from_text($rlib, "pdf");
	rlib_execute($rlib);
	header(rlib_get_content_type($rlib));
	rlib_spool($rlib);
	rlib_free($rlib);
?>
