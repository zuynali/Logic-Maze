#ifndef PAGE_H
#define PAGE_H

#include "config.h"
#include <cstring>
#include <cstdint>

namespace logicmaze {

// Page header structure (128 bytes)
struct PageHeader {
    page_id_t page_id;           // 4 bytes
    PageType page_type;          // 1 byte
    uint8_t padding1[3];         // 3 bytes (alignment)
    uint32_t num_records;        // 4 bytes
    uint32_t free_space;         // 4 bytes
    uint32_t free_space_offset;  // 4 bytes - where free space starts
    uint32_t checksum;           // 4 bytes
    uint8_t reserved[104];       // 104 bytes (for future use)
    
    PageHeader() 
        : page_id(INVALID_PAGE_ID),
          page_type(PageType::INVALID),
          num_records(0),
          free_space(PAGE_DATA_SIZE),
          free_space_offset(0),
          checksum(0) {
        std::memset(padding1, 0, sizeof(padding1));
        std::memset(reserved, 0, sizeof(reserved));
    }
};

static_assert(sizeof(PageHeader) == PAGE_HEADER_SIZE, 
              "PageHeader size must be exactly 128 bytes");

// Page class representing an 8KB page
class Page {
public:
    Page() {
        std::memset(data_, 0, PAGE_SIZE);
    }

    // Get the page header
    PageHeader* GetHeader() {
        return reinterpret_cast<PageHeader*>(data_);
    }

    const PageHeader* GetHeader() const {
        return reinterpret_cast<const PageHeader*>(data_);
    }

    // Get pointer to data area (after header)
    char* GetData() {
        return data_ + PAGE_HEADER_SIZE;
    }

    const char* GetData() const {
        return data_ + PAGE_HEADER_SIZE;
    }

    // Get raw page data
    char* GetRawData() {
        return data_;
    }

    const char* GetRawData() const {
        return data_;
    }

    // Reset page to empty state
    void Reset() {
        std::memset(data_, 0, PAGE_SIZE);
        PageHeader* header = GetHeader();
        *header = PageHeader();
    }

    // Calculate simple checksum
    uint32_t CalculateChecksum() const {
        uint32_t sum = 0;
        const uint32_t* data = reinterpret_cast<const uint32_t*>(data_ + PAGE_HEADER_SIZE);
        size_t count = PAGE_DATA_SIZE / sizeof(uint32_t);
        for (size_t i = 0; i < count; ++i) {
            sum ^= data[i];
        }
        return sum;
    }

    // Verify checksum
    bool VerifyChecksum() const {
        return GetHeader()->checksum == CalculateChecksum();
    }

    // Update checksum
    void UpdateChecksum() {
        GetHeader()->checksum = CalculateChecksum();
    }

private:
    char data_[PAGE_SIZE];
};

}  // namespace logicmaze

#endif  // PAGE_H
