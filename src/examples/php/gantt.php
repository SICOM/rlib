<?php
	$header_data[0][0] = "name";

	for ($i = 1; $i <= 24; $i++)
		$header_data[$i][0] = $i;
  
	$data[0][0] = "row_field";
	$data[0][1] = "bar_start";
	$data[0][2] = "bar_stop";
	$data[0][3] = "label_field";
	$data[0][4] = "bar_label";
	$data[0][5] = "bar_color";
	$data[0][6] = "bar_label_color";

	$data[1][0] = "1";
	$data[1][1] = "3";
	$data[1][2] = "12";
	$data[1][3] = "Task A";
	$data[1][4] = "A";
	$data[1][5] = "blue";
	$data[1][6] = "white";

	$data[2][0] = "2";
	$data[2][1] = "6";
	$data[2][2] = "12";
	$data[2][3] = "Task B";
	$data[2][4] = "B";
	$data[2][5] = "blue";
	$data[2][6] = "white";

	$data[3][0] = "3";
	$data[3][1] = "9";
	$data[3][2] = "12";
	$data[3][3] = "Task C";
	$data[3][4] = "C";
	$data[3][5] = "blue";
	$data[3][6] = "white";

	$data[4][0] = "4";
	$data[4][1] = "11";
	$data[4][2] = "12";
	$data[4][3] = "Task D";
	$data[4][4] = "D";
	$data[4][5] = "blue";
	$data[4][6] = "white";
///
	$data[5][0] = "1";
	$data[5][1] = "13";
	$data[5][2] = "15";
	$data[5][3] = "Task A";
	$data[5][4] = "A'";
	$data[5][5] = "red";
	$data[5][6] = "white";

	$data[6][0] = "2";
	$data[6][1] = "13";
	$data[6][2] = "18";
	$data[6][3] = "Task B";
	$data[6][4] = "B'";
	$data[6][5] = "red";
	$data[6][6] = "white";

	$data[7][0] = "3";
	$data[7][1] = "13";
	$data[7][2] = "22";
	$data[7][3] = "Task C";
	$data[7][4] = "C'";
	$data[7][5] = "red";
	$data[7][6] = "white";

	$data[8][0] = "4";
	$data[8][1] = "13";
	$data[8][2] = "23";
	$data[8][3] = "Task D";
	$data[8][4] = "D'";
	$data[8][5] = "red";
	$data[8][6] = "white";

	$rlib =	rlib_init();
	rlib_add_datasource_array($rlib, "local_array");
	rlib_add_query_as($rlib, "local_array", "header_data", "header_data");
	rlib_add_query_as($rlib, "local_array", "data", "data");
	rlib_set_output_parameter($rlib, "html_image_directory", "/tmp");
	rlib_set_output_parameter($rlib, "trim_links", "1");
	rlib_add_report($rlib, "gantt.xml");
	rlib_set_output_format_from_text($rlib, "html");
	rlib_execute($rlib);
	rlib_spool($rlib);
	rlib_free($rlib);
?>
