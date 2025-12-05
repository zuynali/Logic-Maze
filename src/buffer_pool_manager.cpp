#include "buffer_pool_manager.h"
#include <iostream>

namespace logicmaze {

BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager* disk_manager)
    : pool_size_(pool_size),
      disk_manager_(disk_manager),
      hit_count_(0),
      miss_count_(0) {
    
    // Allocate page array
    pages_ = new Page[pool_size_];
    
    // Create LRU replacer
    replacer_ = new LRUReplacer(pool_size_);
    
    // Initialize free list with all frames
    free_list_.reserve(pool_size_);
    for (size_t i = 0; i < pool_size_; ++i) {
        free_list_.push_back(static_cast<frame_id_t>(i));
    }
}

BufferPoolManager::~BufferPoolManager() {
    FlushAllPages();
    delete[] pages_;
    delete replacer_;
}

Page* BufferPoolManager::FetchPage(page_id_t page_id) {
    std::lock_guard<std::mutex> lock(latch_);

    // Check if page is already in buffer pool
    auto it = page_table_.find(page_id);
    if (it != page_table_.end()) {
        // Cache hit
        frame_id_t frame_id = it->second;
        pin_count_[frame_id]++;
        replacer_->Pin(frame_id);
        hit_count_++;
        return &pages_[frame_id];
    }

    // Cache miss - need to fetch from disk
    miss_count_++;

    // Get a free frame
    frame_id_t frame_id = GetVictimFrame();
    if (frame_id == INVALID_FRAME_ID) {
        return nullptr;  // No available frames
    }

    // If frame was occupied, flush if dirty
    auto frame_it = frame_table_.find(frame_id);
    if (frame_it != frame_table_.end()) {
        page_id_t old_page_id = frame_it->second;
        
        // Flush if dirty
        if (dirty_[frame_id]) {
            // Update checksum before writing
            pages_[frame_id].UpdateChecksum();
            disk_manager_->WritePage(old_page_id, &pages_[frame_id]);
        }
        
        // Remove old page from page table
        page_table_.erase(old_page_id);
    }

    // Read page from disk
    disk_manager_->ReadPage(page_id, &pages_[frame_id]);
    
    // Update checksum after reading
    pages_[frame_id].UpdateChecksum();

    // Update tables
    page_table_[page_id] = frame_id;
    frame_table_[frame_id] = page_id;
    pin_count_[frame_id] = 1;
    dirty_[frame_id] = false;
    replacer_->Pin(frame_id);

    return &pages_[frame_id];
}

bool BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty) {
    std::lock_guard<std::mutex> lock(latch_);

    auto it = page_table_.find(page_id);
    if (it == page_table_.end()) {
        return false;  // Page not in buffer pool
    }

    frame_id_t frame_id = it->second;

    if (pin_count_[frame_id] <= 0) {
        return false;  // Already unpinned
    }

    pin_count_[frame_id]--;

    // Update dirty flag
    if (is_dirty) {
        dirty_[frame_id] = true;
    }

    // If pin count reaches 0, make it evictable
    if (pin_count_[frame_id] == 0) {
        replacer_->Unpin(frame_id);
    }

    return true;
}

bool BufferPoolManager::FlushPage(page_id_t page_id) {
    std::lock_guard<std::mutex> lock(latch_);

    auto it = page_table_.find(page_id);
    if (it == page_table_.end()) {
        return false;  // Page not in buffer pool
    }

    frame_id_t frame_id = it->second;
    
    // Update checksum before writing
    pages_[frame_id].UpdateChecksum();
    
    disk_manager_->WritePage(page_id, &pages_[frame_id]);
    dirty_[frame_id] = false;

    return true;
}

void BufferPoolManager::FlushAllPages() {
    std::lock_guard<std::mutex> lock(latch_);

    for (auto& entry : page_table_) {
        page_id_t page_id = entry.first;
        frame_id_t frame_id = entry.second;
        
        if (dirty_[frame_id]) {
            // Update checksum before writing
            pages_[frame_id].UpdateChecksum();
            
            disk_manager_->WritePage(page_id, &pages_[frame_id]);
            dirty_[frame_id] = false;
        }
    }
}

Page* BufferPoolManager::NewPage(page_id_t* page_id) {
    std::lock_guard<std::mutex> lock(latch_);

    // Get a free frame
    frame_id_t frame_id = GetVictimFrame();
    if (frame_id == INVALID_FRAME_ID) {
        return nullptr;  // No available frames
    }

    // If frame was occupied, flush if dirty
    auto frame_it = frame_table_.find(frame_id);
    if (frame_it != frame_table_.end()) {
        page_id_t old_page_id = frame_it->second;
        
        if (dirty_[frame_id]) {
            disk_manager_->WritePage(old_page_id, &pages_[frame_id]);
        }
        
        page_table_.erase(old_page_id);
    }

    // Allocate new page on disk
    *page_id = disk_manager_->AllocatePage();

    // Reset page
    pages_[frame_id].Reset();
    PageHeader* header = pages_[frame_id].GetHeader();
    header->page_id = *page_id;
    header->page_type = PageType::DATA;
    
    // Update checksum for new page
    pages_[frame_id].UpdateChecksum();

    // Update tables
    page_table_[*page_id] = frame_id;
    frame_table_[frame_id] = *page_id;
    pin_count_[frame_id] = 1;
    dirty_[frame_id] = true;  // New page is dirty
    replacer_->Pin(frame_id);

    return &pages_[frame_id];
}

bool BufferPoolManager::DeletePage(page_id_t page_id) {
    std::lock_guard<std::mutex> lock(latch_);

    auto it = page_table_.find(page_id);
    if (it == page_table_.end()) {
        // Page not in buffer pool, just deallocate on disk
        disk_manager_->DeallocatePage(page_id);
        return true;
    }

    frame_id_t frame_id = it->second;

    // Cannot delete pinned page
    if (pin_count_[frame_id] > 0) {
        return false;
    }

    // Remove from tables
    page_table_.erase(page_id);
    frame_table_.erase(frame_id);
    dirty_.erase(frame_id);
    pin_count_.erase(frame_id);

    // Add frame back to free list
    free_list_.push_back(frame_id);

    // Deallocate on disk
    disk_manager_->DeallocatePage(page_id);

    return true;
}

frame_id_t BufferPoolManager::GetVictimFrame() {
    // First try to get from free list
    if (!free_list_.empty()) {
        frame_id_t frame_id = free_list_.back();
        free_list_.pop_back();
        return frame_id;
    }

    // No free frames, evict using LRU
    frame_id_t victim_frame;
    if (replacer_->Victim(&victim_frame)) {
        return victim_frame;
    }

    // No evictable frames
    return INVALID_FRAME_ID;
}

}  // namespace logicmaze