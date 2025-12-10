#ifndef STORAGEENGINE_PAGE_MANAGER_H
#define STORAGEENGINE_PAGE_MANAGER_H

#include <memory>
#include <mutex>
#include <unordered_map>

#include "../common/types.h"
#include "../page/page.h"
#include "disk_manager.h"
#include "free_space_map.h"

// PageManager coordinates page operations with disk I/O and free space
// tracking. It provides high-level CRUD operations for tuples and transparently
// handles:
//   - Page allocation and deallocation
//   - Page caching for performance
//   - Free space map synchronization
//   - Forwarding chain resolution
//
// Usage example:
//   DiskManager dm("data.db");
//   FreeSpaceMap fsm("data.fsm");
//   PageManager pm(&dm, &fsm);

class PageManager {
 public:
  // Takes ownership of DiskManager and FreeSpaceMap pointers
  // (caller is responsible for cleanup)
  PageManager(DiskManager* disk_manager, FreeSpaceMap* fsm);

  // Flushes all dirty pages to disk
  ~PageManager();

  TupleId InsertTuple(const char* tuple_data, uint16_t tuple_size);

  ErrorCode GetTuple(TupleId tuple_id, char* buffer,
                     uint16_t buffer_size) const;

  ErrorCode UpdateTuple(TupleId tuple_id, const char* new_data,
                        uint16_t new_size);

  ErrorCode DeleteTuple(TupleId tuple_id);

  ErrorCode FlushAllPages();

  ErrorCode CompactPage(page_id_t page_id);

  size_t GetCacheSize() const;
  void ClearCache();

 private:
  DiskManager* disk_manager_;

  FreeSpaceMap* fsm_;

  std::unordered_map<page_id_t, std::unique_ptr<Page>> page_cache_;

  mutable std::mutex cache_mutex_;

  static constexpr size_t MAX_CACHE_SIZE = 100;

  Page* GetPage(page_id_t page_id);

  ErrorCode FlushPage(page_id_t page_id);

  page_id_t AllocateNewPage();

  void UpdateFSM(page_id_t page_id, Page* page) const;

  TupleId FollowForwardingChainFull(TupleId tuple_id) const;

  void EvictPageIfNeeded();

  page_id_t FindPageWithSpace(uint16_t required_size);

  ErrorCode GetTupleFromSlot(Page* page, slot_id_t slot_id, char* buffer,
                             uint16_t buffer_size) const;

  ErrorCode FlushAllPagesInternal();
};

#endif  // STORAGEENGINE_PAGE_MANAGER_H
