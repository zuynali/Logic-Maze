#include "../include/buffer_pool_manager.h"
#include <iostream>
#include <cassert>
#include <chrono>
#include <random>
#include <vector>

using namespace logicmaze;
using namespace std;

// Test 1: Basic Page Operations
void TestBasicPageOperations() {
    cout << "\n=== Test 1: Basic Page Operations ===" << endl;
    
    DiskManager disk_manager("test_db.db");
    BufferPoolManager bpm(10, &disk_manager);
    
    // Allocate new page
    page_id_t page_id;
    Page* page = bpm.NewPage(&page_id);
    assert(page != nullptr);
    cout << "✓ Allocated new page with ID: " << page_id << endl;
    
    // Write data to page
    char* data = page->GetData();
    const char* test_str = "Hello, Logic Maze Database!";
    strcpy(data, test_str);
    
    // Unpin and flush
    bpm.UnpinPage(page_id, true);
    bpm.FlushPage(page_id);
    cout << "✓ Wrote and flushed page" << endl;
    
    // Fetch page again
    page = bpm.FetchPage(page_id);
    assert(page != nullptr);
    assert(strcmp(page->GetData(), test_str) == 0);
    cout << "✓ Fetched page and verified data: \"" << page->GetData() << "\"" << endl;
    
    bpm.UnpinPage(page_id, false);
    
    cout << "Test 1 PASSED" << endl;
}

// Test 2: Page Persistence
void TestPagePersistence() {
    cout << "\n=== Test 2: Page Persistence ===" << endl;
    
    page_id_t page_ids[5];
    
    // Phase 1: Create pages and write data
    {
        DiskManager disk_manager("test_persistence.db");
        BufferPoolManager bpm(10, &disk_manager);
        
        for (int i = 0; i < 5; ++i) {
            Page* page = bpm.NewPage(&page_ids[i]);
            char* data = page->GetData();
            snprintf(data, 100, "Page %d data - test persistence", i);
            bpm.UnpinPage(page_ids[i], true);
        }
        
        bpm.FlushAllPages();
        cout << "✓ Created and flushed 5 pages" << endl;
    }
    
    // Phase 2: Reopen database and verify data
    {
        DiskManager disk_manager("test_persistence.db");
        BufferPoolManager bpm(10, &disk_manager);
        
        for (int i = 0; i < 5; ++i) {
            Page* page = bpm.FetchPage(page_ids[i]);
            assert(page != nullptr);
            
            char expected[100];
            snprintf(expected, 100, "Page %d data - test persistence", i);
            assert(strcmp(page->GetData(), expected) == 0);
            
            bpm.UnpinPage(page_ids[i], false);
        }
        
        cout << "✓ Verified all 5 pages after reopen" << endl;
    }
    
    cout << "Test 2 PASSED" << endl;
}

// Test 3: Buffer Pool Hit Rate
void TestBufferPoolHitRate() {
    cout << "\n=== Test 3: Buffer Pool Hit Rate ===" << endl;
    
    DiskManager disk_manager("test_hitrate.db");
    BufferPoolManager bpm(10, &disk_manager);  // Small buffer pool
    
    // Create 5 pages (less than buffer pool size)
    vector<page_id_t> page_ids;
    for (int i = 0; i < 5; ++i) {
        page_id_t page_id;
        Page* page = bpm.NewPage(&page_id);
        char* data = page->GetData();
        snprintf(data, 100, "Page %d", i);
        bpm.UnpinPage(page_id, true);
        page_ids.push_back(page_id);
    }
    
    // Access pages multiple times (should all be cache hits)
    for (int round = 0; round < 10; ++round) {
        for (page_id_t page_id : page_ids) {
            Page* page = bpm.FetchPage(page_id);
            assert(page != nullptr);
            bpm.UnpinPage(page_id, false);
        }
    }
    
    double hit_rate = bpm.GetHitRate();
    cout << "✓ Hit rate: " << (hit_rate * 100) << "%" << endl;
    cout << "  Hits: " << bpm.GetHitCount() << ", Misses: " << bpm.GetMissCount() << endl;
    
    assert(hit_rate > 0.80);  // Should be >80% hit rate
    cout << "Test 3 PASSED" << endl;
}

// Test 4: LRU Eviction
void TestLRUEviction() {
    cout << "\n=== Test 4: LRU Eviction ===" << endl;
    
    DiskManager disk_manager("test_lru.db");
    BufferPoolManager bpm(5, &disk_manager);  // Very small buffer pool
    
    // Create more pages than buffer pool size
    vector<page_id_t> page_ids;
    for (int i = 0; i < 10; ++i) {
        page_id_t page_id;
        Page* page = bpm.NewPage(&page_id);
        char* data = page->GetData();
        snprintf(data, 100, "Page %d", i);
        bpm.UnpinPage(page_id, true);
        page_ids.push_back(page_id);
    }
    
    cout << "✓ Created 10 pages with buffer pool size 5" << endl;
    cout << "  Hit rate: " << (bpm.GetHitRate() * 100) << "%" << endl;
    
    // Access first 5 pages again (some will be cache misses due to eviction)
    size_t misses_before = bpm.GetMissCount();
    for (int i = 0; i < 5; ++i) {
        Page* page = bpm.FetchPage(page_ids[i]);
        assert(page != nullptr);
        bpm.UnpinPage(page_ids[i], false);
    }
    size_t misses_after = bpm.GetMissCount();
    
    cout << "✓ Eviction occurred: " << (misses_after - misses_before) << " pages evicted" << endl;
    assert(misses_after > misses_before);  // Should have had cache misses
    
    cout << "Test 4 PASSED" << endl;
}

// Test 5: Random Access Benchmark
void TestRandomAccessBenchmark() {
    cout << "\n=== Test 5: Random Access Benchmark ===" << endl;
    
    DiskManager disk_manager("test_benchmark.db");
    BufferPoolManager bpm(100, &disk_manager);
    
    const int NUM_PAGES = 500;
    const int NUM_ACCESSES = 10000;
    
    // Create pages
    vector<page_id_t> page_ids;
    for (int i = 0; i < NUM_PAGES; ++i) {
        page_id_t page_id;
        Page* page = bpm.NewPage(&page_id);
        uint32_t* data = reinterpret_cast<uint32_t*>(page->GetData());
        data[0] = i;  // Store page index
        bpm.UnpinPage(page_id, true);
        page_ids.push_back(page_id);
    }
    
    cout << "✓ Created " << NUM_PAGES << " pages" << endl;
    
    // Random access pattern
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, NUM_PAGES - 1);
    
    auto start = chrono::high_resolution_clock::now();
    
    for (int i = 0; i < NUM_ACCESSES; ++i) {
        int idx = dis(gen);
        Page* page = bpm.FetchPage(page_ids[idx]);
        assert(page != nullptr);
        
        // Verify data
        uint32_t* data = reinterpret_cast<uint32_t*>(page->GetData());
        assert(data[0] == static_cast<uint32_t>(idx));
        
        bpm.UnpinPage(page_ids[idx], false);
    }
    
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    
    cout << "✓ Completed " << NUM_ACCESSES << " random accesses in " 
         << duration.count() << " ms" << endl;
    cout << "  Average: " << (duration.count() * 1000.0 / NUM_ACCESSES) 
         << " μs per access" << endl;
    cout << "  Hit rate: " << (bpm.GetHitRate() * 100) << "%" << endl;
    
    cout << "Test 5 PASSED" << endl;
}

// Test 6: Checksum Verification
void TestChecksumVerification() {
    cout << "\n=== Test 6: Checksum Verification ===" << endl;
    
    DiskManager disk_manager("test_checksum.db");
    BufferPoolManager bpm(10, &disk_manager);
    
    // Create page with data
    page_id_t page_id;
    Page* page = bpm.NewPage(&page_id);
    char* data = page->GetData();
    strcpy(data, "Checksum test data");
    
    // Update checksum and flush
    page->UpdateChecksum();
    bpm.UnpinPage(page_id, true);
    bpm.FlushPage(page_id);
    
    // Fetch and verify
    page = bpm.FetchPage(page_id);
    assert(page->VerifyChecksum());
    cout << "✓ Checksum verified successfully" << endl;
    
    bpm.UnpinPage(page_id, false);
    
    cout << "Test 6 PASSED" << endl;
}

int main() {
    cout << "=====================================" << endl;
    cout << "  Logic Maze Database - Phase 1 Tests" << endl;
    cout << "=====================================" << endl;
    
    try {
        TestBasicPageOperations();
        TestPagePersistence();
        TestBufferPoolHitRate();
        TestLRUEviction();
        TestRandomAccessBenchmark();
        TestChecksumVerification();
        
        cout << "\n=====================================" << endl;
        cout << "  ✓ ALL TESTS PASSED!" << endl;
        cout << "=====================================" << endl;
        
    } catch (const exception& e) {
        cerr << "\n✗ TEST FAILED: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}