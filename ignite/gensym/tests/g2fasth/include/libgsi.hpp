#ifndef INC_LIB_GSI_H
#define INC_LIB_GSI_H

#include <gsi_main.h>
#include "logger.hpp"
#include "gsi_callbacks.h"
#include "tinythread.h"
#include <algorithm>
#include <functional>
#include <map>
#include <string>
#include <memory>

namespace g2 {
namespace fasth {

/**
* Singleton tempate class. Provides a single instance of T class
*/
template <typename T>
class singleton {
public:
    /**
    * Returns instance of T class.
    */
    static T& getInstance() {
        static T instance;
        return instance;
    }
protected:
    /**
    * Protected constructor to disable creating the T instance directly.
    */
    singleton() {}
private:
    /**
    * Private constructors to disable copying and assignment.
    */
    singleton(const singleton&);
    singleton& operator=(const singleton&);
};

/**
* Enumerates supported types for G2 variables.
*/
enum g2_type {
    g2_integer = GSI_INTEGER_TAG,
    g2_symbol = GSI_SYMBOL_TAG,
    g2_string = GSI_STRING_TAG,
    g2_logical = GSI_LOGICAL_TAG,
    g2_float = GSI_FLOAT64_TAG
};

/**
* Stores information about registered G2 variable.
*/

struct g2_variable {
    g2_type type;
    gsi_int handle;

    virtual void get_val(gsi_registered_item item) {}
    virtual void set_val(gsi_registered_item item) {}
};

/**
* Stores information about registered G2 variable of specific type, including variable value.
*/
template <typename T>
struct g2_typed_variable : public g2_variable {
    g2_typed_variable();
    virtual ~g2_typed_variable() {}

    /**
    * Returns value of registered variable to GSI.
    */
    virtual void get_val(gsi_registered_item item);

    /**
    * Assigns the default value to G2 variable which will be returned always.
    * @param new_val New value to assign.
    */
    void assign_def_value(T new_val) {
        d_def_value = new_val;
        d_has_def_value = true;
    }

    /**
    * Assigns a temporary value to G2 variable which will be returned next N times.
    * @param new_val New value to assign.
    * @param count Number of times to return this value.
    */  
    void assign_temp_value(T new_val, int count = 1) {
        d_cur_value = new_val;
        d_cur_count = count;
    }

    std::function<T()> d_handler;

    virtual T value() {
        if (d_cur_count) {
            --d_cur_count;
            return d_cur_value;
        }
        else if (d_has_def_value) {
            return d_def_value;
        }
        else if (d_handler)
            return d_handler();
        else
            return T();
    }
private:
    T d_def_value;
    bool d_has_def_value;
    T d_cur_value;
    int d_cur_count;
};
/**
* Specialized constructors.
*/
template <>
inline g2_typed_variable<int>::g2_typed_variable() : d_has_def_value(false), d_cur_count(0) { type = g2_integer; }
template <>
inline g2_typed_variable<std::string>::g2_typed_variable() : d_has_def_value(false), d_cur_count(0) { type = g2_string; }
template <>
inline g2_typed_variable<bool>::g2_typed_variable() : d_has_def_value(false), d_cur_count(0) { type = g2_logical; }
template <>
inline g2_typed_variable<double>::g2_typed_variable() : d_has_def_value(false), d_cur_count(0) { type = g2_float; }

/**
* Function specializations.
*/
template <>
inline void g2_typed_variable<int>::get_val(gsi_registered_item item) { gsi_set_int(item, value()); }
#if defined(GSI_USE_WIDE_STRING_API)
template <>
inline void g2_typed_variable<std::string>::get_val(gsi_registered_item item) {
    short buf[100];
    gsi_set_str(item, (gsi_char*)g2::fasth::libgsi::gensym_string((const std::string&)value(), &buf[0], 100));
}
#else
template <>
inline void g2_typed_variable<std::string>::get_val(gsi_registered_item item) { gsi_set_str(item, const_cast<char*>(value().c_str())); }
#endif
template <>
inline void g2_typed_variable<bool>::get_val(gsi_registered_item item) { gsi_set_log(item, value()); }
template <>
inline void g2_typed_variable<double>::get_val(gsi_registered_item item) { gsi_set_flt(item, value()); }

/**
* Wrapper for libgsi.
*/
class libgsi : public singleton<libgsi> {
public:
    libgsi() : d_logger(g2::fasth::log_level::REGULAR), d_continuous(false), d_port(22041) {
        d_logger.add_output_stream(std::cout, g2::fasth::log_level::REGULAR);
    }
    /**
    * Setter for GSI continuous mode.
    */
    void continuous(bool val) {
        tthread::lock_guard<tthread::mutex> guard(d_mutex);
        d_continuous = val;
    }
    /**
    * Setter for GSI port number.
    */
    void port(int val) {
        tthread::lock_guard<tthread::mutex> guard(d_mutex);
        d_port = val;
    }

    /**
    * Initializes G2 Gateway, sets up network listeners, and passes control to the API function gsi_run_loop().
    */
    void startgsi() {
        GSI_SET_OPTIONS_FROM_COMPILE();

        int argc = 1;
        char argv0[] = "G2FASTH";
        char* argv[] = {argv0, nullptr};
        gsi_start(argc, argv);
    }

    /**
    * Invokes the G2 Gateway error handler and passes to it user-defined error information.
    * @param error_code An integer (greater than 1023) that represents a user-defined error code.
    * @param message Text that describes the user-defined error code.
    */
    void signal_error(int error_code, const char* message) {
        gsi_signal_error("G2FASTH", error_code, const_cast<char*>(message));
    }

    /// String conversion functions
    /**
    * Converts input wide string to Gensym string.
    * @param wstr Input wide string.
    * @param dest Pointer to the destination array.
    * @param num Maximum number of characters to be written.
    * @return Pointer to the destination array.
    */
    static short* gensym_string(const std::wstring& wstr, short* dest, size_t num);
    /**
    * Converts input c string to Gensym string.
    * @param cstr Input c string.
    * @param dest Pointer to the destination array.
    * @param num Maximum number of characters to be written.
    * @return Pointer to the destination array.
    */
    static short* gensym_string(const std::string& cstr, short* dest, size_t num);
    /**
    * Converts Gensym string to wide string.
    * @param gstr Input Gensym string.
    * @return Converted wide string.
    */
    static std::wstring wide_string(short* gstr);
    /**
    * Converts Gensym string to c string.
    * @param gstr Input Gensym string.
    * @return Converted c string.
    */
    static std::string c_string(short* gstr);

    /// GSI callback functions
    gsi_int gsi_get_tcp_port_() {
        tthread::lock_guard<tthread::mutex> guard(d_mutex);
        return d_port;
    }

    void gsi_set_up_() {
        tthread::lock_guard<tthread::mutex> guard(d_mutex);
        if (!d_continuous)
            gsi_set_option(GSI_ONE_CYCLE);
        gsi_install_error_handler(&libgsi::error_handler_function);
    }

    void gsi_resume_context_() {
    }

    void gsi_receive_registration_(gsi_registration registration) {
    }

    gsi_int gsi_initialize_context_(char* remote_process_init_string, gsi_int length) {
        return GSI_ACCEPT; 
    }

    void gsi_shutdown_context_() {
    }

    void gsi_g2_poll_() {
    }

    void gsi_set_data_(gsi_registered_item* registered_item_array, gsi_int count) {
    }

    void gsi_get_data_(gsi_registered_item* registered_item_array, gsi_int count) {
        gsi_registered_item *object;
        gsi_attr *ret_attr;
        gsi_int n;
        gsi_int obj_handle;

        tthread::lock_guard<tthread::mutex> guard(d_mutex);

        /* Allocate memory for the local object and attribute */
        object = gsi_make_registered_items(1);
        ret_attr = gsi_make_attrs_with_items(1);

        /* Loop through registered items sent to this function. */
        for(n=0; n<count; n++)
        {
            obj_handle = gsi_handle_of(registered_item_array[n]);

            auto var = std::find_if(d_g2_declared_variables.begin(), d_g2_declared_variables.end(), [obj_handle](const std::pair<std::string, std::shared_ptr<g2_variable>> v) {
                return v.second->handle == obj_handle;
            });
            if (var == d_g2_declared_variables.end())
                continue;

            /* Set the handle of the local object to the handle of
            the G2 object to which the value will be returned */
            gsi_set_handle(object[0], gsi_handle_of(registered_item_array[n]));

            /* Set the object_index attribute name and value.
            Note: Must enable object status return (if it is
            disabled) to get the index back to the object */
            gsi_set_attr_name(ret_attr[0], "DATA-VALUE");
            
            var->second->get_val(ret_attr[0]);

            gsi_return_attrs(object[0], ret_attr, 1, gsi_current_context());
        } /* End of for loop */

        /* Release the allocated memory */
        gsi_reclaim_registered_items(object);
        gsi_reclaim_attrs_with_items(ret_attr);
        return;
    }

    void gsi_receive_deregistrations_(gsi_registered_item* registered_item_array, gsi_int count) {
    }

    void gsi_receive_message_(char* message, gsi_int length) {
    }

    void gsi_pause_context_() {
    }

    typedef std::map<std::string, std::shared_ptr<g2_variable>> variable_map;
    /**  
    * This function declares G2 variable for using in the tests.  
    * @param name Name of G2 variable as it's named in KB.  
    * @param handler Optional handler returning value of G2 variable.
    * @return true (if success) or false (if variable with such name was declared before).  
    */  
    template <typename T>
    bool declare_g2_variable(const char* name, std::function<T()> handler = nullptr) {
        tthread::lock_guard<tthread::mutex> guard(d_mutex);
        // Check if already declared
        if (d_g2_declared_variables.count(name))
            return false;
        g2_typed_variable<T>* var = new g2_typed_variable<T>();
        var->d_handler = handler;
        d_g2_declared_variables[name] = std::shared_ptr<g2_variable>(var);
        return true;  
    }
    /**
    * This function returns [const reference to] declared G2 variables map
    * @return declared G2 variables map
    */
    const variable_map& get_declared_g2_variables() const {
        return d_g2_declared_variables;
    }
    /**
    * Assigns the default value to G2 variable which will be returned always.
    * @param name Name of G2 variable as it's named in KB.  
    * @param new_val New value to assign.
    * @return true (if success) or false.
    */
    template <typename T>
    bool assign_def_value(const char* name, T new_val) {
        tthread::lock_guard<tthread::mutex> guard(d_mutex);
        // Must be already declared
        if (!d_g2_declared_variables.count(name))
            return false;
        auto& var = d_g2_declared_variables[name];
        if (!check_type<T>(var->type))
            return false;
        ((g2_typed_variable<T>*)var.get())->assign_def_value(new_val);
        return true;
    }
    /**
    * Assigns a temporary value to G2 variable which will be returned next N times.
    * @param name Name of G2 variable as it's named in KB.  
    * @param new_val New value to assign.
    * @param count Number of times to return this value.
    * @return true (if success) or false.
    */
    template <typename T>
    bool assign_temp_value(const char* name, T new_val, int count = 1) {
        tthread::lock_guard<tthread::mutex> guard(d_mutex);
        // Must be already declared
        if (!d_g2_declared_variables.count(name))
            return false;
        auto& var = d_g2_declared_variables[name];
        if (!check_type<T>(var->type))
            return false;
        ((g2_typed_variable<T>*)var.get())->assign_temp_value(new_val, count);
        return true;
    }
    /**
    * Checks if type of variable is compatible with type T.
    * @param type Type of G2 variable.
    * @return true (if success) or false.
    */
    template <typename T>
    bool check_type(g2_type type);

private:
    variable_map d_g2_declared_variables; // map key is a name
    tthread::mutex d_mutex;
    bool d_continuous;
    int d_port;

    g2::fasth::logger d_logger;
    void error_handler_function(gsi_int error_context, gsi_int error_code, gsi_char *error_message);
};

template <>
inline bool libgsi::check_type<int>(g2_type type) { return type == g2_integer; }
template <>
inline bool libgsi::check_type<std::string>(g2_type type) { return type == g2_symbol || type == g2_string; }
template <>
inline bool libgsi::check_type<bool>(g2_type type) { return type == g2_logical; }
template <>
inline bool libgsi::check_type<double>(g2_type type) { return type == g2_float; }

}
}

#endif // !INC_LIB_GSI_H
