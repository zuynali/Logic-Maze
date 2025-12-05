#include "../include/lru_replacer.h"

namespace logicmaze {

LRUReplacer::LRUReplacer(size_t num_frames) {
    // Create dummy head and tail nodes
    head_ = new Node(INVALID_FRAME_ID);
    tail_ = new Node(INVALID_FRAME_ID);
    head_->next = tail_;
    tail_->prev = head_;
}

LRUReplacer::~LRUReplacer() {
    // Clean up all nodes
    Node* curr = head_;
    while (curr != nullptr) {
        Node* next = curr->next;
        delete curr;
        curr = next;
    }
}

bool LRUReplacer::Victim(frame_id_t* frame_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if there are any evictable frames
    if (tail_->prev == head_) {
        return false;  // No unpinned frames
    }

    // Remove least recently used frame (at tail)
    Node* victim_node = tail_->prev;
    *frame_id = victim_node->frame_id;

    RemoveNode(victim_node);
    node_map_.erase(*frame_id);
    delete victim_node;

    return true;
}

void LRUReplacer::Pin(frame_id_t frame_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = node_map_.find(frame_id);
    if (it == node_map_.end()) {
        return;  // Frame not in replacer
    }

    // Remove from list (pinned frames are not tracked)
    Node* node = it->second;
    RemoveNode(node);
    node_map_.erase(it);
    delete node;
}

void LRUReplacer::Unpin(frame_id_t frame_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = node_map_.find(frame_id);
    if (it != node_map_.end()) {
        // Frame already in replacer, move to front (most recently used)
        Node* node = it->second;
        RemoveNode(node);
        AddToFront(node);
    } else {
        // Add new unpinned frame to front
        Node* node = new Node(frame_id);
        AddToFront(node);
        node_map_[frame_id] = node;
    }
}

size_t LRUReplacer::Size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return node_map_.size();
}

void LRUReplacer::RemoveNode(Node* node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

void LRUReplacer::AddToFront(Node* node) {
    node->next = head_->next;
    node->prev = head_;
    head_->next->prev = node;
    head_->next = node;
}

}  // namespace logicmaze