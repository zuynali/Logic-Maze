#include "disk_manager.h"
#include <iostream>
#include <stdexcept>
#include <sys/stat.h>

namespace logicmaze {

DiskManager::DiskManager(const std::string& db_filename) 
    : db_filename_(db_filename), num_pages_(0) {
    
    // Check if database file exists
    struct stat buffer;
    bool file_exists = (stat(db_filename_.c_str(), &buffer) == 0);

    if (file_exists) {
        // Open existing database
        db_file_.open(db_filename_, std::ios::in | std::ios::out | std::ios::binary);
        if (!db_file_.is_open()) {
            throw std::runtime_error("Failed to open database file: " + db_filename_);
        }

        // Get file size and calculate number of pages
        db_file_.seekg(0, std::ios::end);
        size_t file_size = db_file_.tellg();
        num_pages_ = file_size / PAGE_SIZE;
        
        std::cout << "Opened existing database: " << db_filename_ 
                  << " (" << num_pages_ << " pages)" << std::endl;

        LoadFreePageList();
    } else {
        // Create new database
        db_file_.open(db_filename_, std::ios::out | std::ios::binary);
        if (!db_file_.is_open()) {
            throw std::runtime_error("Failed to create database file: " + db_filename_);
        }
        db_file_.close();

        // Reopen in read/write mode
        db_file_.open(db_filename_, std::ios::in | std::ios::out | std::ios::binary);
        if (!db_file_.is_open()) {
            throw std::runtime_error("Failed to reopen database file: " + db_filename_);
        }

        std::cout << "Created new database: " << db_filename_ << std::endl;
        InitializeDatabase();
    }
}

DiskManager::~DiskManager() {
    if (db_file_.is_open()) {
        SaveFreePageList();
        db_file_.close();
    }
}

void DiskManager::InitializeDatabase() {
    // Create header page (page 0)
    Page header_page;
    PageHeader* header = header_page.GetHeader();
    header->page_id = HEADER_PAGE_ID;
    header->page_type = PageType::HEADER;
    header->num_records = 0;
    
    // Write database metadata to header page data area
    char* data = header_page.GetData();
    uint32_t version = 1;
    uint32_t page_size = PAGE_SIZE;
    std::memcpy(data, &version, sizeof(version));
    std::memcpy(data + 4, &page_size, sizeof(page_size));
    std::memcpy(data + 8, &num_pages_, sizeof(num_pages_));
    
    header_page.UpdateChecksum();
    
    WritePage(HEADER_PAGE_ID, &header_page);
    num_pages_ = 1;
    
    SaveFreePageList();
}

void DiskManager::ReadPage(page_id_t page_id, Page* page) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (page_id >= num_pages_) {
        throw std::out_of_range("Page ID out of range: " + std::to_string(page_id));
    }

    // Seek to page position
    db_file_.seekg(page_id * PAGE_SIZE, std::ios::beg);
    if (db_file_.fail()) {
        throw std::runtime_error("Failed to seek to page " + std::to_string(page_id));
    }

    // Read page data
    db_file_.read(page->GetRawData(), PAGE_SIZE);
    if (db_file_.fail()) {
        throw std::runtime_error("Failed to read page " + std::to_string(page_id));
    }

    // Verify checksum (skip for header and free list pages)
    PageHeader* header = page->GetHeader();
    if (header->page_type != PageType::HEADER && 
        header->page_type != PageType::FREE_LIST &&
        header->checksum != 0) {  // Only verify if checksum was set
        if (!page->VerifyChecksum()) {
            std::cerr << "Warning: Checksum mismatch for page " << page_id << std::endl;
        }
    }
}

void DiskManager::WritePage(page_id_t page_id, const Page* page) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Extend file if necessary
    if (page_id >= num_pages_) {
        num_pages_ = page_id + 1;
    }

    // Seek to page position
    db_file_.seekp(page_id * PAGE_SIZE, std::ios::beg);
    if (db_file_.fail()) {
        throw std::runtime_error("Failed to seek to page " + std::to_string(page_id));
    }

    // Write page data
    db_file_.write(page->GetRawData(), PAGE_SIZE);
    if (db_file_.fail()) {
        throw std::runtime_error("Failed to write page " + std::to_string(page_id));
    }

    db_file_.flush();
}

page_id_t DiskManager::AllocatePage() {
    std::lock_guard<std::mutex> lock(mutex_);

    page_id_t new_page_id;

    // Reuse free page if available
    if (!free_pages_.empty()) {
        new_page_id = free_pages_.back();
        free_pages_.pop_back();
    } else {
        // Allocate new page at end of file
        new_page_id = num_pages_;
        num_pages_++;
    }

    return new_page_id;
}

void DiskManager::DeallocatePage(page_id_t page_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (page_id == HEADER_PAGE_ID) {
        throw std::invalid_argument("Cannot deallocate header page");
    }

    if (page_id >= num_pages_) {
        throw std::out_of_range("Page ID out of range: " + std::to_string(page_id));
    }

    free_pages_.push_back(page_id);
}

void DiskManager::Flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    db_file_.flush();
}

void DiskManager::LoadFreePageList() {
    if (num_pages_ < 2) {
        return;  // No free list page yet
    }

    // Free list is stored in page 1
    Page free_list_page;
    try {
        db_file_.seekg(1 * PAGE_SIZE, std::ios::beg);
        db_file_.read(free_list_page.GetRawData(), PAGE_SIZE);
        
        if (db_file_.fail()) {
            return;  // No free list yet
        }

        PageHeader* header = free_list_page.GetHeader();
        if (header->page_type != PageType::FREE_LIST) {
            return;  // Not a free list page
        }

        // Read free page IDs
        const char* data = free_list_page.GetData();
        uint32_t count = header->num_records;
        free_pages_.clear();
        free_pages_.reserve(count);

        for (uint32_t i = 0; i < count; ++i) {
            page_id_t page_id;
            std::memcpy(&page_id, data + i * sizeof(page_id_t), sizeof(page_id_t));
            free_pages_.push_back(page_id);
        }

        std::cout << "Loaded " << count << " free pages" << std::endl;
    } catch (...) {
        // Ignore errors, start with empty free list
    }
}

void DiskManager::SaveFreePageList() {
    // Create free list page
    Page free_list_page;
    PageHeader* header = free_list_page.GetHeader();
    header->page_id = 1;
    header->page_type = PageType::FREE_LIST;
    header->num_records = free_pages_.size();

    // Write free page IDs
    char* data = free_list_page.GetData();
    for (size_t i = 0; i < free_pages_.size(); ++i) {
        std::memcpy(data + i * sizeof(page_id_t), &free_pages_[i], sizeof(page_id_t));
    }

    free_list_page.UpdateChecksum();

    db_file_.seekp(1 * PAGE_SIZE, std::ios::beg);
    db_file_.write(free_list_page.GetRawData(), PAGE_SIZE);
    db_file_.flush();

    // Ensure num_pages_ accounts for free list page
    if (num_pages_ < 2) {
        num_pages_ = 2;
    }
}

}  // namespace logicmaze