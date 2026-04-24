#include <cstdint>
#include <ctime>
#include <string>


namespace news {
    struct Article {
            uint8_t vix_bias;
            const time_t& published_at;
            const std::string& title;
            const std::string& publisher;
            const std::string& url;
    };
} // namespace news