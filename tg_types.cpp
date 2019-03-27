#include "tg_types.hpp"

using namespace Telegram;

Chat Chat::from_json(json *j) {
    return {
        .id = get_from_json<long>(j, "id"),
        .type = get_from_json<std::string>(j, "type"),
        .title = get_from_json_opt<std::string>(j, "title"),
        .username = get_from_json_opt<std::string>(j, "username"),
    };
}

User User::from_json(json *j) {
    return {
        .id = get_from_json<long>(j, "id"),
        .is_bot = get_from_json<bool>(j, "is_bot"),
        .username = get_from_json<std::string>(j, "username"),
        .first_name = get_from_json_opt<std::string>(j, "first_name"),
        .last_name = get_from_json_opt<std::string>(j, "last_name"),
    };
}

Message Message::from_json(json *j) {
    return {
        .message_id = get_from_json<long>(j, "message_id"),
        .text = get_from_json_opt<std::string>(j, "text"),
        .chat = get_from_json<Chat>(j, "chat"),
        .from = get_from_json_opt<User>(j, "from"),
    };
}

Update Update::from_json(json *j) {
    return {
        .update_id = get_from_json<unsigned long>(j, "update_id"),
        .message = get_from_json_opt<Message>(j, "message"),
    };
}