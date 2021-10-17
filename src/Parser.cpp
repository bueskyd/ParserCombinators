#include "Parser.h"

namespace prs
{
    Parser<char> AnyOf(const std::string& characters)
    {
        Parser<char> parser = [=](const StringState& state, const std::string& string)
        {
            if (state.position < static_cast<int>(string.length()))
                for (size_t i = 0; i < characters.size(); ++i)
                    if (string[state.position] == characters[i])
                        return Success<char>(state.position + 1, characters[i]);
            return Fail<char>(state.position);
        };
        return parser;
    }

    Parser<Void> operator>>(const Parser<Void>& first, const Parser<Void>& second)
    {
        return [first, second](const StringState& state, const std::string& string)
        {
            if (!state.success)
                return Fail<Void>(state.position);
            auto firstResult = first(string, state.position);
            if (!firstResult.Success())
                return Fail<Void>(state.position);
            auto secondResult = second(string, firstResult.GetPosition());
            if (!secondResult.Success())
                return Fail<Void>(state.position);
            return Success(secondResult.GetPosition(), Void());
        };
    }

    Parser<char> Char(char character)
    {
        return [character](const StringState& state, const std::string& string)
        {
            bool success = string[state.position] == character;
            if (success)
                return Success(state.position + 1, character);
            return Fail<char>(state.position);
        };
    }

    Parser<std::string> String(const std::string& pattern)
    {
        return [pattern](const StringState& state, const std::string& string)
        {
            for (size_t i = 0; i < pattern.length(); ++i)
            {
                if (string[state.position + i] != pattern[i] ||
                    state.position + i >= string.length())
                    return Fail<std::string>(state.position);
            }
            int position = state.position + static_cast<int>(pattern.length());
            return Success(position, pattern);
        };
    }

    Parser<char> any = [](const StringState& state, const std::string& string)
    {
        if (state.position < static_cast<int>(string.length()))
            return Success(state.position + 1, string[state.position]);
        return Fail<char>(state.position);
    };

    Parser<char> letter = [](const StringState& state, const std::string& string)
    {
        char c = string[state.position];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
            return Success(state.position + 1, c);
        return Fail<char>(state.position);
    };

    Parser<char> digit = [](const StringState& state, const std::string& string)
    {
        char c = string[state.position];
        if (c >= '0' && c <= '9')
            return Success(state.position + 1, c);
        return Fail<char>(state.position);
    };

    Parser<char> whitespace = [](const StringState& state, const std::string& string)
    {
        char c = string[state.position];
        if (c == ' ' || c == '\n' || c == '\t')
            return Success(state.position + 1, c);
        return Fail<char>(state.position);
    };

    Parser<char> alphanumeric = [](const StringState& state, const std::string& string)
    {
        char c = string[state.position];
        if ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9'))
            return Success(state.position + 1, c);
        return Fail<char>(state.position);
    };

    Parser<std::string> whitespaces = [](const StringState& state, const std::string& string)
    {
        int position = state.position;
        char c;
        while (true)
        {
            c = string[position];
            if (c == ' ' || c == '\n' || c == '\t')
                ++position;
            else
                break;
        }
        int count = position - state.position;
        return Success(position, string.substr(state.position, count));
    };

    Parser<std::string> letters = [](const StringState& state, const std::string& string)
    {
        int position = state.position;
        char c;
        while (true)
        {
            c = string[position];
            if ((c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z'))
                ++position;
            else
                break;
        }
        int count = position - state.position;
        return Success(position, string.substr(state.position, count));
    };

    Parser<std::string> digits = [](const StringState& state, const std::string& string)
    {
        int position = state.position;
        char c;
        while (true)
        {
            c = string[position];
            if (c >= '0' && c <= '9')
                ++position;
            else
                break;
        }
        int count = position - state.position;
        return Success(position, string.substr(state.position, count));
    };

    Parser<std::string> word = whitespaces >> letters | [](const auto& pair)
    {
        return pair.second;
    };

    Parser<int> integer =
        Parser<int>([](const StringState& state, const std::string& string)
            {
                int position = state.position;
                char c = string[position];
                if (c == '-')
                    ++position;

                //Fail when the first character is '0' and if the integer has at least two digits
                char c1 = string[position];
                char c2 = string[position + 1];
                if (c1 == '0' && c2 >= '0' && c2 <= '9')
                    return Fail<int>(state.position);

                while (true)
                {
                    c = string[position];
                    if (c >= '0' && c <= '9')
                        ++position;
                    else
                        break;
                }
                int count = position - state.position;
                int result = std::stoi(string.substr(state.position, count));
                return Success(position, result);
            });

    Parser<std::string> alphanumerics = [](const StringState& state, const std::string& string)
    {
        int position = state.position;
        char c;
        while (true)
        {
            c = string[position];
            if ((c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9'))
                ++position;
            else
                break;
        }
        int count = position - state.position;
        return Success(position, string.substr(state.position, count));
    };
}
