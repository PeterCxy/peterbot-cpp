#ifndef TG_TYPES_HPP
#define TG_TYPES_HPP
#include "util.hpp"
#include <nlohmann/json.hpp>
#include <optional>
#include <type_traits>
#include <vector>

// A macro to simply concat two lines of code
// difference from `CAT`: this does NOT concat tokens
// just the lines of code.
#define MY_CAT(x, y) x; y
// The folding function used when generating the struct fields
// converts `(type) name` to `type name;` using `PAIR` macro
// which I took from somewhere else and folds them together
// Basically, the `MY_CAT` is recursively applied onto the result
// of the previous application (deferred, not expanded yet)
// and the new argument, the result is kind of like
// > MY_CAT(MY_CAT(MY_CAT(arg1, PAIR(arg2)), PAIR(arg3)), PAIR(arg4)), PAIR(arg5)
// Note how the `__VA_ARGS__` is used here to enable recursive
// application. Also note that,
// 1. the final result lacks a final layer of `MY_CAT`. This can
//    be simply fixed by calling the most outer macro with an
//    extra empty argument, which is exactly what we do later in
//    this file.
// 2. the first argumeng `arg1` needs to be pre-processed because
//    it cannot be processed in this macro (we recursively apply
//    the same thing over and over again).
#define TELEGRAM_TYPE_GENERATOR_STRUCT(x, y, ...) \
    MY_CAT(x, PAIR(y)), __VA_ARGS__
// The folding function used to generate the `from_json` function
// it generates each assignment from json to the actual fields.
#define TELEGRAM_TYPE_GENERATOR_FROM_JSON(x, y, ...) \
    MY_CAT(x, _TELEGRAM_TYPE_GENERATOR_FROM_JSON(STRIP(y), TYPEOF(y))), __VA_ARGS__
// Inner macro to be called by TELEGRAM_TYPE_GENERATOR_FROM_JSON
// `x` should be the name of the field while `t` should be the type
// of the field. This double-layer is needed because the `STR()` macro
// will cancel the effect of `STRIP`, essentially converting `STRIP(x)`
// itself into a string literal, which is not what we want.
#define _TELEGRAM_TYPE_GENERATOR_FROM_JSON(x, t) \
    ret.x = get_from_json<t>(j, STR(x))

// A macro to generate a struct with `from_json` method
// corresponding to a Telegram API message type.
// See below inside Telegram namespace for usage.
// Note that struct fields should be defined with `(type) name`
// so that this macro can properly process them by
// generating the fields and also the `from_json` function.
#define TELEGRAM_TYPE_GENERATE(name, x, ...) \
    struct name { \
        M(EVAL(WHILE(PRED, TELEGRAM_TYPE_GENERATOR_STRUCT, PAIR(x), __VA_ARGS__))); \
        static name from_json(json *j) { \
            name ret; \
            M(EVAL(WHILE(PRED, TELEGRAM_TYPE_GENERATOR_FROM_JSON, \
                _TELEGRAM_TYPE_GENERATOR_FROM_JSON(STRIP(x), TYPEOF(x)), __VA_ARGS__))); \
            return ret; \
        } \
    };

namespace Telegram {
    using namespace nlohmann;

    // Source: <https://gist.github.com/fenbf/d2cd670704b82e2ce7fd>
    // `value` is true when T has `from_json`
    template<typename T>
    class Deserializable {
        private:
            typedef char YesType[1];
            typedef char NoType[2];

            template <typename C> static YesType& test(decltype(&C::from_json));
            template <typename C> static NoType& test(...);

        public:
            enum {
                value = sizeof(test<T>(0)) == sizeof(YesType),
                notValue = sizeof(test<T>(0)) != sizeof(YesType)
            };
    };

    // A type_trait helper to determine whether a template argument U
    // is in the form of `std::optional<T>`. If so, `T` is also unwrapped
    // inside the helper as `inner_type` for easier use.
    template<typename>
    struct IsOptional : std::false_type {};
    template<typename T>
    struct IsOptional<std::optional<T>> : std::true_type {
        typedef T inner_type;
    };

    // General case: T is some normal type which is not a `Deserializable`
    // nor an optional type. In this case, we rely on `nlohmann_json` to
    // properly parse the result for us. This is expected to work only
    // for primitive types and not any other complicated types.
    template<typename T>
    typename std::enable_if<
        Deserializable<T>::notValue && std::negation<IsOptional<T>>::value, T>::type
    get_from_json(json *j, const char* key) {
        return (*j)[key].get<T>();
    }

    // If T is itself an Telegram data type with my custom `from_json` method,
    // deserialize it from its own `from_json` method, which may in turn
    // call the general case of `get_from_json` again.
    template<typename T>
    typename std::enable_if<Deserializable<T>::value, T>::type
    get_from_json(json *j, const char* key) {
        return T::from_json(&(*j)[key]);
    }

    // If T is of `std::optional<U>`, we allow the entry to be missing,
    // and when it is present, use the other cases of `get_from_json`
    // to parse the entry.
    template<typename T>
    typename std::enable_if<IsOptional<T>::value, T>::type
    get_from_json(json *j, const char* key) {
        if (j->contains(key)) {
            return get_from_json<typename IsOptional<T>::inner_type>(j, key);
        } else {
            return {};
        }
    }

    // Definitions of Telegram data types and auto-generated content
    TELEGRAM_TYPE_GENERATE(Chat,
        (long) id,
        (std::string) type,
        (std::optional<std::string>) title,
        (std::optional<std::string>) username,
    )

    TELEGRAM_TYPE_GENERATE(User,
        (long) id,
        (bool) is_bot,
        (std::string) username,
        (std::optional<std::string>) first_name,
        (std::optional<std::string>) last_name,
    )

    TELEGRAM_TYPE_GENERATE(Message,
        (long) message_id,
        (std::optional<std::string>) text,
        (Chat) chat,
        (std::optional<User>) from, // Only present when a message is from real human
    )

    TELEGRAM_TYPE_GENERATE(Update,
        (unsigned long) update_id,
        (std::optional<Message>) message,
    )

    // Generic type of a vector of all the above types
    template<typename T>
    struct TgVector :
        std::vector<typename std::enable_if<Deserializable<T>::value, T>::type> {
        static TgVector<T> from_json(json *j) {
            TgVector<T> ret;
            for (auto& elem : *j) {
                json jelem = elem.get<json>();
                ret.push_back(T::from_json(&jelem));
            }
            return ret;
        }
    };

    // Updates = a vector of update
    using Updates = TgVector<Update>;
};

#endif