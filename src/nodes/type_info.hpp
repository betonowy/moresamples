#pragma once

#include <string_view>

#define TYPE_INFO_STR(type) ::nodes::type_info::type
#define TYPE_INFO(type) ::nodes::type_info::SerializedType::type

namespace nodes::type_info {
enum class SerializedType {
    UNDEFINED,
    Value,
    Math,
    Generator,
    Envelope,
    AudioOutput,
    Splitter,
    CombFilter,
    HighPassFilter,
};

#define TYPE_INFO_STR_DEFINITION(type)  \
    inline constexpr auto type = #type; \
    static_assert(true || static_cast<bool>(SerializedType::type))

TYPE_INFO_STR_DEFINITION(Value);
TYPE_INFO_STR_DEFINITION(Math);
TYPE_INFO_STR_DEFINITION(Generator);
TYPE_INFO_STR_DEFINITION(Envelope);
TYPE_INFO_STR_DEFINITION(AudioOutput);
TYPE_INFO_STR_DEFINITION(Splitter);
TYPE_INFO_STR_DEFINITION(CombFilter);
TYPE_INFO_STR_DEFINITION(HighPassFilter);

#undef TYPE_INFO_STR_DEFINITION

std::string_view stringify(SerializedType value);
SerializedType destringify(std::string_view str);
} // namespace nodes::type_info
