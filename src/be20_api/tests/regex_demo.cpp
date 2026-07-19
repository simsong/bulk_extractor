/*
 * Small test program to show how to use C++17 regular expressions.
 */

#include <iostream>
#include <string>
#include <regex>

int main(int argc,char **argv)
{
    std::string s("abc123def");
    std::regex  r("([0-9]+)");
    std::smatch m;
    if (std::regex_search(s, m, r)){
        std::cout << "Matches '" << m.str() << "'\n";
    }

    /* Try 32-bit vecotrs */
    std::u32string s32(U"Hello");
    std::cout << "len(s32)=" << s32.size() << "\n";
    std::basic_regex<char>  r8("([0-9]+)");
    std::basic_regex<wchar_t> r16(L"([0-9]+)");

    // this doesn't work:
    //std::basic_regex<char32_t>  r32(U"([0-9]+)");
    return(0);
}
