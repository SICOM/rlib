<?php
	$data[0][0] = "first_name";
	$data[0][1] = "last_name";
	$data[0][2] = "color";
	$data[0][3] = "group";

	$data[1][0] = "Bob";
	$data[1][1] = "Doan";
	$data[1][2] = "blue";
	$data[1][3] = "1";

	$data[2][0] = "Eric";
	$data[2][1] = "Eburuschkin";
	$data[2][2] = "green";
	$data[2][3] = "1";

	$data[3][0] = "Mike";
	$data[3][1] = "Roth";
	$data[3][2] = "yellow";
	$data[3][3] = "2";

	$data[4][0] = "Bob";
	$data[4][1] = "Kratz";
	$data[4][2] = "pink";
	$data[4][3] = "2";

	$data[5][0] = "Steve";
	$data[5][1] = "Tilden";
	$data[5][2] = "purple";
	$data[5][3] = "2";

	$moredata[0][0] = "name";
	$moredata[1][0] = "1";
	$moredata[2][0] = "2";
	$moredata[3][0] = "3";
	$moredata[4][0] = "4";
	$moredata[5][0] = "5";
	$moredata[6][0] = "6";
	$moredata[7][0] = "7";
	$moredata[8][0] = "8";
	$moredata[9][0] = "9";
	$moredata[10][0] = "10";
	$moredata[11][0] = "11";
	$moredata[12][0] = "12";
	$moredata[13][0] = "13";
	$moredata[14][0] = "14";
	$moredata[15][0] = "15";
	$moredata[16][0] = "16";
	$moredata[17][0] = "17";
	$moredata[18][0] = "18";
	$moredata[19][0] = "19";
	$moredata[20][0] = "20";
	$moredata[21][0] = "21";
	$moredata[22][0] = "22";
	$moredata[23][0] = "23";
	$moredata[24][0] = "24";
	$moredata[25][0] = "25";
	$moredata[26][0] = "26";
	$moredata[27][0] = "27";
	$moredata[28][0] = "28";
	$moredata[29][0] = "29";
	$moredata[30][0] = "30";
	$moredata[31][0] = "31";
	$moredata[32][0] = "32";
	$moredata[33][0] = "33";
	$moredata[34][0] = "34";
	$moredata[35][0] = "35";
	$moredata[36][0] = "36";
	$moredata[37][0] = "37";
	$moredata[38][0] = "38";
	$moredata[39][0] = "39";
	$moredata[40][0] = "40";
	$moredata[41][0] = "41";
	$moredata[42][0] = "42";
	$moredata[43][0] = "43";
	$moredata[44][0] = "44";
	$moredata[45][0] = "45";
	$moredata[46][0] = "46";
	$moredata[47][0] = "47";
	$moredata[48][0] = "48";
	$moredata[49][0] = "49";
	$moredata[50][0] = "50";
	$moredata[51][0] = "51";
	$moredata[52][0] = "52";
	$moredata[53][0] = "53";
	$moredata[54][0] = "54";
	$moredata[55][0] = "55";
	$moredata[56][0] = "56";
	$moredata[57][0] = "57";
	$moredata[58][0] = "58";
	$moredata[59][0] = "59";


	$rlib =	rlib_init();
	rlib_add_datasource_array($rlib, "local_array");
	rlib_add_query_as($rlib, "local_array", "data", "data");
	rlib_add_query_as($rlib, "local_array", "moredata", "moredata");
	rlib_add_report($rlib, "flow_part.xml");
	rlib_set_output_format_from_text($rlib, "pdf");
	rlib_execute($rlib);
	header(rlib_get_content_type($rlib));
	rlib_spool($rlib);
	rlib_free($rlib);
?>
