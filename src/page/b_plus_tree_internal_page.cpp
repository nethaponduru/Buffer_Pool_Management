/**
 * b_plus_tree_internal_page.cpp
 */
#include <iostream>
#include <sstream>

#include "common/exception.h"
#include "page/b_plus_tree_internal_page.h"
#include "common/logger.h"

namespace cmudb {
/*****************************************************************************
 * HELPER METHODS AND UTILITIES
 *****************************************************************************/
/*
 * Init method after creating a new internal page
 * Including set page type, set current size, set page id, set parent id and set
 * max page size
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::Init(page_id_t page_id,
                                          page_id_t parent_id) {
  SetPageType(IndexPageType::INTERNAL_PAGE);
  SetSize(1); // 1 for the first invalid key                            
  SetPageId(page_id);
  SetParentPageId(parent_id);

  // header size is 20 bytes, another 4 bytes for the 1st invalid k/v pair
  // Total record size divded by each record size is max allowed size
  int size = (PAGE_SIZE - sizeof(B_PLUS_TREE_INTERNAL_PAGE_TYPE)) / sizeof(MappingType) - 1; 
  LOG_INFO("Max size of internal page is: %d", size);
  SetMaxSize(size);
}

/*  
 * Helper method to get/set the key associated with input "index"(a.k.a
 * array offset)
 */
INDEX_TEMPLATE_ARGUMENTS
KeyType B_PLUS_TREE_INTERNAL_PAGE_TYPE::KeyAt(int index) const {
  // replace with your own code
  
  return array[index].first;
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::SetKeyAt(int index, const KeyType &key) {
  array[index].first = key;
}

/*
 * Helper method to find and return array index(or offset), so that its value
 * equals to input "value"
 */
INDEX_TEMPLATE_ARGUMENTS
int B_PLUS_TREE_INTERNAL_PAGE_TYPE::ValueIndex(const ValueType &value) const {
  for(int i = 0; i < GetSize(); i++) {
    if (array[i].second == value)
      return i;
  }
  return -1;
}

/*
 * Helper method to get the value associated with input "index"(a.k.a array
 * offset)
 */
INDEX_TEMPLATE_ARGUMENTS
ValueType B_PLUS_TREE_INTERNAL_PAGE_TYPE::ValueAt(int index) const { 
  return array[index].second; 
}

/*****************************************************************************
 * LOOKUP
 *****************************************************************************/
/*
 * Find and return the child pointer(page_id) which points to the child page
 * that contains input "key"
 * Start the search from the second key(the first key should always be invalid)
 */
INDEX_TEMPLATE_ARGUMENTS
ValueType
B_PLUS_TREE_INTERNAL_PAGE_TYPE::Lookup(const KeyType &key,
                                       const KeyComparator &comparator) const {
  // Use binary search
  int foundIndex = 0;
  int e = GetSize();
  for (int b = 1; b < e; b++) {
    if (comparator(array[b].first, key) <= 0) {
      foundIndex = b;
    }
  }
  return array[foundIndex].second;
}

/*****************************************************************************
 * INSERTION
 *****************************************************************************/
/*
 * Populate new root page with old_value + new_key & new_value
 * When the insertion cause overflow from leaf page all the way upto the root
 * page, you should create a new root page and populate its elements.
 * NOTE: This method is only called within InsertIntoParent()(b_plus_tree.cpp)
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::PopulateNewRoot(
    const ValueType &old_value, const KeyType &new_key,
    const ValueType &new_value) {
  // must be a new page
  assert(GetSize() == 1);
  array[0].second = old_value;
  array[1].first = new_key;
  array[1].second = new_value;
  IncreaseSize(1);
}

/*
 * Insert new_key & new_value pair right after the pair with its value ==
 * old_value
 * @return:  new size after insertion
 */
INDEX_TEMPLATE_ARGUMENTS
int B_PLUS_TREE_INTERNAL_PAGE_TYPE::InsertNodeAfter(
    const ValueType &old_value, const KeyType &new_key,
    const ValueType &new_value) {
  
  int preIndex = ValueIndex(old_value);
  assert(preIndex != -1);
  
  // Shift all nodes from preIndex + 1 to right by 1
  for (int i = GetSize(); i >= preIndex + 1; i--) {
    array[i].first = array[i - 1].first;
    array[i].second = array[i - 1].second;
  }

  // insert node after previous old node
  array[preIndex + 1].first = new_key;
  array[preIndex + 1].second = new_value;

  IncreaseSize(1);
  return GetSize();
}

/*****************************************************************************
 * SPLIT
 *****************************************************************************/
/*
 * Remove half of key & value pairs from this page to "recipient" page
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::MoveHalfTo(
    BPlusTreeInternalPage *recipient,
    BufferPoolManager *buffer_pool_manager) {
  // remove from split index to the end
  int splitIndex = (GetSize() + 1) / 2;
  recipient->CopyHalfFrom(&array[splitIndex], GetSize() - splitIndex, buffer_pool_manager);

  // we need to fix the removed nodes
  // the children's parents are different now!
  for (int i = splitIndex; i < GetSize(); i++) {
    auto *page = buffer_pool_manager->FetchPage(ValueAt(i));
    BPlusTreePage *node =
        reinterpret_cast<BPlusTreePage *>(page->GetData());
    node->SetParentPageId(recipient->GetPageId());
    buffer_pool_manager->UnpinPage(page->GetPageId(), true);
  }
  IncreaseSize(-1 * (GetSize() - splitIndex));
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::CopyHalfFrom(
    MappingType *items, int size, BufferPoolManager *buffer_pool_manager) {
    // it is a new page
    for (int i = 0; i < size; i++) {
      array[1 + i] = items[i];
    }
    IncreaseSize(size);
}    

/*****************************************************************************
 * REMOVE
 *****************************************************************************/
/*
 * Remove the key & value pair in internal page according to input index(a.k.a
 * array offset)
 * NOTE: store key&value pair continuously after deletion
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::Remove(int index) {
  // Shuffle the data
  for (int i = index; i < GetSize() - 1; ++i) {
    array[i] = array[i+1];
  }
  IncreaseSize(-1);
}

/*
 * Remove the only key & value pair in internal page and return the value
 * NOTE: only call this method within AdjustRoot()(in b_plus_tree.cpp)
 */
INDEX_TEMPLATE_ARGUMENTS
ValueType B_PLUS_TREE_INTERNAL_PAGE_TYPE::RemoveAndReturnOnlyChild() {
  assert(GetSize() == 2);
  ValueType v = array[1].second;
  array[1].second = INVALID_PAGE_ID;
  IncreaseSize(-1);
  return v;
}
/*****************************************************************************
 * MERGE
 *****************************************************************************/
/*
 * Remove all of key & value pairs from this page to "recipient" page, then
 * update relavent key & value pair in its parent page.
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::MoveAllTo(
  BPlusTreeInternalPage *recipient, int index_in_parent,
  BufferPoolManager *buffer_pool_manager) {

  // remove my Parent's memory of me
  auto *pPage = buffer_pool_manager->FetchPage(GetParentPageId());
  BPlusTreeInternalPage *parentNode =
        reinterpret_cast<BPlusTreeInternalPage *>(pPage->GetData());
  
  // assumption: current page is at the right hand of recipient
  assert(parentNode->ValueAt(index_in_parent) == GetPageId());


  // unpin parent page
  buffer_pool_manager->UnpinPage(GetParentPageId(), true);

  // Note: When copy internal page, the index 0 needs to be populated with the correct key
  // Caller must already fill the index 0 real key

  recipient->CopyAllFrom(&array[0], GetSize(), buffer_pool_manager);  

  // give my children to the new recipient
  for (int i = 0; i < GetSize(); i++) {
    auto *page = buffer_pool_manager->FetchPage(ValueAt(i));
    BPlusTreePage *node =
        reinterpret_cast<BPlusTreePage *>(page->GetData());
    node->SetParentPageId(recipient->GetPageId());
    buffer_pool_manager->UnpinPage(ValueAt(i), true);
  }
  
  SetSize(0); // we are empty
 }

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::CopyAllFrom(
    MappingType *items, int size, BufferPoolManager *buffer_pool_manager) {
  int startIndex = GetSize();
  for (int i = 0; i < size; i++) {
    array[startIndex + i] = items[i];
  }    
  IncreaseSize(size);
}

/*****************************************************************************
 * REDISTRIBUTE
 *****************************************************************************/
/*
 * Remove the first key & value pair from this page to tail of "recipient"
 * page, then update relavent key & value pair in its parent page.
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::MoveFirstToEndOf(
    BPlusTreeInternalPage *recipient,
    BufferPoolManager *buffer_pool_manager) {
  
  recipient->CopyLastFrom(array[0], buffer_pool_manager);

  // set the transfering node's child page's parent to recipient 
  auto *page = buffer_pool_manager->FetchPage(array[0].second);
  BPlusTreePage *node =
      reinterpret_cast<BPlusTreePage *>(page->GetData());
  node->SetParentPageId(recipient->GetPageId());
  buffer_pool_manager->UnpinPage(array[0].second, true);

  Remove(0);

  auto *pPage = buffer_pool_manager->FetchPage(GetParentPageId());
  BPlusTreeInternalPage *parentNode =
        reinterpret_cast<BPlusTreeInternalPage *>(pPage->GetData());

  // if parent's node key is our removed key, change the node key
  int ourPageIdInParentIndex = parentNode->ValueIndex(GetPageId());
  parentNode->SetKeyAt(ourPageIdInParentIndex, KeyAt(0)); // Our new first key. Copy up to parent

  buffer_pool_manager->UnpinPage(GetParentPageId(), true);

  // we do not need to remove, since Remove(0) already took care for us
  //IncreaseSize(-1);
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::CopyLastFrom(
    const MappingType &pair, BufferPoolManager *buffer_pool_manager) {
  array[GetSize()] = pair;
  IncreaseSize(1);
}

/*
 * Remove the last key & value pair from this page to head of "recipient"
 * page, then update relavent key & value pair in its parent page.
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::MoveLastToFrontOf(
    BPlusTreeInternalPage *recipient, int parent_index, // This 'parent_index' should be recipient's index?
    BufferPoolManager *buffer_pool_manager) {

  MappingType pair = array[GetSize() - 1];
  recipient->CopyLastFrom(pair, buffer_pool_manager);

  // Set the moved node's child to the correct parent
  auto *page = buffer_pool_manager->FetchPage(pair.second);
  BPlusTreePage *node =
      reinterpret_cast<BPlusTreePage *>(page->GetData());
  assert(node);
  node->SetParentPageId(recipient->GetPageId());
  
  buffer_pool_manager->UnpinPage(pair.second, true); 

  auto *pPage = buffer_pool_manager->FetchPage(GetParentPageId());
  BPlusTreeInternalPage *parentNode =
        reinterpret_cast<BPlusTreeInternalPage *>(pPage->GetData());

  // parent_index should recipient's position in parent node
  // update the key content
  parentNode->SetKeyAt(parent_index, pair.first); 

  // now we do clean up, including remove the last node
  buffer_pool_manager->UnpinPage(GetParentPageId(), true);
  Remove(GetSize() - 1); // remove last one
  IncreaseSize(-1);
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::CopyFirstFrom(
    const MappingType &pair, int parent_index,
    BufferPoolManager *buffer_pool_manager) {
  
  // move every item to the next, to give space to our new record
  for (int i = GetSize(); i > 0; i--) {
    array[i] = array[i - 1];
  }
  array[0] = pair;
  IncreaseSize(1);
}

INDEX_TEMPLATE_ARGUMENTS
MappingType B_PLUS_TREE_INTERNAL_PAGE_TYPE::PushUpIndex() {
  MappingType pair = array[1];
  array[0].second = pair.second;

  Remove(1);

  return pair;
}

/*****************************************************************************
 * DEBUG
 *****************************************************************************/
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::QueueUpChildren(
    std::queue<BPlusTreePage *> *queue,
    BufferPoolManager *buffer_pool_manager) {
  for (int i = 0; i < GetSize(); i++) {
    auto *page = buffer_pool_manager->FetchPage(array[i].second);
    if (page == nullptr)
      throw Exception(EXCEPTION_TYPE_INDEX,
                      "all page are pinned while printing");
    BPlusTreePage *node =
        reinterpret_cast<BPlusTreePage *>(page->GetData());
    queue->push(node);
  }
}

INDEX_TEMPLATE_ARGUMENTS
std::string B_PLUS_TREE_INTERNAL_PAGE_TYPE::ToString(bool verbose) const {
  if (GetSize() == 0) {
    return "";
  }
  std::ostringstream os;
  if (true) {
    os << "[pageId: " << GetPageId() << " parentId: " << GetParentPageId()
       << "]<" << GetSize() << "> ";
  }

  int entry = verbose ? 0 : 1;
  int end = GetSize();
  bool first = true;
  while (entry < end) {
    if (first) {
      first = false;
    } else {
      os << " ";
    }
    os << std::dec << array[entry].first.ToString();
    if (verbose) {
      os << "(" << array[entry].second << ")";
    }
    ++entry;
  }
  return os.str();
}

// valuetype for internalNode should be page id_t
template class BPlusTreeInternalPage<GenericKey<4>, page_id_t,
                                           GenericComparator<4>>;
template class BPlusTreeInternalPage<GenericKey<8>, page_id_t,
                                           GenericComparator<8>>;
template class BPlusTreeInternalPage<GenericKey<16>, page_id_t,
                                           GenericComparator<16>>;
template class BPlusTreeInternalPage<GenericKey<32>, page_id_t,
                                           GenericComparator<32>>;
template class BPlusTreeInternalPage<GenericKey<64>, page_id_t,
                                           GenericComparator<64>>;
} // namespace cmudb
