#ifndef TG_TYPES_HPP
#define TG_TYPES_HPP
#include <nlohmann/json.hpp>
#include <optional>
#include <type_traits>
#include <vector>

namespace Telegram {
    using namespace nlohmann;

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

    template<typename T>
    typename std::enable_if<Deserializable<T>::notValue, T>::type
    get_from_json(json *j, const char* key) {
        return (*j)[key].get<T>();
    }

    template<typename T>
    typename std::enable_if<Deserializable<T>::value, T>::type
    get_from_json(json *j, const char* key) {
        return T::from_json(&(*j)[key]);
    }

    template<typename T>
    std::optional<T> get_from_json_opt(json *j, const char* key) {
        if (j->contains(key)) {
            return get_from_json<T>(j, key);
        } else {
            return {};
        }
    }

    struct Chat {
        long id;
        std::string type;
        std::optional<std::string> title;
        std::optional<std::string> username;

        static Chat from_json(json *j);
    };

    struct User {
        long id;
        bool is_bot;
        std::string username;
        std::optional<std::string> first_name;
        std::optional<std::string> last_name;

        static User from_json(json *j);
    };

    struct Message {
        long message_id;
        std::optional<std::string> text;
        Chat chat;
        std::optional<User> from; // The USER the message is from, None if not from user

        static Message from_json(json *j);
    };

    struct Update {
        unsigned long update_id;
        std::optional<Message> message;

        static Update from_json(json *j);
    };

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