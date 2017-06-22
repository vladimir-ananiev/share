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
#include <stdexcept>

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
    g2_none = GSI_NULL_TAG,
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
protected:
    g2_variable(g2_type type, bool declaration): handle(0), dec_type(g2_none), reg_type(g2_none) {
        if (declaration)
            dec_type = type;
        else
            reg_type = type;
    }
public:
    bool type_ok() { return dec_type == reg_type; }
    bool declared() { return dec_type != g2_none; }
    bool registered() { return reg_type != g2_none; }

    virtual g2_variable* clone() { return nullptr; }
    virtual void get_val(gsi_registered_item item) {}
    virtual void set_val(gsi_registered_item item) {}

    g2_type dec_type;
    g2_type reg_type;
    gsi_int handle;
};

/**
* Stores information about registered G2 variable of specific type, including variable value.
*/
template <typename T>
struct g2_typed_variable : public g2_variable {
    g2_typed_variable(bool declaration);
    virtual ~g2_typed_variable() {}

    virtual g2_variable* clone() {
        //g2_typed_variable<T>* cloned = new g2_typed_variable<T>(*this);
        //*cloned = *this;
        return new g2_typed_variable<T>(*this);
    }

    /**
    * Gets value of registered variable to GSI.
    */
    virtual void get_val(gsi_registered_item item);
    /**
    * Sets value of registered variable from GSI.
    */
    virtual void set_val(gsi_registered_item item);

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
inline g2_typed_variable<int>::g2_typed_variable(bool declaration) : g2_variable(g2_integer,declaration), d_has_def_value(false), d_cur_count(0) {}
template <>
inline g2_typed_variable<std::string>::g2_typed_variable(bool declaration) : g2_variable(g2_string,declaration), d_has_def_value(false), d_cur_count(0) {}
template <>
inline g2_typed_variable<bool>::g2_typed_variable(bool declaration) : g2_variable(g2_logical,declaration), d_has_def_value(false), d_cur_count(0) {}
template <>
inline g2_typed_variable<double>::g2_typed_variable(bool declaration) : g2_variable(g2_float,declaration), d_has_def_value(false), d_cur_count(0) {}

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

template <>
inline void g2_typed_variable<int>::set_val(gsi_registered_item item) { assign_temp_value(int_of(item)); }
template <>
#if defined(GSI_USE_WIDE_STRING_API)
inline void g2_typed_variable<std::string>::set_val(gsi_registered_item item) { assign_temp_value(libgsi::c_string(str_of(item))); }
#else
inline void g2_typed_variable<std::string>::set_val(gsi_registered_item item) { assign_temp_value(str_of(item)); }
#endif
template <>
inline void g2_typed_variable<bool>::set_val(gsi_registered_item item) { assign_temp_value(!!log_of(item)); }
template <>
inline void g2_typed_variable<double>::set_val(gsi_registered_item item) { assign_temp_value(flt_of(item)); }

/**
* Wrapper for libgsi.
*/
class libgsi : public singleton<libgsi> {
public:
    libgsi() : d_logger(g2::fasth::log_level::REGULAR), d_continuous(false), d_port(22041),
            d_ignore_not_registered_variables(false), d_ignore_not_declared_variables(false),
            d_vars_string_handle(0) {
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
        // Declare RPC handlers
        for (auto it = d_g2_declared_functions.begin(); it != d_g2_declared_functions.end(); it++ ) {
#if defined(GSI_USE_WIDE_STRING_API)
            short buf[100];
            gsi_rpc_declare_local(it->second, (gsi_char*)g2::fasth::libgsi::gensym_string(it->first, &buf[0], 100));
#else
            gsi_rpc_declare_local(it->second, const_cast<char*>(it->first.c_str()));
#endif
        }
    }

    void gsi_resume_context_() {
    }

    void gsi_receive_registration_(gsi_registration registration) {
        gsi_attr identifying_attribute_one = identifying_attr_of(registration,1);
        std::string name = name_of(registration);
        gsi_int type = type_of(registration);

        tthread::lock_guard<tthread::mutex> guard(d_mutex);
        std::shared_ptr<g2_variable> var;

        if (name == "VARIABLES")
        {
            d_vars_string_handle = handle_of(registration);
        }
        else 
        {
            if (d_g2_variables.count(name))
            {   // Variable was declared
                var = d_g2_variables[name];
                if (type>=GSI_INTEGER_TAG && type<=GSI_FLOAT64_TAG)
                    var->reg_type = (g2_type)type;
                else
                    return;
            }
            else
            {
                if (type == GSI_INTEGER_TAG)
                    var = std::shared_ptr<g2_variable>(new g2_typed_variable<int>(false));
                else if (type == GSI_FLOAT64_TAG)
                    var = std::shared_ptr<g2_variable>(new g2_typed_variable<double>(false));
                else if (type == GSI_LOGICAL_TAG)
                    var = std::shared_ptr<g2_variable>(new g2_typed_variable<bool>(false));
                else if (type == GSI_STRING_TAG || type == GSI_SYMBOL_TAG)
                {
                    var = std::shared_ptr<g2_variable>(new g2_typed_variable<std::string>(false));
                    var->reg_type = (g2_type)type;
                }
                else
                    return;
                d_g2_variables[name] = var;
            }
            var->handle = handle_of(registration);
        }

        printf("Variable %s (type tag %d) is registered\n", name.c_str(), type);
    }

    gsi_int gsi_initialize_context_(char* remote_process_init_string, gsi_int length) {
        if (d_g2_init)
            d_g2_init();
        return GSI_ACCEPT; 
    }

    void gsi_shutdown_context_() {
        if (d_g2_shutdown)
            d_g2_shutdown();
    }

    void gsi_g2_poll_() {
    }

    void gsi_set_data_(gsi_registered_item* registered_item_array, gsi_int count) {
        tthread::lock_guard<tthread::mutex> guard(d_mutex);

        for (int i=0; i<count; i++) {
            // Get item handle
            gsi_int handle = handle_of(registered_item_array[i]);
            // Find variable by handle
            auto it = std::find_if(d_g2_variables.begin(), d_g2_variables.end(), [handle](const std::pair<std::string, std::shared_ptr<g2_variable>> v) {
                return v.second->handle == handle;
            });
            bool ok = it != d_g2_variables.end(); // Shoud be true
            if (ok)
            {
                //printf("Set variable %s\n", it->first.c_str());
                if (!it->second->declared())
                {
                    ok = false;
                    if (!d_ignore_not_declared_variables && !d_vars_string_handle)
                        throw std::runtime_error("Variable was not declared");
                }
                else if (!it->second->type_ok())
                {
                    ok = false;
                    if (!d_ignore_not_declared_variables && !d_vars_string_handle)
                        throw std::runtime_error("Variable was declared with different type");
                }
            }
            if (ok)
            {
                it->second->set_val(registered_item_array[i]);
            }
        }
    }

    void gsi_get_data_(gsi_registered_item* registered_item_array, gsi_int count) {
        puts("gsi_get_data(0)");
        {
            tthread::lock_guard<tthread::mutex> guard(d_mutex);

            for (int i=0; i<count; i++) {
                puts("gsi_get_data(1)");
                set_status(registered_item_array[i], NO_ERR);
                puts("gsi_get_data(2)");
                // Get item handle
                gsi_int handle = handle_of(registered_item_array[i]);
                puts("gsi_get_data(3)");
                // Find variable by handle
                auto it = std::find_if(d_g2_variables.begin(), d_g2_variables.end(), [handle](const std::pair<std::string, std::shared_ptr<g2_variable>> v) {
                    return v.second->handle == handle;
                });
                puts("gsi_get_data(4)");
                bool ok = it != d_g2_variables.end(); // Shoud be true
                if (!ok)
                {
                    puts("gsi_get_data(5)");
                    // For regression tests
                    if (handle == d_vars_string_handle)
                    {
                        puts("gsi_get_data(6)");
                        std::string var_str = "";
                        std::for_each(d_g2_variables.begin(), d_g2_variables.end(), [&](const std::pair<std::string, std::shared_ptr<g2_variable>>& var)
                        {
                            printf("VARIABLES += %s\n", var.first.c_str());
                            if ((var.second->declared() && var.second->type_ok()) || d_ignore_not_declared_variables)
                                var_str += var.first + ",";
                            else if (d_ignore_not_registered_variables)
                                var_str += var.first + ",";
                        });
                        puts("gsi_get_data(7)");
                        puts(var_str.c_str());
                        if (var_str.length())
                            var_str = var_str.substr(0, var_str.length()-1);
                        puts("gsi_get_data(8)");
                        puts(var_str.c_str());
                        gsi_set_str(registered_item_array[i], (char*)var_str.c_str());
                        puts("gsi_get_data(9)");
                        puts(var_str.c_str());
                    }
                    continue;
                }
                else
                    printf("Var name = %s\n", it->first.c_str());

                it->second->get_val(registered_item_array[i]);

                if (!it->second->declared() || !it->second->type_ok())
                {
                    if (!d_ignore_not_declared_variables && !d_vars_string_handle)
                        throw std::runtime_error("Variable was not declared");
                }
            }
        }
        // Pass variable values to G2
        gsi_return_values(registered_item_array, count, current_context); 
    }

    void gsi_receive_deregistrations_(gsi_registered_item* registered_item_array, gsi_int count) {
        tthread::lock_guard<tthread::mutex> guard(d_mutex);

        for (int i=0; i<count; i++) {
            // Get item handle
            gsi_int handle = handle_of(registered_item_array[i]);
            // Find variable by handle
            auto it = std::find_if(d_g2_variables.begin(), d_g2_variables.end(), [handle](const std::pair<std::string, std::shared_ptr<g2_variable>> v) {
                return v.second->handle == handle;
            });
            if (it == d_g2_variables.end())
            {
                if (handle == d_vars_string_handle)
                    d_vars_string_handle = 0;
                continue;
            }
            //printf("Variable %s is unregistered\n", it->first.c_str());
            if (!it->second->declared())
                d_g2_variables.erase(it);
            else
            {
                it->second->handle = 0;
                it->second->reg_type = g2_none;
            }
        }
    }

    void gsi_receive_message_(char* message, gsi_int length) {
    }

    void gsi_pause_context_() {
    }

    typedef std::map<std::string, std::shared_ptr<g2_variable>> variable_map;
    typedef std::map<std::string, gsi_rpc_local_fn_type*> function_map;
    /**  
    * This function declares G2 variable for using in the tests.  
    * @param name Name of G2 variable as it's named in KB.  
    * @param handler Optional handler returning value of G2 variable.
    * @return true (if success) or false (if variable with such name was declared before).  
    */  
    template <typename T>
    bool declare_g2_variable(const char* name, std::function<T()> handler = nullptr) {
        tthread::lock_guard<tthread::mutex> guard(d_mutex);
        std::shared_ptr<g2_variable> var;
        if (d_g2_variables.count(name))
        {
            var = d_g2_variables[name];
            // Check if variable was already declared
            if (var->declared())
                return false;
            // Set var's dec_type
            std::shared_ptr<g2_variable> tmp = std::shared_ptr<g2_variable>(new g2_typed_variable<T>(true));
            var->dec_type = tmp->dec_type;
        }
        else
        {
            var = std::shared_ptr<g2_variable>(new g2_typed_variable<T>(true));
            d_g2_variables[name] = var;
        }
        ((g2_typed_variable<T>*)var.get())->d_handler = handler;
        return true;
    }
    /**
    * This function returns G2 variables map
    * @param only_declared If true (default), returns only declared variables. If false, returns all variables (including registered, but not declared).
    * @return The copy of declared G2 variables map
    */
    variable_map get_g2_variables(bool only_declared=true) {
        tthread::lock_guard<tthread::mutex> guard(d_mutex);
        variable_map vars;
        std::for_each(d_g2_variables.begin(), d_g2_variables.end(), [&](const std::pair<std::string, std::shared_ptr<g2_variable>>& var)
        {
            bool insert = false;
            if (var.second->declared())
            {
                insert = var.second->registered();
                if (!insert)
                {
                    if (!d_ignore_not_registered_variables)
                        throw std::runtime_error("Variable was not registered");
                    insert = true;
                }
            }
            else
            {   // Registered but not declared
                insert = !only_declared;
            }
            if (insert)
            {
                std::shared_ptr<g2_variable> copy(var.second->clone());
                vars[var.first] = copy;
            }
        });
        return vars;
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
        if (!d_g2_variables.count(name))
            return false;
        auto& var = d_g2_variables[name];
        if (!check_type<T>(var->dec_type))
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
        if (!d_g2_variables.count(name))
            return false;
        auto& var = d_g2_variables[name];
        if (!check_type<T>(var->dec_type))
            return false;
        if (!var->registered() && !d_ignore_not_registered_variables)
            throw std::runtime_error("Variable was not registered");
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
    /**
    * Specifies that declared, but not registered variables should not raise an error
    */
    void ignore_not_registered_variables() {
        tthread::lock_guard<tthread::mutex> guard(d_mutex);
        d_ignore_not_registered_variables = true;
    }
    /**
    * Specifies that declared, but not registered variables should raise an error
    */
    void dont_ignore_not_registered_variables() {
        tthread::lock_guard<tthread::mutex> guard(d_mutex);
        d_ignore_not_registered_variables = false;
    }
    /**
    * Specifies that registered, but not declared variables should not raise an error
    */
    void ignore_not_declared_variables() {
        tthread::lock_guard<tthread::mutex> guard(d_mutex);
        d_ignore_not_declared_variables = true;
    }
    /**
    * Specifies that registered, but not declared variables should not raise an error
    */
    void dont_ignore_not_declared_variables() {
        tthread::lock_guard<tthread::mutex> guard(d_mutex);
        d_ignore_not_declared_variables = false;
    }
    /**
    * This function declares G2 local function for using in the tests.
    * @param name Name of G2 local function as it's named in KB.
    * @param function Pointer to the local function.
    * @return true (if success) or false (if function with such name was declared before).  
    */  
    bool declare_g2_function(const char* name, gsi_rpc_local_fn_type* function) {
        tthread::lock_guard<tthread::mutex> guard(d_mutex);
        // Check if already declared
        if (d_g2_declared_functions.count(name))
            return false;
        d_g2_declared_functions[name] = function;
        return true;  
    }
    /**
    * This function declares G2 context initialization function.
    * @param init_fn Pointer to the initialization function.
    * @return true (if success) or false (if function was declared before).
    */
    bool declare_g2_init(std::function<void()> init_fn) {
        if (d_g2_init)
            return false;   // Already declared
        d_g2_init = init_fn;
        return true;
    }
    /**
    * This function declares G2 context shutdown function.
    * @param shutdown_fn Pointer to the shutdown function.
    * @return true (if success) or false (if function was declared before).
    */
    bool declare_g2_shutdown(std::function<void()> shutdown_fn) {
        if (d_g2_shutdown)
            return false;   // Already declared
        d_g2_shutdown = shutdown_fn;
        return true;
    }

private:
    tthread::mutex d_mutex;
    variable_map d_g2_variables; // map key is a name
    function_map d_g2_declared_functions; // map key is a name
    std::function<void()> d_g2_init;
    std::function<void()> d_g2_shutdown;
    bool d_continuous;
    int d_port;
    bool d_ignore_not_registered_variables;
    bool d_ignore_not_declared_variables;
    gsi_int d_vars_string_handle; // For regression tests

    g2::fasth::logger d_logger;
    static void error_handler_function(gsi_int error_context, gsi_int error_code, gsi_char *error_message);
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
