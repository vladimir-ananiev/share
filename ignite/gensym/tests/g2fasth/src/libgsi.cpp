#include "libgsi.hpp"
#include <sstream>
#include <stdexcept>

/// Customized error handler function
void g2::fasth::libgsi::error_handler_function(gsi_int error_context, gsi_int error_code, gsi_char *error_message)
{
    std::stringstream ss;
    ss << "GSI error in context " << error_context;
    ss << ". Code: " << error_code;
    ss << ". Message: " << error_message;
    d_logger.log(g2::fasth::log_level::REGULAR, ss.str());
}

short* g2::fasth::libgsi::gensym_string(const std::wstring& wstr, short* dest, size_t num)
{
    if (!dest)
        throw std::invalid_argument("Destination pointer is null.");

    // Get string length
    size_t n = wstr.length();

    // Copy string to buffer
    short* buf = (short*)calloc(n + 1, sizeof(short));
    short* dst = buf;
    const wchar_t* src = wstr.c_str();
    while (num-- && (*dst++ = (short)*src++)) ;

    short* result = gsi_convert_wide_string_to_unicode(buf, GSI_CHAR_SET_GENSYM);
    free(buf);
    while (num-- && (*dest++ = (short)*result++)) ;
    return dest;
}

short* g2::fasth::libgsi::gensym_string(const std::string& cstr, short* dest, size_t num)
{
    if (!dest)
        throw std::invalid_argument("Destination pointer is null.");
    short* result = gsi_convert_string_to_unicode(const_cast<char*>(cstr.c_str()), GSI_CHAR_SET_GENSYM);
    while (num-- && (*dest++ = (short)*result++)) ;
    return dest;
}

std::wstring g2::fasth::libgsi::wide_string(short* gstr)
{
    if (!gstr)
        throw std::invalid_argument("Pointer to input string is null.");
    std::string cstr((char*)gsi_convert_unicode_to_string(gstr, GSI_CHAR_SET_GENSYM));
    short* wstr = gsi_convert_unicode_to_wide_string((short*)(cstr.c_str()), GSI_CHAR_SET_GENSYM);

    // Get string length
    const short* pos;
    for (pos = wstr; *pos; ++pos) ;
    size_t num = pos - wstr;

    // Copy string to buffer
    wchar_t* buf = (wchar_t*)calloc(num + 1, sizeof(wchar_t));
    wchar_t* dest = buf;
    while (num-- && (*dest++ = (wchar_t)*wstr++)) ;

    // Return result
    std::wstring result(buf);
    free(buf);
    return result;
}

std::string g2::fasth::libgsi::c_string(short* gstr)
{
    if (!gstr)
        throw std::invalid_argument("Pointer to input string is null.");
    std::string result(gsi_convert_unicode_to_string(gstr, GSI_CHAR_SET_GENSYM));
    return result;
}
