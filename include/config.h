#ifndef CONFIG_H
#define CONFIG_H

#include <cstddef>
#include <cstdint>

namespace logicmaze {

// Page configuration
constexpr size_t PAGE_SIZE = 8192;  // 8KB pages
constexpr size_t PAGE_HEADER_SIZE = 128;  // 128 bytes for page header
constexpr size_t PAGE_DATA_SIZE = PAGE_SIZE - PAGE_HEADER_SIZE;  // 8064 bytes

// Buffer pool configuration
constexpr size_t BUFFER_POOL_SIZE = 100;  // 100 pages = 800KB

// Page types
enum class PageType : uint8_t {
    INVALID = 0,
    HEADER = 1,
    DATA = 2,
    INDEX = 3,
    FREE_LIST = 4
};

// Page ID type
using page_id_t = uint32_t;
constexpr page_id_t INVALID_PAGE_ID = 0xFFFFFFFF;
constexpr page_id_t HEADER_PAGE_ID = 0;

// Frame ID type (buffer pool frame)
using frame_id_t = int32_t;
constexpr frame_id_t INVALID_FRAME_ID = -1;

}  // namespace logicmaze

#endif  // CONFIG_H
