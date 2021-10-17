#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>
#include <concepts>
#include <initializer_list>
#include <optional>
#include <functional>

/*
* 
* A parser combinator library.
* This library tries to get as close as possible to the syntax of parser combinators in the FParsec library.
* The library is build using lambda expressions.
* 
*/

namespace prs
{
    struct StringState
    {
        bool success = true;
        int position = 0;

        StringState() { }

        StringState(bool success, int position)
            : success(success), position(position) { }
    };

    template<typename T>
    class ParseResult
    {
    private:
        StringState state;
        std::optional<T> result;
    public:
        ParseResult() : state(StringState()) { }

        ParseResult(const StringState& state, T result)
            : state(state), result(std::move(result)) { }

        ParseResult(const StringState& state)
            : state(state) { }

        const StringState& GetStringState() const
        {
            return state;
        }

        bool Success() const
        {
            return state.success;
        }

        int GetPosition() const
        {
            return state.position;
        }

        T& GetResult()
        {
            return result.value();
        }
    };

    template<typename T1, typename T2>
    struct Pair
    {
        T1 first;
        T2 second;
    };

    /*
    * A struct used as returntype of parsers that do not return a meaningful result.
    */
    struct Void { };

    template<typename T>
    [[nodiscard]]
    inline ParseResult<T> Success(int position, T result)
    {
        return ParseResult<T>({ true, position }, std::move(result));
    }

    template<typename T>
    [[nodiscard]]
    inline ParseResult<T> Fail(int position)
    {
        return ParseResult<T>({ false, position });
    }

    /*
    * A class representing a parser.
    */
    template<typename T>
    class Parser
    {
    private:
        std::function<ParseResult<T>(const StringState&, const std::string&)> parser;
    public:
        using ReturnType = T;

        template<typename P>
        [[nodiscard]]
        inline Parser(P parser)
            : parser(parser)
        {
            using PReturnType = decltype(parser(StringState(), std::string("")));
            static_assert(std::same_as<PReturnType, ParseResult<T>>,
                "The return type of \"parser\" of type \"P\" is not a ParseResult<T>.");
        }

        /*
        * Operator used for using the current parser to parse an input beginning at a specified position.
        */
        [[nodiscard]]
        inline ParseResult<T> operator()(const std::string& string, int position) const
        {
            return parser({ true, position }, string);
        }

        /*
        * Operator used for using the current parser to parse an input.
        */
        [[nodiscard]]
        inline ParseResult<T> operator()(const std::string& string) const
        {
            return parser({ true, 0 }, string);
        }

        /*
        * Operator used for mapping the result of the parser.
        */
        template<typename F>
        [[nodiscard]]
        inline auto operator|(const F& function) const
        {
            using ReturnType = decltype(function(ParseResult<T>().GetResult()));

            Parser<ReturnType> p = [=, *this](const StringState& state, const std::string& string)
            {
                auto result = parser(state, string);
                if (result.Success())
                    return Success(result.GetPosition(), function(result.GetResult()));
                return Fail<ReturnType>(state.position);
            };
            return p;
        }

        [[nodiscard]]
        inline auto operator~() const
        {
            Parser<Void> p = [*this](const StringState& state, const std::string& string)
            {
                auto result = parser(state, string);
                if (result.Success())
                    return Success(result.GetPosition(), Void());
                return Fail<Void>(state.position);
            };
            return p;
        }
    };

    /*
    * Operator used for creating sequences of parsers.
    * Returns a new parser that is a combination of the arguments.
    * The result always returns Void.
    */
    template<typename T>
    [[nodiscard]]
    inline Parser<T> operator>>(const Parser<T>& first, const Parser<Void>& second)
    {
        return [first, second](const StringState& state, const std::string& string)
        {
            if (!state.success)
                return Fail<T>(state.position);
            auto firstResult = first(string, state.position);
            if (!firstResult.Success())
                return Fail<T>(state.position);
            auto secondResult = second(string, firstResult.GetPosition());
            if (!secondResult.Success())
                return Fail<T>(state.position);
            return Success(secondResult.GetPosition(), std::move(firstResult.GetResult()));
        };
    }

    /*
    * Operator used for creating sequences of parsers.
    * Returns a new parser that is a combination of the arguments.
    */
    template<typename T>
    [[nodiscard]]
    inline Parser<T> operator>>(const Parser<Void>& first, const Parser<T>& second)
    {
        return [first, second](const StringState& state, const std::string& string)
        {
            if (!state.success)
                return Fail<T>(state.position);
            auto firstResult = first(string, state.position);
            if (!firstResult.Success())
                return Fail<T>(state.position);
            auto secondResult = second(string, firstResult.GetPosition());
            if (!secondResult.Success())
                return Fail<T>(state.position);
            return Success<T>(secondResult.GetPosition(), std::move(secondResult.GetResult()));
        };
    }

    /*
    * Operator used for creating sequences of parsers.
    * Returns a new parser that is a combination of the arguments.
    */
    [[nodiscard]]
    Parser<Void> operator>>(const Parser<Void>& first, const Parser<Void>& second);

    template<typename T1, typename T2>
    [[nodiscard]]
    inline Parser<Pair<T1, T2>> operator>>(Parser<T1> first, Parser<T2> second)
    {
        return [first, second](const StringState& state, const std::string& string)
        {
            if (!state.success)
                return Fail<Pair<T1, T2>>(state.position);
            auto firstResult = first(string, state.position);
            if (!firstResult.Success())
                return Fail<Pair<T1, T2>>(state.position);
            auto secondResult = second(string, firstResult.GetPosition());
            if (!secondResult.Success())
                return Fail<Pair<T1, T2>>(state.position);
            auto result = Pair<T1, T2>{ std::move(firstResult.GetResult()), std::move(secondResult.GetResult()) };
            return Success(secondResult.GetPosition(), std::move(result));
        };
    }

    /*
    * Returns a new parser that is successful if either of the arguments successfully parse a string.
    */
    template<typename T>
    [[nodiscard]]
    inline Parser<T> operator||(const Parser<T>& first, const Parser<T>& second)
    {
        return [=](const StringState& state, const std::string& string)
        {
            auto firstResult = first(string, state.position);
            if (firstResult.Success())
                return firstResult;
            auto secondResult = second(string, state.position);
            if (secondResult.Success())
                return secondResult;
            return Fail<T>(state.position);
        };
    }

    template<typename T>
    [[nodiscard]]
    inline Parser<T> Try(const Parser<T>& parser, const T& failResult)
    {
        return[=](const StringState& state, const std::string& string)
        {
            auto result = parser(string, state.position);
            if (result.Success())
                return result;
            return Success(state.position, failResult);
        };
    }

    [[nodiscard]]
    Parser<char> Char(char character);

    [[nodiscard]]
    Parser<std::string> String(const std::string& pattern);

    template<typename T>
    [[nodiscard]]
    inline Parser<std::vector<T>> Many(const Parser<T>& parser)
    {
        return [parser](const StringState& state, const std::string& string)
        {
            int position = state.position;
            std::vector<T> results;
            while (true)
            {
                if (position == string.length())
                    break;
                auto tempResult = parser(string, position);
                if (tempResult.Success())
                {
                    results.push_back(std::move(tempResult.GetResult()));
                    position = tempResult.GetPosition();
                }
                else
                    break;
            }
            return Success(position, results);
        };
    }

    template<typename T>
    [[nodiscard]]
    inline Parser<std::vector<T>> AtLeast(uint32_t count, const Parser<T>& parser)
    {
        return[=](const StringState& state, const std::string& string)
        {
            auto matches = Many(parser)(string, state.position);
            const auto& result = matches.GetResult();
            if (result.size() >= count)
                return Success(matches.GetPosition(), result);
            return Fail<std::vector<T>>(state.position);
        };
    }

    template<typename T>
    [[nodiscard]]
    inline Parser<std::vector<T>> AtLeastOne(const Parser<T>& parser)
    {
        return[=](const StringState& state, const std::string& string)
        {
            return AtLeast(parser, 1)(string, state.position);
        };
    }

    template<typename T>
    [[nodiscard]]
    inline Parser<std::vector<T>> Between(uint32_t min, uint32_t max, const Parser<T>& parser)
    {
        return[=](const StringState& state, const std::string& string)
        {
            auto matches = Many(parser)(string, state.position);
            const auto& result = matches.GetResult();
            if (result.size() >= min && result.size() <= max)
                return Success(matches.GetPosition(), result);
            return Fail<std::vector<T>>(state.position);
        };
    }

    template<typename T>
    [[nodiscard]]
    inline auto AnyOf(const std::initializer_list<Parser<T>>& parsers)
    {
        std::vector<Parser<T>> p = parsers;
        Parser<T> parser = [=](const StringState& state, const std::string& string)
        {
            if (state.position < string.length())
                for (size_t i = 0; i < p.size(); ++i)
                {
                    auto result = p[i](string, state.position);
                    if (result.Success())
                        return result;
                }
            return Fail<T>(state.position);
        };
        return parser;
    }

    [[nodiscard]]
    Parser<char> AnyOf(const std::string& characters);

    template<typename T>
    [[nodiscard]]
    inline auto Not(const Parser<T>& parser)
    {
        Parser<Void> p = [=](const StringState& state, const std::string& string)
        {
            auto result = parser(string, state.position);
            if (result.Success())
                return Fail<Void>(state.position);
            return Success(state.position + 1, Void());
        };
        return p;
    }

    extern Parser<char> any;
    extern Parser<char> letter;
    extern Parser<char> digit;
    extern Parser<char> whitespace;
    extern Parser<char> alphanumeric;
    extern Parser<std::string> whitespaces;
    extern Parser<std::string> letters;
    extern Parser<std::string> digits;
    extern Parser<std::string> word;
    extern Parser<int> integer;
    extern Parser<std::string> alphanumerics;
}

#endif
