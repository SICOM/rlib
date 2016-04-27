<?php
	$hostname = "localhost";
	$username = "rlib";
	$password = "rlib";
	$database = "rlib";	

	$rlib =	rlib_init();
	rlib_add_datasource_odbc($rlib, "local_mysql", "test", "user", "password");
	rlib_add_query_as($rlib, "local_mysql", "select * FROM plu", "products");
	rlib_add_report($rlib, "products.xml");
	rlib_set_output_format_from_text($rlib, "txt");
	rlib_execute($rlib);
	header(rlib_get_content_type($rlib));
	rlib_spool($rlib);
	rlib_free($rlib);
?>
