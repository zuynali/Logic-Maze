#ifndef LRU_REPLACER_H
#define LRU_REPLACER_H

#include "config.h"
#include <unordered_map>
#include <mutex>

namespace logicmaze {

class LRUReplacer {
private:
    struct Node {
        frame_id_t frame_id;
        Node* prev;
        Node* next;
        
        Node(frame_id_t id) : frame_id(id), prev(nullptr), next(nullptr) {}
    };

public:
    explicit LRUReplacer(size_t num_frames);
    ~LRUReplacer();

    LRUReplacer(const LRUReplacer&) = delete;
    LRUReplacer& operator=(const LRUReplacer&) = delete;

    bool Victim(frame_id_t* frame_id);
    void Pin(frame_id_t frame_id);
    void Unpin(frame_id_t frame_id);
    size_t Size() const;

private:
    void RemoveNode(Node* node);
    void AddToFront(Node* node);

    Node* head_;
    Node* tail_;
    std::unordered_map<frame_id_t, Node*> node_map_;
    mutable std::mutex mutex_;
};

}  // namespace logicmaze

#endif  // LRU_REPLACER_H
