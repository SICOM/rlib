<?php
	$header_data[0][0] = "name";

	$colors = Array("black", "silver", "gray", "white", "maroon", "red", "purple", "fuchsia", "green", "lime", "olive", "yellow", "navy", "blue", "teal", "aqua", "bobkratz", "everton");

	for ($i = 1; $i <= 22; $i++) {
		$header_data[$i][0] = $i;
	}
  
	$data[0][0] = "row_field";
	$data[0][1] = "bar_start";
	$data[0][2] = "bar_stop";
	$data[0][3] = "label_field";
	$data[0][4] = "bar_label";
	$data[0][5] = "bar_color";
	$data[0][6] = "bar_label_color";

	for ($i = 1; $i <= 18; $i++) {
		$data[$i][0] = $i;
		$data[$i][1] = $i;
		$data[$i][2] = $i + 4;
		$data[$i][3] = $i;
		$data[$i][4] = $colors[$i - 1];
		$data[$i][5] = $colors[$i - 1];;
		$data[$i][6] = "black";
		if ($colors[$i - 1] == "black" || $colors[$i - 1] == "navy")
			$data[$i][6] = "white";
	}

	$rlib =	rlib_init();
	rlib_add_datasource_array($rlib, "local_array");
	rlib_add_query_as($rlib, "local_array", "header_data", "header_data");
	rlib_add_query_as($rlib, "local_array", "data", "data");
	rlib_set_output_parameter($rlib, "html_image_directory", "/tmp");
	rlib_set_output_parameter($rlib, "trim_links", "1");
	rlib_add_report($rlib, "gantt_colors.xml");
	rlib_set_output_format_from_text($rlib, "html");
	rlib_execute($rlib);
	rlib_spool($rlib);
	rlib_free($rlib);
?>
