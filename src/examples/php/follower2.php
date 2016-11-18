<?php
	$data[0][0] = "first_name";
	$data[0][1] = "last_name";
	$data[0][2] = "color";
	$data[0][3] = "group";
	$data[0][4] = "breakfast";

	$data[1][0] = "Bob";
	$data[1][1] = "Doan";
	$data[1][2] = "blue";
	$data[1][3] = "1";
	$data[1][4] = "Green Eggs And Spam I Am I Am";

	$data[2][0] = "Terry";
	$data[2][1] = "Doan";
	$data[2][2] = "blue";
	$data[2][3] = "1";
	$data[2][4] = "Ham and Eggs";

	$data[3][0] = "Eric";
	$data[3][1] = "Buruschkin";
	$data[3][2] = "green";
	$data[3][3] = "1";
	$data[3][4] = "Green Eggs And Spam I Am I Am";

	$data[4][0] = "Mike";
	$data[4][1] = "Roth";
	$data[4][2] = "yellow";
	$data[4][3] = "2";
	$data[4][4] = "Green Eggs And Spam I Am I Am";

	$data[5][0] = "Bob";
	$data[5][1] = "Kratz";
	$data[5][2] = "pink";
	$data[5][3] = "2";
	$data[5][4] = "Green Eggs And Spam I Am I Am";

	$data[6][0] = "Steve";
	$data[6][1] = "Tilden";
	$data[6][2] = "purple";
	$data[6][3] = "2";
	$data[6][4] = "Dude";


	$more_data[0][0] = "last_name";
	$more_data[0][1] = "initials";

	$more_data[1][0] = "Doan";
	$more_data[1][1] = "WRD";

	$more_data[2][0] = "Doan";
	$more_data[2][1] = "TD";

	//$more_data[3][0] = "Buruschkin";
	//$more_data[3][1] = "ERB";

	$rlib =	rlib_init();
	rlib_add_datasource_array($rlib, "local_array");
	rlib_add_query_as($rlib, "local_array", "data", "data");
	rlib_add_query_as($rlib, "local_array", "more_data", "more_data");
	rlib_add_resultset_follower($rlib, "data", "more_data");
	rlib_add_report($rlib, "follower.xml");
	rlib_set_output_format_from_text($rlib, "txt");
	rlib_execute($rlib);
	header(rlib_get_content_type($rlib));
	rlib_spool($rlib);
	rlib_free($rlib);
?>
