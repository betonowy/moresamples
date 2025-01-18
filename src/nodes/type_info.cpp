#include "type_info.hpp"

namespace nodes::type_info {
std::string_view stringify(SerializedType value) {
#define CASE(name)             \
    case SerializedType::name: \
        return #name

    switch (value) {
        CASE(UNDEFINED);
        CASE(Value);
        CASE(Math);
        CASE(Generator);
        CASE(Envelope);
        CASE(AudioOutput);
        CASE(Splitter);
        CASE(CombFilter);
        CASE(BiQuadFilter);
        CASE(FrequencyResponse);
    }
#undef CASE
    return "UNDEFINED";
}

SerializedType destringify(std::string_view str) {
#define CASE(name)                   \
    if (str == #name) {              \
        return SerializedType::name; \
    }                                \
    (void)0

    CASE(Value);
    CASE(Math);
    CASE(Generator);
    CASE(Envelope);
    CASE(AudioOutput);
    CASE(Splitter);
    CASE(CombFilter);
    CASE(BiQuadFilter);
    CASE(FrequencyResponse);

#undef CASE
    return SerializedType::UNDEFINED;
}
} // namespace nodes::type_info
