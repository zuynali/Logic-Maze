#ifndef DISK_MANAGER_H
#define DISK_MANAGER_H

#include "config.h"
#include "page.h"
#include <string>
#include <fstream>
#include <mutex>
#include <vector>

namespace logicmaze {

class DiskManager {
public:
    explicit DiskManager(const std::string& db_filename);
    ~DiskManager();

    DiskManager(const DiskManager&) = delete;
    DiskManager& operator=(const DiskManager&) = delete;

    void ReadPage(page_id_t page_id, Page* page);
    void WritePage(page_id_t page_id, const Page* page);
    page_id_t AllocatePage();
    void DeallocatePage(page_id_t page_id);
    page_id_t GetNumPages() const { return num_pages_; }
    void Flush();

private:
    void InitializeDatabase();
    void LoadFreePageList();
    void SaveFreePageList();

    std::string db_filename_;
    std::fstream db_file_;
    page_id_t num_pages_;
    std::vector<page_id_t> free_pages_;
    mutable std::mutex mutex_;
};

}  // namespace logicmaze

#endif  // DISK_MANAGER_H
