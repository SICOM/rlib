/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 3.0.8
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */


public class rlib {
  public static SWIGTYPE_p_rlib rlib_init() {
    long cPtr = rlibJNI.rlib_init();
    return (cPtr == 0) ? null : new SWIGTYPE_p_rlib(cPtr, false);
  }

  public static SWIGTYPE_p_gboolean rlib_add_datasource_mysql(SWIGTYPE_p_rlib r, String input_name, String database_host, String database_user, String database_password, String database_database) {
    return new SWIGTYPE_p_gboolean(rlibJNI.rlib_add_datasource_mysql(SWIGTYPE_p_rlib.getCPtr(r), input_name, database_host, database_user, database_password, database_database), true);
  }

  public static SWIGTYPE_p_gboolean rlib_add_datasource_postgres(SWIGTYPE_p_rlib r, String input_name, String conn) {
    return new SWIGTYPE_p_gboolean(rlibJNI.rlib_add_datasource_postgres(SWIGTYPE_p_rlib.getCPtr(r), input_name, conn), true);
  }

  public static SWIGTYPE_p_gboolean rlib_add_datasource_odbc(SWIGTYPE_p_rlib r, String input_name, String source, String user, String password) {
    return new SWIGTYPE_p_gboolean(rlibJNI.rlib_add_datasource_odbc(SWIGTYPE_p_rlib.getCPtr(r), input_name, source, user, password), true);
  }

  public static SWIGTYPE_p_gboolean rlib_add_datasource_xml(SWIGTYPE_p_rlib r, String input_name) {
    return new SWIGTYPE_p_gboolean(rlibJNI.rlib_add_datasource_xml(SWIGTYPE_p_rlib.getCPtr(r), input_name), true);
  }

  public static SWIGTYPE_p_gboolean rlib_add_datasource_csv(SWIGTYPE_p_rlib r, String input_name) {
    return new SWIGTYPE_p_gboolean(rlibJNI.rlib_add_datasource_csv(SWIGTYPE_p_rlib.getCPtr(r), input_name), true);
  }

  public static SWIGTYPE_p_gint rlib_add_query_as(SWIGTYPE_p_rlib r, String input_source, String sql, String name) {
    return new SWIGTYPE_p_gint(rlibJNI.rlib_add_query_as(SWIGTYPE_p_rlib.getCPtr(r), input_source, sql, name), true);
  }

  public static SWIGTYPE_p_gboolean rlib_add_search_path(SWIGTYPE_p_rlib r, String path) {
    return new SWIGTYPE_p_gboolean(rlibJNI.rlib_add_search_path(SWIGTYPE_p_rlib.getCPtr(r), path), true);
  }

  public static int rlib_add_report(SWIGTYPE_p_rlib r, String name) {
    return rlibJNI.rlib_add_report(SWIGTYPE_p_rlib.getCPtr(r), name);
  }

  public static int rlib_add_report_from_buffer(SWIGTYPE_p_rlib r, String buffer) {
    return rlibJNI.rlib_add_report_from_buffer(SWIGTYPE_p_rlib.getCPtr(r), buffer);
  }

  public static SWIGTYPE_p_gboolean rlib_execute(SWIGTYPE_p_rlib r) {
    return new SWIGTYPE_p_gboolean(rlibJNI.rlib_execute(SWIGTYPE_p_rlib.getCPtr(r)), true);
  }

  public static String rlib_get_content_type_as_text(SWIGTYPE_p_rlib r) {
    return rlibJNI.rlib_get_content_type_as_text(SWIGTYPE_p_rlib.getCPtr(r));
  }

  public static SWIGTYPE_p_gboolean rlib_spool(SWIGTYPE_p_rlib r) {
    return new SWIGTYPE_p_gboolean(rlibJNI.rlib_spool(SWIGTYPE_p_rlib.getCPtr(r)), true);
  }

  public static void rlib_set_output_format(SWIGTYPE_p_rlib r, int format) {
    rlibJNI.rlib_set_output_format(SWIGTYPE_p_rlib.getCPtr(r), format);
  }

  public static void rlib_set_output_format_from_text(SWIGTYPE_p_rlib r, String name) {
    rlibJNI.rlib_set_output_format_from_text(SWIGTYPE_p_rlib.getCPtr(r), name);
  }

  public static SWIGTYPE_p_gboolean rlib_add_resultset_follower_n_to_1(SWIGTYPE_p_rlib r, String leader, String leader_field, String follower, String follower_field) {
    return new SWIGTYPE_p_gboolean(rlibJNI.rlib_add_resultset_follower_n_to_1(SWIGTYPE_p_rlib.getCPtr(r), leader, leader_field, follower, follower_field), true);
  }

  public static SWIGTYPE_p_gboolean rlib_add_resultset_follower(SWIGTYPE_p_rlib r, String leader, String follower) {
    return new SWIGTYPE_p_gboolean(rlibJNI.rlib_add_resultset_follower(SWIGTYPE_p_rlib.getCPtr(r), leader, follower), true);
  }

  public static SWIGTYPE_p_gchar rlib_get_output(SWIGTYPE_p_rlib r) {
    long cPtr = rlibJNI.rlib_get_output(SWIGTYPE_p_rlib.getCPtr(r));
    return (cPtr == 0) ? null : new SWIGTYPE_p_gchar(cPtr, false);
  }

  public static SWIGTYPE_p_gsize rlib_get_output_length(SWIGTYPE_p_rlib r) {
    return new SWIGTYPE_p_gsize(rlibJNI.rlib_get_output_length(SWIGTYPE_p_rlib.getCPtr(r)), true);
  }

  public static SWIGTYPE_p_gboolean rlib_signal_connect(SWIGTYPE_p_rlib r, int signal_number, SWIGTYPE_p_f_p_rlib_p_void__int signal_function, SWIGTYPE_p_void data) {
    return new SWIGTYPE_p_gboolean(rlibJNI.rlib_signal_connect(SWIGTYPE_p_rlib.getCPtr(r), signal_number, SWIGTYPE_p_f_p_rlib_p_void__int.getCPtr(signal_function), SWIGTYPE_p_void.getCPtr(data)), true);
  }

  public static SWIGTYPE_p_gboolean rlib_signal_connect_string(SWIGTYPE_p_rlib r, String signal_name, SWIGTYPE_p_f_p_rlib_p_void__int signal_function, SWIGTYPE_p_void data) {
    return new SWIGTYPE_p_gboolean(rlibJNI.rlib_signal_connect_string(SWIGTYPE_p_rlib.getCPtr(r), signal_name, SWIGTYPE_p_f_p_rlib_p_void__int.getCPtr(signal_function), SWIGTYPE_p_void.getCPtr(data)), true);
  }

  public static SWIGTYPE_p_gboolean rlib_query_refresh(SWIGTYPE_p_rlib r) {
    return new SWIGTYPE_p_gboolean(rlibJNI.rlib_query_refresh(SWIGTYPE_p_rlib.getCPtr(r)), true);
  }

  public static SWIGTYPE_p_gboolean rlib_add_parameter(SWIGTYPE_p_rlib r, String name, String value) {
    return new SWIGTYPE_p_gboolean(rlibJNI.rlib_add_parameter(SWIGTYPE_p_rlib.getCPtr(r), name, value), true);
  }

  public static void rlib_set_locale(SWIGTYPE_p_rlib r, String locale) {
    rlibJNI.rlib_set_locale(SWIGTYPE_p_rlib.getCPtr(r), locale);
  }

  public static SWIGTYPE_p_gchar rlib_bindtextdomain(SWIGTYPE_p_rlib r, String domainname, String dirname) {
    long cPtr = rlibJNI.rlib_bindtextdomain(SWIGTYPE_p_rlib.getCPtr(r), domainname, dirname);
    return (cPtr == 0) ? null : new SWIGTYPE_p_gchar(cPtr, false);
  }

  public static void rlib_set_radix_character(SWIGTYPE_p_rlib r, SWIGTYPE_p_gchar radix_character) {
    rlibJNI.rlib_set_radix_character(SWIGTYPE_p_rlib.getCPtr(r), SWIGTYPE_p_gchar.getCPtr(radix_character));
  }

  public static void rlib_set_output_parameter(SWIGTYPE_p_rlib r, String parameter, String value) {
    rlibJNI.rlib_set_output_parameter(SWIGTYPE_p_rlib.getCPtr(r), parameter, value);
  }

  public static void rlib_set_output_encoding(SWIGTYPE_p_rlib r, String encoding) {
    rlibJNI.rlib_set_output_encoding(SWIGTYPE_p_rlib.getCPtr(r), encoding);
  }

  public static SWIGTYPE_p_gboolean rlib_set_datasource_encoding(SWIGTYPE_p_rlib r, String input_name, String encoding) {
    return new SWIGTYPE_p_gboolean(rlibJNI.rlib_set_datasource_encoding(SWIGTYPE_p_rlib.getCPtr(r), input_name, encoding), true);
  }

  public static void rlib_free(SWIGTYPE_p_rlib r) {
    rlibJNI.rlib_free(SWIGTYPE_p_rlib.getCPtr(r));
  }

  public static String rlib_version() {
    return rlibJNI.rlib_version();
  }

  public static SWIGTYPE_p_gboolean rlib_graph_add_bg_region(SWIGTYPE_p_rlib r, String graph_name, String region_label, String color, double start, double end) {
    return new SWIGTYPE_p_gboolean(rlibJNI.rlib_graph_add_bg_region(SWIGTYPE_p_rlib.getCPtr(r), graph_name, region_label, color, start, end), true);
  }

  public static SWIGTYPE_p_gboolean rlib_graph_clear_bg_region(SWIGTYPE_p_rlib r, String graph_name) {
    return new SWIGTYPE_p_gboolean(rlibJNI.rlib_graph_clear_bg_region(SWIGTYPE_p_rlib.getCPtr(r), graph_name), true);
  }

  public static SWIGTYPE_p_gboolean rlib_graph_set_x_minor_tick(SWIGTYPE_p_rlib r, String graph_name, String x_value) {
    return new SWIGTYPE_p_gboolean(rlibJNI.rlib_graph_set_x_minor_tick(SWIGTYPE_p_rlib.getCPtr(r), graph_name, x_value), true);
  }

  public static SWIGTYPE_p_gboolean rlib_graph_set_x_minor_tick_by_location(SWIGTYPE_p_rlib r, String graph_name, int location) {
    return new SWIGTYPE_p_gboolean(rlibJNI.rlib_graph_set_x_minor_tick_by_location(SWIGTYPE_p_rlib.getCPtr(r), graph_name, location), true);
  }

  public static SWIGTYPE_p_gdouble rlib_graph(SWIGTYPE_p_rlib r, SWIGTYPE_p_rlib_part part, SWIGTYPE_p_rlib_report report, double left_margin_offset, SWIGTYPE_p_double top_margin_offset) {
    return new SWIGTYPE_p_gdouble(rlibJNI.rlib_graph(SWIGTYPE_p_rlib.getCPtr(r), SWIGTYPE_p_rlib_part.getCPtr(part), SWIGTYPE_p_rlib_report.getCPtr(report), left_margin_offset, SWIGTYPE_p_double.getCPtr(top_margin_offset)), true);
  }

  public static SWIGTYPE_p_gboolean rlib_parse(SWIGTYPE_p_rlib r) {
    return new SWIGTYPE_p_gboolean(rlibJNI.rlib_parse(SWIGTYPE_p_rlib.getCPtr(r)), true);
  }

  public static void rlib_set_query_cache_size(SWIGTYPE_p_rlib r, int cache_size) {
    rlibJNI.rlib_set_query_cache_size(SWIGTYPE_p_rlib.getCPtr(r), cache_size);
  }

}
