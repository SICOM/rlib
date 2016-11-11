/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 3.0.8
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */


public class rlibJNI {
  public final static native long rlib_init();
  public final static native long rlib_add_datasource_mysql(long jarg1, String jarg2, String jarg3, String jarg4, String jarg5, String jarg6);
  public final static native long rlib_add_datasource_postgres(long jarg1, String jarg2, String jarg3);
  public final static native long rlib_add_datasource_odbc(long jarg1, String jarg2, String jarg3, String jarg4, String jarg5);
  public final static native long rlib_add_datasource_xml(long jarg1, String jarg2);
  public final static native long rlib_add_datasource_csv(long jarg1, String jarg2);
  public final static native long rlib_add_query_as(long jarg1, String jarg2, String jarg3, String jarg4);
  public final static native long rlib_add_search_path(long jarg1, String jarg2);
  public final static native int rlib_add_report(long jarg1, String jarg2);
  public final static native int rlib_add_report_from_buffer(long jarg1, String jarg2);
  public final static native long rlib_execute(long jarg1);
  public final static native String rlib_get_content_type_as_text(long jarg1);
  public final static native long rlib_spool(long jarg1);
  public final static native void rlib_set_output_format(long jarg1, int jarg2);
  public final static native void rlib_set_output_format_from_text(long jarg1, String jarg2);
  public final static native long rlib_add_resultset_follower_n_to_1(long jarg1, String jarg2, String jarg3, String jarg4, String jarg5);
  public final static native long rlib_add_resultset_follower(long jarg1, String jarg2, String jarg3);
  public final static native long rlib_get_output(long jarg1);
  public final static native long rlib_get_output_length(long jarg1);
  public final static native long rlib_signal_connect(long jarg1, int jarg2, long jarg3, long jarg4);
  public final static native long rlib_signal_connect_string(long jarg1, String jarg2, long jarg3, long jarg4);
  public final static native long rlib_query_refresh(long jarg1);
  public final static native long rlib_add_parameter(long jarg1, String jarg2, String jarg3);
  public final static native void rlib_set_locale(long jarg1, String jarg2);
  public final static native long rlib_bindtextdomain(long jarg1, String jarg2, String jarg3);
  public final static native void rlib_set_radix_character(long jarg1, long jarg2);
  public final static native void rlib_set_output_parameter(long jarg1, String jarg2, String jarg3);
  public final static native void rlib_set_output_encoding(long jarg1, String jarg2);
  public final static native long rlib_set_datasource_encoding(long jarg1, String jarg2, String jarg3);
  public final static native void rlib_free(long jarg1);
  public final static native String rlib_version();
  public final static native long rlib_graph_add_bg_region(long jarg1, String jarg2, String jarg3, String jarg4, double jarg5, double jarg6);
  public final static native long rlib_graph_clear_bg_region(long jarg1, String jarg2);
  public final static native long rlib_graph_set_x_minor_tick(long jarg1, String jarg2, String jarg3);
  public final static native long rlib_graph_set_x_minor_tick_by_location(long jarg1, String jarg2, int jarg3);
  public final static native long rlib_graph(long jarg1, long jarg2, long jarg3, double jarg4, long jarg5);
  public final static native long rlib_parse(long jarg1);
  public final static native void rlib_set_query_cache_size(long jarg1, int jarg2);
}
