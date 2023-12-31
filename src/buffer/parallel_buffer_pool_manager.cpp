//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// parallel_buffer_pool_manager.cpp
//
// Identification: src/buffer/buffer_pool_manager.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/parallel_buffer_pool_manager.h"

namespace bustub {

ParallelBufferPoolManager::ParallelBufferPoolManager(size_t num_instances, size_t pool_size, DiskManager *disk_manager,
                                                     LogManager *log_manager) {
  // Allocate and create individual BufferPoolManagerInstances
  alloc_index_ = 0;
  pool_size_ = pool_size;
  num_instances_ = num_instances;
  instaces_ = new BufferPoolManagerInstance *[num_instances];
  for (size_t i = 0; i < num_instances; ++i) {
    instaces_[i] = new BufferPoolManagerInstance(pool_size, num_instances, i, disk_manager, log_manager);
  }
}

// Update constructor to destruct all BufferPoolManagerInstances and deallocate any associated memory
ParallelBufferPoolManager::~ParallelBufferPoolManager() {
  for (size_t i = 0; i < num_instances_; ++i) {
    delete instaces_[i];
  }
  delete[] instaces_;
}

size_t ParallelBufferPoolManager::GetPoolSize() {
  // Get size of all BufferPoolManagerInstances
  return num_instances_ * pool_size_;
}

BufferPoolManager *ParallelBufferPoolManager::GetBufferPoolManager(page_id_t page_id) {
  // Get BufferPoolManager responsible for handling given page id. You can use this method in your other methods.
  return instaces_[page_id % num_instances_];
}

Page *ParallelBufferPoolManager::FetchPgImp(page_id_t page_id) {
  // Fetch page for page_id from responsible BufferPoolManagerInstance
  BufferPoolManager *buffer_manager = GetBufferPoolManager(page_id);
  return buffer_manager->FetchPage(page_id);
}

bool ParallelBufferPoolManager::UnpinPgImp(page_id_t page_id, bool is_dirty) {
  // Unpin page_id from responsible BufferPoolManagerInstance
  BufferPoolManager *buffer_manager = GetBufferPoolManager(page_id);
  return buffer_manager->UnpinPage(page_id, is_dirty);
}

bool ParallelBufferPoolManager::FlushPgImp(page_id_t page_id) {
  // Flush page_id from responsible BufferPoolManagerInstance
  BufferPoolManager *buffer_manager = GetBufferPoolManager(page_id);
  return buffer_manager->FlushPage(page_id);
}

Page *ParallelBufferPoolManager::NewPgImp(page_id_t *page_id) {
  // create new page. We will request page allocation in a round robin manner from the underlying
  // BufferPoolManagerInstances
  // 1.   From a starting index of the BPMIs, call NewPageImpl until either 1) success and return 2) looped around to
  // starting index and return nullptr
  // 2.   Bump the starting index (mod number of instances) to start search at a different BPMI each time this function
  // is called

  // BufferPoolManagerInstance::AllocatePage() 是按照 BPI 在 BPM 中的 index 和 总数分配的，
  // 即 BPI 分配的 page_id 通过 BPM 的 hash 运算得到的 BPI 就是调用该函数的。

  mutex_.lock();
  int start_index = alloc_index_;
  alloc_index_ = (alloc_index_ + 1) % static_cast<int>(num_instances_);
  mutex_.unlock();

  for (size_t i = 0; i < num_instances_; ++i) {
    Page *new_page = instaces_[(start_index + i) % num_instances_]->NewPage(page_id);
    if (new_page != nullptr) {
      return new_page;
    }
  }
  return nullptr;
}

bool ParallelBufferPoolManager::DeletePgImp(page_id_t page_id) {
  // Delete page_id from responsible BufferPoolManagerInstance
  BufferPoolManager *buffer_manager = GetBufferPoolManager(page_id);
  return buffer_manager->DeletePage(page_id);
}

void ParallelBufferPoolManager::FlushAllPgsImp() {
  // flush all pages from all BufferPoolManagerInstances
  for (size_t i = 0; i < num_instances_; ++i) {
    instaces_[i]->FlushAllPages();
  }
}

}  // namespace bustub
