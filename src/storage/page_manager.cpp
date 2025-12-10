#include "../../include/storage/page_manager.h"

#include <cstring>

#include "../../include/common/logger.h"

PageManager::PageManager(DiskManager* disk_manager, FreeSpaceMap* fsm)
    : disk_manager_(disk_manager), fsm_(fsm) {
  if (disk_manager_ == nullptr) {
    LOG_ERROR("PageManager: DiskManager is null");
    throw std::invalid_argument("DiskManager cannot be null");
  }

  if (fsm_ == nullptr) {
    LOG_ERROR("PageManager: FreeSpaceMap is null");
    throw std::invalid_argument("FreeSpaceMap cannot be null");
  }

  // Initialize FSM
  if (!fsm_->Initialize()) {
    LOG_ERROR("PageManager: Failed to initialize FreeSpaceMap");
    throw std::runtime_error("Failed to initialize FreeSpaceMap");
  }

  LOG_INFO("PageManager: Initialized successfully");
}

PageManager::~PageManager() {
  LOG_INFO("PageManager: Flushing all pages before destruction");
  FlushAllPages();
}

TupleId PageManager::InsertTuple(const char* tuple_data, uint16_t tuple_size) {
  std::lock_guard<std::mutex> lock(cache_mutex_);

  if (tuple_data == nullptr) {
    LOG_ERROR("PageManager::InsertTuple: Tuple data is null");
    return {0, INVALID_SLOT_ID};
  }

  if (tuple_size == 0) {
    LOG_ERROR("PageManager::InsertTuple: Tuple size is zero");
    return {0, INVALID_SLOT_ID};
  }

  if (tuple_size > PAGE_SIZE - sizeof(PageHeader) - SLOT_ENTRY_SIZE) {
    LOG_ERROR_STREAM("PageManager::InsertTuple: Tuple too large ("
                     << tuple_size << " bytes)");
    return {0, INVALID_SLOT_ID};
  }

  uint16_t required_space = tuple_size + SLOT_ENTRY_SIZE;

  Page* page = nullptr;
  page_id_t page_id = INVALID_PAGE_ID;
  slot_id_t slot_id = INVALID_SLOT_ID;

  // Try up to 3 times to find a page with space
  // FSM approximation may return pages without enough actual space
  for (int attempt = 0; attempt < 3 && slot_id == INVALID_SLOT_ID; attempt++) {
    page_id = FindPageWithSpace(required_space);

    if (page_id == INVALID_PAGE_ID) {
      page_id = AllocateNewPage();
      if (page_id == INVALID_PAGE_ID) {
        LOG_ERROR("PageManager::InsertTuple: Failed to allocate new page");
        return {0, INVALID_SLOT_ID};
      }
    }

    page = GetPage(page_id);
    if (page == nullptr) {
      LOG_ERROR_STREAM("PageManager::InsertTuple: Failed to get page "
                       << page_id);
      return {0, INVALID_SLOT_ID};
    }

    slot_id = page->InsertTuple(tuple_data, tuple_size);

    if (slot_id == INVALID_SLOT_ID) {
      // Try compacting if the page has fragmentation
      if (page->ShouldCompact()) {
        LOG_INFO_STREAM("PageManager::InsertTuple: Compacting page "
                        << page_id << " to reclaim fragmented space");
        page->CompactPage();

        // Try inserting again after compaction
        slot_id = page->InsertTuple(tuple_data, tuple_size);

        if (slot_id != INVALID_SLOT_ID) {
          LOG_INFO_STREAM(
              "PageManager::InsertTuple: Insert succeeded after compaction");
          break;  // Success after compaction
        }
      }

      // Still failed - mark page as full
      fsm_->UpdatePageFreeSpace(page_id, 0);
      LOG_INFO_STREAM("PageManager::InsertTuple: Page "
                      << page_id << " didn't have enough space, "
                      << "marked as full in FSM");
    }
  }

  if (slot_id == INVALID_SLOT_ID) {
    LOG_ERROR("PageManager::InsertTuple: Failed to insert after 3 attempts");
    return {0, INVALID_SLOT_ID};
  }

  UpdateFSM(page_id, page);

  LOG_INFO_STREAM("PageManager::InsertTuple: Inserted tuple at page "
                  << page_id << ", slot " << slot_id);

  return {page_id, slot_id};
}

ErrorCode PageManager::GetTuple(TupleId tuple_id, char* buffer,
                                uint16_t buffer_size) const {
  std::lock_guard<std::mutex> lock(cache_mutex_);

  if (buffer == nullptr) {
    LOG_ERROR("PageManager::GetTuple: Buffer is null");
    return {-1, "PageManager::GetTuple: Buffer is null"};
  }

  if (buffer_size == 0) {
    LOG_ERROR("PageManager::GetTuple: Buffer size is zero");
    return {-2, "PageManager::GetTuple: Buffer size is zero"};
  }

  TupleId final_tuple_id = FollowForwardingChainFull(tuple_id);

  if (final_tuple_id.page_id == 0 && final_tuple_id.slot_id == 0) {
    LOG_ERROR_STREAM("PageManager::GetTuple: Invalid tuple or circular chain "
                     << "at page " << tuple_id.page_id << ", slot "
                     << tuple_id.slot_id);
    return {-3,
            "PageManager::GetTuple: Invalid tuple ID or circular forwarding "
            "chain"};
  }

  Page* page = const_cast<PageManager*>(this)->GetPage(final_tuple_id.page_id);

  if (page == nullptr) {
    LOG_ERROR_STREAM("PageManager::GetTuple: Failed to get page "
                     << final_tuple_id.page_id);
    return {-4, "PageManager::GetTuple: Failed to get page"};
  }

  return GetTupleFromSlot(page, final_tuple_id.slot_id, buffer, buffer_size);
}

ErrorCode PageManager::UpdateTuple(TupleId tuple_id, const char* new_data,
                                   uint16_t new_size) {
  std::lock_guard<std::mutex> lock(cache_mutex_);

  if (new_data == nullptr) {
    LOG_ERROR("PageManager::UpdateTuple: New data is null");
    return {-1, "PageManager::UpdateTuple: New data is null"};
  }

  if (new_size == 0) {
    LOG_ERROR("PageManager::UpdateTuple: New size is zero");
    return {-2, "PageManager::UpdateTuple: New size is zero"};
  }

  TupleId current_tuple_id = FollowForwardingChainFull(tuple_id);

  if (current_tuple_id.page_id == 0 && current_tuple_id.slot_id == 0) {
    LOG_ERROR_STREAM("PageManager::UpdateTuple: Invalid tuple or circular "
                     << "chain at page " << tuple_id.page_id << ", slot "
                     << tuple_id.slot_id);
    return {-3,
            "PageManager::UpdateTuple: Invalid tuple ID or circular "
            "forwarding chain"};
  }

  Page* current_page = GetPage(current_tuple_id.page_id);

  if (current_page == nullptr) {
    LOG_ERROR_STREAM("PageManager::UpdateTuple: Failed to get page "
                     << current_tuple_id.page_id);
    return {-4, "PageManager::UpdateTuple: Failed to get page"};
  }

  ErrorCode result = current_page->UpdateTupleInPlace(current_tuple_id.slot_id,
                                                      new_data, new_size);

  if (result.code == 0) {
    UpdateFSM(current_tuple_id.page_id, current_page);
    LOG_INFO_STREAM("PageManager::UpdateTuple: Updated tuple in-place at page "
                    << current_tuple_id.page_id << ", slot "
                    << current_tuple_id.slot_id);
    return {0, "PageManager::UpdateTuple: Success (in-place)"};
  }

  LOG_INFO_STREAM("PageManager::UpdateTuple: In-place update failed ("
                  << result.message << "), creating forwarding chain");

  uint16_t required_space = new_size + SLOT_ENTRY_SIZE;
  page_id_t new_page_id = FindPageWithSpace(required_space);

  if (new_page_id == INVALID_PAGE_ID) {
    new_page_id = AllocateNewPage();
    if (new_page_id == INVALID_PAGE_ID) {
      LOG_ERROR("PageManager::UpdateTuple: Failed to allocate new page");
      return {-5, "PageManager::UpdateTuple: Failed to allocate new page"};
    }
  }

  Page* new_page = GetPage(new_page_id);
  if (new_page == nullptr) {
    LOG_ERROR_STREAM("PageManager::UpdateTuple: Failed to get new page "
                     << new_page_id);
    return {-6, "PageManager::UpdateTuple: Failed to get new page"};
  }

  slot_id_t new_slot_id = new_page->InsertTuple(new_data, new_size);

  if (new_slot_id == INVALID_SLOT_ID) {
    LOG_ERROR("PageManager::UpdateTuple: Failed to insert new version");
    return {-7, "PageManager::UpdateTuple: Failed to insert new version"};
  }

  Page* original_page = GetPage(tuple_id.page_id);
  if (original_page == nullptr) {
    LOG_ERROR_STREAM("PageManager::UpdateTuple: Failed to get original page "
                     << tuple_id.page_id);
    return {-8, "PageManager::UpdateTuple: Failed to get original page"};
  }

  ErrorCode forward_result = original_page->MarkSlotForwarded(
      tuple_id.slot_id, new_page_id, new_slot_id);

  if (forward_result.code != 0) {
    LOG_ERROR_STREAM("PageManager::UpdateTuple: Failed to mark slot forwarded ("
                     << forward_result.message << ")");
    return {-9, "PageManager::UpdateTuple: Failed to mark slot forwarded"};
  }

  UpdateFSM(tuple_id.page_id, original_page);
  UpdateFSM(new_page_id, new_page);

  LOG_INFO_STREAM(
      "PageManager::UpdateTuple: Created forwarding chain from page "
      << tuple_id.page_id << ", slot " << tuple_id.slot_id << " to page "
      << new_page_id << ", slot " << new_slot_id);

  return {0, "PageManager::UpdateTuple: Success (forwarding chain created)"};
}

ErrorCode PageManager::DeleteTuple(TupleId tuple_id) {
  std::lock_guard<std::mutex> lock(cache_mutex_);

  TupleId current_tuple_id = FollowForwardingChainFull(tuple_id);

  if (current_tuple_id.page_id == 0 && current_tuple_id.slot_id == 0) {
    LOG_ERROR_STREAM("PageManager::DeleteTuple: Invalid tuple or circular "
                     << "chain at page " << tuple_id.page_id << ", slot "
                     << tuple_id.slot_id);
    return {-1,
            "PageManager::DeleteTuple: Invalid tuple ID or circular "
            "forwarding chain"};
  }

  Page* page = GetPage(current_tuple_id.page_id);

  if (page == nullptr) {
    LOG_ERROR_STREAM("PageManager::DeleteTuple: Failed to get page "
                     << current_tuple_id.page_id);
    return {-2, "PageManager::DeleteTuple: Failed to get page"};
  }

  ErrorCode result = page->DeleteTuple(current_tuple_id.slot_id);

  if (result.code != 0) {
    LOG_ERROR_STREAM("PageManager::DeleteTuple: Failed to delete tuple ("
                     << result.message << ")");
    return result;
  }

  UpdateFSM(current_tuple_id.page_id, page);

  LOG_INFO_STREAM("PageManager::DeleteTuple: Deleted tuple at page "
                  << current_tuple_id.page_id << ", slot "
                  << current_tuple_id.slot_id);

  return {0, "PageManager::DeleteTuple: Success"};
}

ErrorCode PageManager::FlushAllPagesInternal() {
  LOG_INFO_STREAM("PageManager::FlushAllPages: Flushing " << page_cache_.size()
                                                          << " pages");

  for (auto& [page_id, page] : page_cache_) {
    if (page != nullptr && page->IsDirty()) {
      ErrorCode result = FlushPage(page_id);
      if (result.code != 0) {
        LOG_ERROR_STREAM("PageManager::FlushAllPages: Failed to flush page "
                         << page_id << " (" << result.message << ")");
        return result;
      }
    }
  }

  if (!fsm_->Flush()) {
    LOG_ERROR("PageManager::FlushAllPages: Failed to flush FSM");
    return {-1, "PageManager::FlushAllPages: Failed to flush FSM"};
  }

  LOG_INFO("PageManager::FlushAllPages: All pages flushed successfully");
  return {0, "PageManager::FlushAllPages: Success"};
}

ErrorCode PageManager::FlushAllPages() {
  std::lock_guard<std::mutex> lock(cache_mutex_);
  return FlushAllPagesInternal();
}

ErrorCode PageManager::CompactPage(page_id_t page_id) {
  std::lock_guard<std::mutex> lock(cache_mutex_);

  Page* page = GetPage(page_id);
  if (page == nullptr) {
    LOG_ERROR_STREAM("PageManager::CompactPage: Failed to get page "
                     << page_id);
    return {-1, "PageManager::CompactPage: Failed to get page"};
  }

  if (!page->ShouldCompact()) {
    LOG_INFO_STREAM("PageManager::CompactPage: Page "
                    << page_id << " does not need compaction");
    return {0, "PageManager::CompactPage: No compaction needed"};
  }

  page->CompactPage();
  UpdateFSM(page_id, page);

  LOG_INFO_STREAM("PageManager::CompactPage: Successfully compacted page "
                  << page_id);
  return {0, "PageManager::CompactPage: Success"};
}

size_t PageManager::GetCacheSize() const {
  std::lock_guard<std::mutex> lock(cache_mutex_);
  return page_cache_.size();
}

void PageManager::ClearCache() {
  std::lock_guard<std::mutex> lock(cache_mutex_);
  FlushAllPagesInternal();
  page_cache_.clear();
  LOG_INFO("PageManager::ClearCache: Cache cleared");
}

Page* PageManager::GetPage(page_id_t page_id) {
  // Check cache first
  auto it = page_cache_.find(page_id);
  if (it != page_cache_.end()) {
    return it->second.get();
  }

  auto new_page = Page::CreateNew();
  if (new_page == nullptr) {
    LOG_ERROR_STREAM("PageManager::GetPage: Failed to create page object for "
                     << page_id);
    return nullptr;
  }

  try {
    disk_manager_->ReadPage(page_id, new_page->GetRawBuffer());

    if (!new_page->VerifyChecksum()) {
      LOG_ERROR_STREAM(
          "PageManager::GetPage: Checksum verification failed for page "
          << page_id);
      return nullptr;
    }

    EvictPageIfNeeded();

    Page* page_ptr = new_page.get();
    page_cache_[page_id] = std::move(new_page);

    LOG_INFO_STREAM("PageManager::GetPage: Loaded page " << page_id
                                                         << " from disk");
    return page_ptr;

  } catch (const std::exception& e) {
    LOG_ERROR_STREAM("PageManager::GetPage: Exception loading page "
                     << page_id << ": " << e.what());
    return nullptr;
  }
}

ErrorCode PageManager::FlushPage(page_id_t page_id) {
  auto it = page_cache_.find(page_id);
  if (it == page_cache_.end()) {
    return {0, "PageManager::FlushPage: Page not in cache"};
  }

  Page* page = it->second.get();
  if (page == nullptr) {
    return {-1, "PageManager::FlushPage: Page is null"};
  }

  if (!page->IsDirty()) {
    return {0, "PageManager::FlushPage: Page not dirty"};
  }

  try {
    uint32_t checksum = page->ComputeChecksum();
    page->SetChecksum(checksum);

    disk_manager_->WritePage(page_id, page->GetRawBuffer());

    LOG_INFO_STREAM("PageManager::FlushPage: Flushed page " << page_id);
    return {0, "PageManager::FlushPage: Success"};

  } catch (const std::exception& e) {
    LOG_ERROR_STREAM("PageManager::FlushPage: Exception flushing page "
                     << page_id << ": " << e.what());
    return {-2, "PageManager::FlushPage: Exception: " + std::string(e.what())};
  }
}

page_id_t PageManager::AllocateNewPage() {
  page_id_t page_id = disk_manager_->AllocatePage();

  if (page_id == INVALID_PAGE_ID) {
    LOG_ERROR("PageManager::AllocateNewPage: Failed to allocate page ID");
    return INVALID_PAGE_ID;
  }

  auto new_page = Page::CreateNew();
  if (new_page == nullptr) {
    LOG_ERROR("PageManager::AllocateNewPage: Failed to create page object");
    return INVALID_PAGE_ID;
  }

  new_page->SetPageId(page_id);

  EvictPageIfNeeded();

  page_cache_[page_id] = std::move(new_page);

  Page* page = page_cache_[page_id].get();
  UpdateFSM(page_id, page);

  LOG_INFO_STREAM("PageManager::AllocateNewPage: Allocated new page "
                  << page_id);

  return page_id;
}

void PageManager::UpdateFSM(page_id_t page_id, Page* page) const {
  if (page == nullptr) {
    return;
  }

  uint16_t free_space = page->GetFreeEnd() - page->GetFreeStart();
  fsm_->UpdatePageFreeSpace(page_id, free_space);

  LOG_INFO_STREAM("PageManager::UpdateFSM: Updated FSM for page "
                  << page_id << " (free space: " << free_space << " bytes)");
}

TupleId PageManager::FollowForwardingChainFull(TupleId tuple_id) const {
  if (tuple_id.page_id == 0 || tuple_id.slot_id == INVALID_SLOT_ID) {
    LOG_ERROR_STREAM(
        "PageManager::FollowForwardingChainFull: Invalid input TupleId ("
        << tuple_id.page_id << ", " << tuple_id.slot_id << ")");
    return {0, 0};
  }

  Page* page = const_cast<PageManager*>(this)->GetPage(tuple_id.page_id);

  if (page == nullptr) {
    LOG_ERROR_STREAM(
        "PageManager::FollowForwardingChainFull: Failed to get page "
        << tuple_id.page_id);
    return {0, 0};
  }

  if (tuple_id.slot_id >= page->GetSlotCount()) {
    LOG_ERROR_STREAM("PageManager::FollowForwardingChainFull: Slot ID "
                     << tuple_id.slot_id << " out of range (slot_count: "
                     << page->GetSlotCount() << ")");
    return {0, 0};
  }

  TupleId result = page->FollowForwardingChain(tuple_id.slot_id);

  LOG_INFO_STREAM(
      "PageManager::FollowForwardingChainFull: Followed chain from ("
      << tuple_id.page_id << ", " << tuple_id.slot_id << ") to ("
      << result.page_id << ", " << result.slot_id << ")");

  return result;
}

void PageManager::EvictPageIfNeeded() {
  if (page_cache_.size() < MAX_CACHE_SIZE) {
    return;
  }

  for (auto it = page_cache_.begin(); it != page_cache_.end(); ++it) {
    if (it->second != nullptr && !it->second->IsDirty()) {
      LOG_INFO_STREAM("PageManager::EvictPageIfNeeded: Evicting page "
                      << it->first);
      page_cache_.erase(it);
      return;
    }
  }

  auto it = page_cache_.begin();
  if (it != page_cache_.end()) {
    page_id_t page_id = it->first;
    FlushPage(page_id);
    LOG_INFO_STREAM("PageManager::EvictPageIfNeeded: Flushed and evicted page "
                    << page_id);
    page_cache_.erase(it);
  }
}

page_id_t PageManager::FindPageWithSpace(uint16_t required_size) {
  page_id_t page_id = fsm_->FindPageWithSpace(required_size);

  if (page_id != INVALID_PAGE_ID) {
    LOG_INFO_STREAM("PageManager::FindPageWithSpace: Found page "
                    << page_id << " with " << required_size << " bytes needed");
  }

  return page_id;
}

ErrorCode PageManager::GetTupleFromSlot(Page* page, slot_id_t slot_id,
                                        char* buffer,
                                        uint16_t buffer_size) const {
  if (page == nullptr) {
    return {-1, "PageManager::GetTupleFromSlot: Page is null"};
  }

  if (!page->IsSlotValid(slot_id)) {
    LOG_ERROR_STREAM("PageManager::GetTupleFromSlot: Slot " << slot_id
                                                            << " is not valid");
    return {-2, "PageManager::GetTupleFromSlot: Slot is not valid"};
  }

  SlotEntry slot_entry = page->GetSlotEntry(slot_id);

  if (buffer_size < slot_entry.length) {
    LOG_ERROR_STREAM("PageManager::GetTupleFromSlot: Buffer too small ("
                     << buffer_size << " < " << slot_entry.length << ")");
    return {-3, "PageManager::GetTupleFromSlot: Buffer too small"};
  }

  const char* page_data = page->GetRawBuffer();
  std::memcpy(buffer, page_data + slot_entry.offset, slot_entry.length);

  if (buffer_size > slot_entry.length) {
    buffer[slot_entry.length] = '\0';
  }

  LOG_INFO_STREAM("PageManager::GetTupleFromSlot: Retrieved tuple from slot "
                  << slot_id << " (size: " << slot_entry.length << " bytes)");

  return {0, "PageManager::GetTupleFromSlot: Success"};
}
