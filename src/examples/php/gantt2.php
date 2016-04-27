<?php
	$header_data[0][0] = "name";

	$header_data[1][0] = "06:00";
	$header_data[2][0] = "07:00";
	$header_data[3][0] = "08:00";
	$header_data[4][0] = "09:00";
	$header_data[5][0] = "10:00";
	$header_data[6][0] = "11:00";
	$header_data[7][0] = "12:00";
	$header_data[8][0] = "01:00";
	$header_data[9][0] = "02:00";
	$header_data[10][0] = "03:00";
	$header_data[11][0] = "04:00";
	$header_data[12][0] = "05:00";
	$header_data[13][0] = "06:00";
  
	$data[0][0] = "row_field";
	$data[0][1] = "bar_start";
	$data[0][2] = "bar_stop";
	$data[0][3] = "label_field";
	$data[0][4] = "bar_label";
	$data[0][5] = "bar_color";
	$data[0][6] = "bar_label_color";

	$data[1][0] = "1";
	$data[1][1] = "1";
	$data[1][2] = "3";
	$data[1][3] = "Jill";
	$data[1][4] = "Kitchen";
	$data[1][5] = "blue";
	$data[1][6] = "white";

	$data[2][0] = "2";
	$data[2][1] = "5";
	$data[2][2] = "11";
	$data[2][3] = "John";
	$data[2][4] = "Salad Station";
	$data[2][5] = "lime";
	$data[2][6] = "black";

	$data[3][0] = "3";
	$data[3][1] = "3";
	$data[3][2] = "7";
	$data[3][3] = "Beth";
	$data[3][4] = "Cashier";
	$data[3][5] = "green";
	$data[3][6] = "white";

	$data[4][0] = "4";
	$data[4][1] = "4";
	$data[4][2] = "8";
	$data[4][3] = "Mike";
	$data[4][4] = "Drinks";
	$data[4][5] = "aqua";
	$data[4][6] = "black";

	$data[5][0] = "5";
	$data[5][1] = "8";
	$data[5][2] = "13";
	$data[5][3] = "Rich";
	$data[5][4] = "Cashier";
	$data[5][5] = "green";
	$data[5][6] = "white";

	$data[6][0] = "6";
	$data[6][1] = "1";
	$data[6][2] = "7";
	$data[6][3] = "Melissa";
	$data[6][4] = "Manager";
	$data[6][5] = "black";
	$data[6][6] = "white";

	$data[7][0] = "7";
	$data[7][1] = "3";
	$data[7][2] = "9";
	$data[7][3] = "Sylvia";
	$data[7][4] = "Burger Station";
	$data[7][5] = "maroon";
	$data[7][6] = "white";

	$data[8][0] = "8";
	$data[8][1] = "7";
	$data[8][2] = "12";
	$data[8][3] = "Tom";
	$data[8][4] = "Dessert Station";
	$data[8][5] = "fuchsia";
	$data[8][6] = "white";

	$data[9][0] = "9";
	$data[9][1] = "1";
	$data[9][2] = "10";
	$data[9][3] = "Ruby";
	$data[9][4] = "Cashier";
	$data[9][5] = "green";
	$data[9][6] = "white";

	$data[10][0] = "10";
	$data[10][1] = "3";
	$data[10][2] = "8";
	$data[10][3] = "Ben";
	$data[10][4] = "Salad Station";
	$data[10][5] = "lime";
	$data[10][6] = "black";

	$data[11][0] = "11";
	$data[11][1] = "1";
	$data[11][2] = "6";
	$data[11][3] = "Frank";
	$data[11][4] = "Kitchen";
	$data[11][5] = "blue";
	$data[11][6] = "white";

	$data[12][0] = "12";
	$data[12][1] = "10";
	$data[12][2] = "15";
	$data[12][3] = "Dave";
	$data[12][4] = "Burger Station";
	$data[12][5] = "maroon";
	$data[12][6] = "white";

	$data[13][0] = "1";
	$data[13][1] = "11";
	$data[13][2] = "13";
	$data[13][3] = "Jill";
	$data[13][4] = "Kitchen";
	$data[13][5] = "blue";
	$data[13][6] = "white";

	$rlib =	rlib_init();
	rlib_add_datasource_array($rlib, "local_array");
	rlib_add_query_as($rlib, "local_array", "header_data", "header_data");
	rlib_add_query_as($rlib, "local_array", "data", "data");
	rlib_set_output_parameter($rlib, "html_image_directory", "/tmp");
	rlib_set_output_parameter($rlib, "trim_links", "1");
	rlib_add_report($rlib, "gantt2.xml");
	rlib_set_output_format_from_text($rlib, "html");
	rlib_execute($rlib);
	rlib_spool($rlib);
	rlib_free($rlib);
?>
