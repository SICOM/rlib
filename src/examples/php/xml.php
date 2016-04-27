<?php
	$rlib =	rlib_init();
	rlib_version();
	rlib_add_datasource_xml($rlib, "local_xml");
	rlib_add_query_as($rlib, "local_xml", "data.xml", "data");
	rlib_add_report($rlib, "array.xml");
	rlib_set_output_format_from_text($rlib, "pdf");
	rlib_execute($rlib);
	header(rlib_get_content_type($rlib));
	rlib_spool($rlib);
	rlib_free($rlib);
?>
