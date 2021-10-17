#include <iostream>
#include "Parser.h"

int main()
{
    //Example use of the parser combinator library
    auto parsea = prs::Char('a');
    auto parseb = prs::Char('b');
    auto parseab = parsea >> parseb | [](const auto& results)
    {
        return std::string({ results.first, results.second });
    };
    std::string ab = "ab";
    auto result = parseab(ab);
    if (result.Success())
        std::cout << "Successful parse! Result is \"" << result.GetResult() << "\"\n";
    else
        std::cout << "Parsing failed!\n";

    std::string x;
    std::getline(std::cin, x);
    return 0;
}
