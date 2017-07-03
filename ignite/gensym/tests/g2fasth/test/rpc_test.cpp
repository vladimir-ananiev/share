#include <stdio.h>
#include "rt-suite.hpp"
#include "g2fasth.hpp"
#include "test_agent.hpp"

#include "libgsi.hpp"
#include "gsi_misc.h"

using namespace g2::fasth;

#define STORE_SIZE               5
#define MAX_STORE_INDEX          STORE_SIZE - 1

gsi_item *item_or_value_store;
long      store_index = 0;
gsi_item *error_object_ptr;

void init()
{
    item_or_value_store = gsi_make_items(STORE_SIZE);
    store_index = 0;
}

void shutdown()
{
    for (store_index=0;store_index<STORE_SIZE;store_index++)
        gsirtl_free_i_or_v_contents(item_or_value_store[store_index]);
    gsi_reclaim_items(item_or_value_store);
}

void rpc_add_float(gsi_item rpc_args[],gsi_int count,call_identifier_type call_index)
{
    double a,b,c;
    a = gsi_flt_of(rpc_args[0]);
    b = gsi_flt_of(rpc_args[1]);
    c = a + b;
    gsi_set_flt(rpc_args[0], c);
    gsi_rpc_return_values(rpc_args, 1, call_index, current_context);
}

void rpc_text(gsi_item rpc_args[],gsi_int count,call_identifier_type call_index)
{
#if defined(GSI_USE_WIDE_STRING_API)
    gsi_set_str(rpc_args[0], L"RPC text value");
#else
    gsi_set_str(rpc_args[0], "RPC text value");
#endif
    gsi_rpc_return_values(rpc_args, 1, call_index, current_context);
}

void rpc_error_mode(gsi_item rpc_args[],gsi_int count,call_identifier_type call_index)
{
    int mode = gsi_log_of(rpc_args[0]);
    g2::fasth::libgsi::getInstance().error_mode(mode > 0);
}

void rpc_action(gsi_item rpc_args[],gsi_int count,call_identifier_type call_index)
{
    g2::fasth::libgsi& gsiobj = g2::fasth::libgsi::getInstance();
    gsi_item* arguments = gsi_make_items(1);
    gsi_set_str(arguments[0], "Synchronous call from G2FASTH");
    
    gsiobj.start_remote("G2-REMOTE", arguments);
    gsi_reclaim_items(arguments);
}

void rpc_asynch(gsi_item rpc_args[],gsi_int count,call_identifier_type call_index)
{
    g2::fasth::libgsi& gsiobj = g2::fasth::libgsi::getInstance();
    gsi_item* arguments = gsi_make_items(1);
    gsi_set_str(arguments[0], "Test");
    
    gsiobj.call_remote("G2-ASYNCH", arguments);
    gsi_reclaim_items(arguments);
}

void exit_thread(void*)
{
    puts("exit_thread(): exiting in 1 sec...");
    tthread::this_thread::sleep_for(tthread::chrono::milliseconds(1000));
    exit(0);
}
void rpc_exit(gsi_item rpc_args[],gsi_int count,call_identifier_type call_index)
{
    puts("rpc_exit()");
    new tthread::thread(exit_thread, NULL);
}

void rpc_test(gsi_item rpc_args[],gsi_int count,call_identifier_type call_index)
{
    string test_code(str_of(rpc_args[0]));

    printf("rpc_test(%s)\n", test_code.c_str());

    RegTestSuite rts(test_code);

    rts.execute();

    string result = rts.get_results()[0].outcome() == test_outcome::pass ? "pass" : "fail";

    gsi_set_str(rpc_args[0], (char*)result.c_str());
    gsi_rpc_return_values(rpc_args, 1, call_index, current_context);
}

void rpc_declare_variable(gsi_item rpc_args[],gsi_int count,call_identifier_type call_index)
{
    string var_name(str_of(rpc_args[0]));
    string var_type(str_of(rpc_args[1]));

    printf("rpc_declare_variable(%s:%s)\n", var_name.c_str(), var_type.c_str());

    libgsi& gsi = libgsi::getInstance();

    if (var_type == "integer")
        gsi.declare_g2_variable<int>(var_name);
    else if (var_type == "float")
        gsi.declare_g2_variable<double>(var_name);
    else if (var_type == "logical")
        gsi.declare_g2_variable<bool>(var_name);
    else if (var_type == "string" || var_type == "text")
        gsi.declare_g2_variable<string>(var_name);

    gsi_rpc_return_values(NULL, 0, call_index, current_context);
}

void asynch_receiver(gsi_item rpc_args[], gsi_int count)
{
    char* value = gsi_str_of(rpc_args[0]);
    std::cout << value << std::endl;
}

void rpc_update_vars(gsi_item rpc_args[],gsi_int count,call_identifier_type call_index)
{
    g2::fasth::libgsi& gsiobj = g2::fasth::libgsi::getInstance();
    gsiobj.update_g2_variable("INTEGER-DAT");
    gsiobj.update_g2_variable("FLOAT64-DAT");
}

int int_handler()
{
    static int x = 0;
    return ++x;
}

double float_handler()
{
    static double d = 0.0;
    return d += 3.14;
}

void receive_item_or_value(gsi_item arg_array[],gsi_int count,call_identifier_type call_index)
{
/*
 *  Display the entire contents of the argument to standard out.
 *  Items arguments are displayed recursively attribute by attribute.
 */
    printf("count = %d, store_index = %d\n",count,store_index);
    printf("-------------------------\n");
    printf("  Printing argument\n");
    printf("-------------------------\n");
    gsirtl_display_item_or_value(arg_array[0],STARTING_LEVEL,STANDARD_OUT);
    printf("------------------------- end of argument.\n");

/*
 *  This program keeps a rotating store of STORE_SIZE items.
 *  Copy the argument into the item_or_value_store which is 
 *  an array of gsi_item's.
 */
    gsirtl_copy_item_or_value(arg_array[0],item_or_value_store[store_index]);

/*
 *  Increment the store index (resetting it if it is maxed) and free
 *  memory for displaced items.  Note that the item structure itself
 *  is not reclaimed but any sub parts (values or attributes) which
 *  take up memory are reclaimed.
 */
    if (store_index == MAX_STORE_INDEX)
      store_index = 0;
    else
      store_index++;
    gsirtl_free_i_or_v_contents(item_or_value_store[store_index]);

} /* end receive_item_or_value */


void receive_item_transfer(gsi_item arg_array[],gsi_int count,call_identifier_type call_index)
{
/*
 *  The contract of this function is a simple superset of 
 *  that for receive_item_or_value(), so it is called here.
 */
    receive_item_or_value(arg_array,count,call_index);

/*
 *  This function is called as opposed to started by G2 and as such
 *  must return even if no arguments are returned.
 */
    gsi_rpc_return_values(NULL_PTR,0,call_index,current_context);

} /* end receive_item_transfer */


void receive_and_return_copy(gsi_item arg_array[],gsi_int count,call_identifier_type call_index)
{
    gsi_int   duplicate_index;
/*
 *  The contract of this function is a simple superset of 
 *  that for receive_item_or_value(), so it is called here.
 */
    receive_item_or_value(arg_array,count,call_index);

/*
 *  Return the copy to G2.  Note, that receive_item_or_value()
 *  increments the store index so we have to look back one to
 *  get at the duplicate that was generated by the function.
 */
    if (store_index == 0)
      duplicate_index = MAX_STORE_INDEX;
    else
      duplicate_index = store_index - 1;
    printf("\n------>\n"); 
    gsi_rpc_return_values(&item_or_value_store[duplicate_index],1,call_index,current_context);
    printf("   <\n");
} /* end receive_and_return_copy */


void receive_request_for_copy(gsi_item arg_array[],gsi_int count,call_identifier_type call_index)
{
    long      i;
    char     *search_name = sym_of(arg_array[0]);
    char     *iv_name;
    gsi_item  item_or_value;

/*
 *  This function, searches the item_or_value_store 
 *  for an item given the item's name.
 */
    for (i=0; i<=MAX_STORE_INDEX; i++) {
      item_or_value = item_or_value_store[i];
      iv_name = name_of(item_or_value);
      if (strcmp(iv_name,search_name) == 0) {
	gsi_rpc_return_values(&item_or_value,1,call_index,current_context);
	return; }
    } /* end for */

    gsi_rpc_return_values(error_object_ptr,1,call_index,current_context);

} /* receive_request_for_copy */


void prepare_test_3227();
void prepare_test_3228();
void prepare_test_3229();
void prepare_test_3230();

int main(int argc, char **argv) {
    g2_options options;
    options.parse_arguments(&argc, argv);
    options.set_signal_handler();

    int port_num = 22060;
    if (argc > 1)
    {
        int tmp = strtol(argv[1], nullptr, 10);
        if (tmp)
          port_num = tmp;
    }

    g2::fasth::libgsi& gsiobj = g2::fasth::libgsi::getInstance();
    gsiobj.continuous(true);
    gsiobj.port(port_num);
    
    gsiobj.declare_g2_function("RPC-ADD-FLOAT", rpc_add_float);
    gsiobj.declare_g2_function("RPC-TEXT", rpc_text);
    gsiobj.declare_g2_function("RPC-ERROR-MODE", rpc_error_mode);
    gsiobj.declare_g2_function("RPC-ACTION", rpc_action);
    gsiobj.declare_g2_function("RPC-ASYNCH", rpc_asynch);
    gsiobj.declare_g2_function("RPC-EXIT", rpc_exit);
    gsiobj.declare_g2_function("RPC-UPDATE-VARS", rpc_update_vars);

    if (argc > 2)
    {
        string test_code = argv[2];

        if (test_code == "3227")
            prepare_test_3227();
        else if (test_code == "3228")
            prepare_test_3228();
        else if (test_code == "3229")
            prepare_test_3229();
        else if (test_code == "3230")
            prepare_test_3230();
    }
    else
    {
        gsiobj.declare_g2_variable<int>("INTEGER-DAT", int_handler);
        gsiobj.declare_g2_variable<double>("FLOAT64-DAT", float_handler);
    }

    gsiobj.declare_g2_function("RPC-ADD-FLOAT", rpc_add_float);
    gsiobj.declare_g2_function("RPC-TEXT", rpc_text);

    gsiobj.declare_g2_function("RECEIVE-AND-DISPLAY-ITEM", receive_item_or_value);
    gsiobj.declare_g2_function("RECEIVE-AND-RETURN-ITEM-COPY", receive_and_return_copy);
    gsiobj.declare_g2_function("RECEIVE-AND-DISPLAY-TRANSFER", receive_item_transfer);
    gsiobj.declare_g2_function("RECEIVE-REQUEST-ITEM-COPY", receive_request_for_copy);

    gsiobj.declare_g2_function("RPC-EXIT", rpc_exit);
    gsiobj.declare_g2_function("RPC-TEST", rpc_test);
    gsiobj.declare_g2_function("RPC-DECLARE-VARIABLE", rpc_declare_variable);
    gsiobj.declare_g2_remotefn("G2-REMOTE", 1, 0);
    gsiobj.declare_g2_remotefn("G2-ASYNCH", 1, 1, asynch_receiver);

    gsiobj.declare_g2_init(init);
    gsiobj.declare_g2_shutdown(shutdown);

    gsiobj.startgsi();
    return 0;
}

void prepare_test_3227()
{
    libgsi& gsi = libgsi::getInstance();

    gsi.dont_ignore_not_declared_variables();

    gsi.declare_g2_variable<int>("INTEGER-DAT");
    //gsi.declare_g2_variable<double>("FLOAT64-DAT"); // uncomment to cause an error
}

void prepare_test_3228()
{
    libgsi& gsi = libgsi::getInstance();

    gsi.ignore_not_declared_variables();

    gsi.declare_g2_variable<int>("INTEGER-DAT");
}

void prepare_test_3229()
{
    libgsi& gsi = libgsi::getInstance();

    gsi.ignore_not_registered_variables();

    gsi.declare_g2_variable<int>("INTEGER-DAT");
    gsi.declare_g2_variable<int>("NOT-REGISTERED");
}

void prepare_test_3230()
{
    libgsi& gsi = libgsi::getInstance();

    gsi.dont_ignore_not_registered_variables();

    gsi.declare_g2_variable<int>("INTEGER-DAT");
    gsi.declare_g2_variable<int>("NOT-REGISTERED");
}


