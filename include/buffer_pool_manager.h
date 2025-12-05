#ifndef BUFFER_POOL_MANAGER_H
#define BUFFER_POOL_MANAGER_H

#include "config.h"
#include "page.h"
#include "disk_manager.h"
#include "lru_replacer.h"
#include <unordered_map>
#include <mutex>
#include <memory>

namespace logicmaze {

class BufferPoolManager {
public:
    BufferPoolManager(size_t pool_size, DiskManager* disk_manager);
    ~BufferPoolManager();

    BufferPoolManager(const BufferPoolManager&) = delete;
    BufferPoolManager& operator=(const BufferPoolManager&) = delete;

    Page* FetchPage(page_id_t page_id);
    bool UnpinPage(page_id_t page_id, bool is_dirty);
    bool FlushPage(page_id_t page_id);
    void FlushAllPages();
    Page* NewPage(page_id_t* page_id);
    bool DeletePage(page_id_t page_id);

    size_t GetPoolSize() const { return pool_size_; }
    size_t GetHitCount() const { return hit_count_; }
    size_t GetMissCount() const { return miss_count_; }
    double GetHitRate() const {
        size_t total = hit_count_ + miss_count_;
        return total == 0 ? 0.0 : static_cast<double>(hit_count_) / total;
    }

private:
    frame_id_t GetVictimFrame();

    size_t pool_size_;
    Page* pages_;
    DiskManager* disk_manager_;
    LRUReplacer* replacer_;
    
    std::unordered_map<page_id_t, frame_id_t> page_table_;
    std::unordered_map<frame_id_t, page_id_t> frame_table_;
    std::unordered_map<frame_id_t, int> pin_count_;
    std::unordered_map<frame_id_t, bool> dirty_;
    
    std::vector<frame_id_t> free_list_;
    
    mutable std::mutex latch_;
    
    size_t hit_count_;
    size_t miss_count_;
};

}  // namespace logicmaze

#endif  // BUFFER_POOL_MANAGER_H
