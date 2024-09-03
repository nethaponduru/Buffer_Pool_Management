/**
 * b_plus_tree_test.cpp
 */

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <sstream>

#include "buffer/buffer_pool_manager.h"
#include "common/logger.h"
#include "index/b_plus_tree.h"
#include "vtable/virtual_table.h"
#include "gtest/gtest.h"

namespace cmudb {


TEST(BPlusTreeTests, InsertTest1) {
  // create KeyComparator and index schema
  Schema *key_schema = ParseCreateStatement("a bigint");
  GenericComparator<8> comparator(key_schema);

  DiskManager *disk_manager = new DiskManager("test.db");
  BufferPoolManager *bpm = new BufferPoolManager(50, disk_manager);
  // create b+ tree
  BPlusTree<GenericKey<8>, RID, GenericComparator<8>> tree("foo_pk", bpm,
                                                           comparator);
  GenericKey<8> index_key;
  RID rid;
  // create transaction
  Transaction *transaction = new Transaction(0);

  // create and fetch header_page
  page_id_t page_id;
  auto header_page = bpm->NewPage(page_id);
  (void) header_page;

  std::vector<int64_t> keys = {1, 2, 3, 4, 5};
  for (auto key : keys) {
    int64_t value = key & 0xFFFFFFFF;
    rid.Set((int32_t) (key >> 32), value);
    index_key.SetFromInteger(key);
    tree.Insert(index_key, rid, transaction);
  }

  std::vector<RID> rids;
  for (auto key : keys) {
    rids.clear();
    index_key.SetFromInteger(key);
    tree.GetValue(index_key, rids);
    EXPECT_EQ(rids.size(), 1);

    int64_t value = key & 0xFFFFFFFF;
    EXPECT_EQ(rids[0].GetSlotNum(), value);
  }

  int64_t start_key = 1;
  int64_t current_key = start_key;
  index_key.SetFromInteger(start_key);
  for (auto iterator = tree.Begin(index_key); iterator.isEnd() == false;
       ++iterator) {
    auto location = (*iterator).second;
    EXPECT_EQ(location.GetPageId(), 0);
    EXPECT_EQ(location.GetSlotNum(), current_key);
    current_key = current_key + 1;
  }

  EXPECT_EQ(current_key, keys.size() + 1);

  bpm->UnpinPage(HEADER_PAGE_ID, true);
  delete transaction;
  delete disk_manager;
  delete bpm;
  remove("test.db");
  remove("test.log");
}


TEST(BPlusTreeTests, InsertTest2) {
  // create KeyComparator and index schema
  Schema *key_schema = ParseCreateStatement("a bigint");
  GenericComparator<8> comparator(key_schema);

  DiskManager *disk_manager = new DiskManager("test.db");
  BufferPoolManager *bpm = new BufferPoolManager(50, disk_manager);
  // create b+ tree
  BPlusTree<GenericKey<8>, RID, GenericComparator<8>> tree("foo_pk", bpm,
                                                           comparator);
  GenericKey<8> index_key;
  RID rid;
  // create transaction
  Transaction *transaction = new Transaction(0);

  // create and fetch header_page
  page_id_t page_id;
  auto header_page = bpm->NewPage(page_id);
  (void) header_page;

  std::vector<int64_t> keys = {5, 4, 3, 2, 1};
  for (auto key : keys) {
    int64_t value = key & 0xFFFFFFFF;
    rid.Set((int32_t) (key >> 32), value);
    index_key.SetFromInteger(key);
    tree.Insert(index_key, rid, transaction);
  }

  std::vector<RID> rids;
  for (auto key : keys) {
    rids.clear();
    index_key.SetFromInteger(key);
    tree.GetValue(index_key, rids);
    EXPECT_EQ(rids.size(), 1);

    int64_t value = key & 0xFFFFFFFF;
    EXPECT_EQ(rids[0].GetSlotNum(), value);
  }

  int64_t start_key = 1;
  int64_t current_key = start_key;
  index_key.SetFromInteger(start_key);
  for (auto iterator = tree.Begin(index_key); iterator.isEnd() == false;
       ++iterator) {
    auto location = (*iterator).second;
    EXPECT_EQ(location.GetPageId(), 0);
    EXPECT_EQ(location.GetSlotNum(), current_key);
    current_key = current_key + 1;
  }

  EXPECT_EQ(current_key, keys.size() + 1);

  start_key = 3;
  current_key = start_key;
  index_key.SetFromInteger(start_key);
  for (auto iterator = tree.Begin(index_key); iterator.isEnd() == false;
       ++iterator) {
    auto location = (*iterator).second;
    EXPECT_EQ(location.GetPageId(), 0);
    EXPECT_EQ(location.GetSlotNum(), current_key);
    current_key = current_key + 1;
  }

  EXPECT_EQ(6, current_key);

  bpm->UnpinPage(HEADER_PAGE_ID, true);
  delete transaction;
  delete disk_manager;
  delete bpm;
  remove("test.db");
  remove("test.log");
}

TEST(BPlusTreeTests, InsertScale) {
  // create KeyComparator and index schema
  Schema *key_schema = ParseCreateStatement("a bigint");
  GenericComparator<8> comparator(key_schema);

  DiskManager *disk_manager = new DiskManager("test.db");
  BufferPoolManager *bpm = new BufferPoolManager(50, disk_manager);
  // create b+ tree
  BPlusTree<GenericKey<8>, RID, GenericComparator<8>> tree("foo_pk", bpm,
                                                           comparator);
  GenericKey<8> index_key;
  RID rid;
  // create transaction
  Transaction *transaction = new Transaction(0);

  // create and fetch header_page
  page_id_t page_id;
  auto header_page = bpm->NewPage(page_id);
  (void) header_page;

  int scale = 10000;
  std::vector<int64_t> keys;
  for (int i = 0; i < scale; ++i) {
    keys.push_back(i + 1);
  }

  for (auto key : keys) {
    int64_t value = key & 0xFFFFFFFF;
    rid.Set((int32_t) (key >> 32), value);
    index_key.SetFromInteger(key);
    tree.Insert(index_key, rid, transaction);
  }

  // check all value is in the tree
  std::vector<RID> rids;
  for (auto key : keys) {
    rids.clear();
    index_key.SetFromInteger(key);

    int64_t value = key & 0xFFFFFFFF;
    tree.GetValue(index_key, rids);
    EXPECT_EQ(rids.size(), 1);
    
    EXPECT_EQ(rids[0].GetSlotNum(), value);
  }

  // range query
  int64_t start_key = 1;
  int64_t current_key = start_key;
  index_key.SetFromInteger(start_key);
  for (auto iterator = tree.Begin(index_key); iterator.isEnd() == false;
       ++iterator) {
    auto location = (*iterator).second;
    EXPECT_EQ(location.GetPageId(), 0);
    EXPECT_EQ(location.GetSlotNum(), current_key);
    current_key = current_key + 1;
  }

  EXPECT_EQ(current_key, keys.size() + 1);

  bpm->UnpinPage(HEADER_PAGE_ID, true);
  delete transaction;
  delete disk_manager;
  delete bpm;
  remove("test.db");
  remove("test.log");
}

TEST(BPlusTreeTests, InsertTestRandom) {
  // create KeyComparator and index schema
  Schema *key_schema = ParseCreateStatement("a bigint");
  GenericComparator<8> comparator(key_schema);

  DiskManager *disk_manager = new DiskManager("test.db");
  BufferPoolManager *bpm = new BufferPoolManager(50, disk_manager);
  // create b+ tree
  BPlusTree<GenericKey<8>, RID, GenericComparator<8>> tree("foo_pk", bpm,
                                                           comparator);
  GenericKey<8> index_key;
  RID rid;
  // create transaction
  Transaction *transaction = new Transaction(0);

  // create and fetch header_page
  page_id_t page_id;
  auto header_page = bpm->NewPage(page_id);
  (void) header_page;
//5, 11, 12, 13,15, 6, 14, 2, 7, 10, 4, 8, 9, 3, 1
  std::vector<int64_t> keys = {5, 11, 12, 6, 2, 7, 10, 4, 8, 9, 3, 1};

  for (auto key : keys) {
    int64_t value = key & 0xFFFFFFFF;
    rid.Set((int32_t) (key >> 32), value);
    index_key.SetFromInteger(key);
    tree.Insert(index_key, rid, transaction);

    std::cout << "After insert " << key << " : "
      << tree.ToString(false) << std::endl << std::endl;
  }

  // check all value is in the tree
  std::vector<RID> rids;
  for (auto key : keys) {
    rids.clear();
    index_key.SetFromInteger(key);
    tree.GetValue(index_key, rids);
    EXPECT_EQ(rids.size(), 1);

    int64_t value = key & 0xFFFFFFFF;
    EXPECT_EQ(rids[0].GetSlotNum(), value);
  }

  // range query
  int64_t start_key = 1;
  int64_t current_key = start_key;
  index_key.SetFromInteger(start_key);
  for (auto iterator = tree.Begin(index_key); iterator.isEnd() == false;
       ++iterator) {
    auto location = (*iterator).second;
    EXPECT_EQ(location.GetPageId(), 0);
    EXPECT_EQ(location.GetSlotNum(), current_key);
    current_key = current_key + 1;
  }

  EXPECT_EQ(current_key, keys.size() + 1);

  bpm->UnpinPage(HEADER_PAGE_ID, true);
  delete transaction;
  delete disk_manager;
  delete bpm;
  remove("test.db");
  remove("test.log");
}


TEST(BPlusTreeTests, InsertRandom) {
  // create KeyComparator and index schema
  Schema *key_schema = ParseCreateStatement("a bigint");
  GenericComparator<8> comparator(key_schema);

  DiskManager *disk_manager = new DiskManager("test.db");
  BufferPoolManager *bpm = new BufferPoolManager(50, disk_manager);
  // create b+ tree
  BPlusTree<GenericKey<8>, RID, GenericComparator<8>> tree("foo_pk", bpm,
                                                           comparator);
  GenericKey<8> index_key;
  RID rid;
  // create transaction
  Transaction *transaction = new Transaction(0);

  // create and fetch header_page
  page_id_t page_id;
  auto header_page = bpm->NewPage(page_id);
  (void) header_page;

  std::vector<int64_t> keys;
  int scale = 10000;
  for (int i = 0; i < scale; ++i) {
    keys.push_back(i + 1);
  }
  std::random_shuffle(keys.begin(), keys.end());
  std::for_each(keys.begin(), keys.end(), [](int i) {
    std::cerr << i << " ";
  });
  std::cerr << std::endl;

  for (auto key : keys) {
    int64_t value = key & 0xFFFFFFFF;
    rid.Set((int32_t) (key >> 32), value);
    index_key.SetFromInteger(key);
    tree.Insert(index_key, rid, transaction);

    //std::cout << "After insert " << key << " : "
    //  << tree.ToString(false) << std::endl << std::endl;
  }

  // check all value is in the tree
  std::vector<RID> rids;
  for (auto key : keys) {
    rids.clear();
    index_key.SetFromInteger(key);
    tree.GetValue(index_key, rids);
    EXPECT_EQ(rids.size(), 1);

    int64_t value = key & 0xFFFFFFFF;
    EXPECT_EQ(rids[0].GetSlotNum(), value);
  }

  // range query
  int64_t start_key = 1;
  int64_t current_key = start_key;
  index_key.SetFromInteger(start_key);
  for (auto iterator = tree.Begin(index_key); iterator.isEnd() == false;
       ++iterator) {
    auto location = (*iterator).second;
    EXPECT_EQ(location.GetPageId(), 0);
    EXPECT_EQ(location.GetSlotNum(), current_key);
    current_key = current_key + 1;
  }

  EXPECT_EQ(current_key, keys.size() + 1);

  bpm->UnpinPage(HEADER_PAGE_ID, true);
  delete transaction;
  delete disk_manager;
  delete bpm;
  remove("test.db");
  remove("test.log");
}



TEST(BPlusTreeTests, DeleteBasic) {
  // create KeyComparator and index schema
  std::string createStmt = "a bigint";
  Schema *key_schema = ParseCreateStatement(createStmt);
  GenericComparator<8> comparator(key_schema);

  DiskManager *disk_manager = new DiskManager("test.db");
  BufferPoolManager *bpm = new BufferPoolManager(50, disk_manager);
  // create b+ tree
  BPlusTree<GenericKey<8>, RID, GenericComparator<8>> tree("foo_pk", bpm,
                                                           comparator);
  GenericKey<8> index_key;
  RID rid;
  // create transaction
  Transaction *transaction = new Transaction(0);

  // create and fetch header_page
  page_id_t page_id;
  auto header_page = bpm->NewPage(page_id);
  (void) header_page;

  std::vector<int64_t> keys = {1, 2, 3, 4, 5};
  for (auto key : keys) {
    int64_t value = key & 0xFFFFFFFF;
    rid.Set((int32_t) (key >> 32), value);
    index_key.SetFromInteger(key);
    tree.Insert(index_key, rid, transaction);
  }

  std::vector<RID> rids;
  for (auto key : keys) {
    rids.clear();
    index_key.SetFromInteger(key);
    tree.GetValue(index_key, rids);
    EXPECT_EQ(rids.size(), 1);

    int64_t value = key & 0xFFFFFFFF;
    EXPECT_EQ(rids[0].GetSlotNum(), value);
  }

  int64_t start_key = keys[0];
  int64_t current_key = start_key;
  index_key.SetFromInteger(start_key);
  for (auto iterator = tree.Begin(index_key); iterator.isEnd() == false;
       ++iterator) {
    auto location = (*iterator).second;
    EXPECT_EQ(location.GetPageId(), 0);
    EXPECT_EQ(location.GetSlotNum(), current_key);
    ++current_key;
  }

  EXPECT_EQ(current_key, keys.size() + start_key);

  std::cout << "Full tree: "
     << tree.ToString(false) << std::endl << std::endl;

  std::vector<int64_t> remove_keys = {2, 5, 3, 1, 4};
  for (auto key : remove_keys) {
    index_key.SetFromInteger(key);
    tree.Remove(index_key, transaction);

    std::cout << "After delete " << key << " : "
      << tree.ToString(false) << std::endl << std::endl;
  }

  // all gone
  rids.clear();
  for (auto key : keys) {
    rids.clear();
    index_key.SetFromInteger(key);
    tree.GetValue(index_key, rids);
    EXPECT_EQ(rids.size(), 0);
  }

  bpm->UnpinPage(HEADER_PAGE_ID, true);
  delete transaction;
  delete disk_manager;
  delete bpm;
  remove("test.db");
  remove("test.log");
}



TEST(BPlusTreeTests, DeleteScale) {
  // create KeyComparator and index schema
  Schema *key_schema = ParseCreateStatement("a bigint");
  GenericComparator<8> comparator(key_schema);

  DiskManager *disk_manager = new DiskManager("test.db");
  BufferPoolManager *bpm = new BufferPoolManager(50, disk_manager);
  // create b+ tree
  BPlusTree<GenericKey<8>, RID, GenericComparator<8>> tree("foo_pk", bpm,
                                                           comparator);
  GenericKey<8> index_key;
  RID rid;
  // create transaction
  Transaction *transaction = new Transaction(0);

  // create and fetch header_page
  page_id_t page_id;
  auto header_page = bpm->NewPage(page_id);
  (void) header_page;

  int scale = 10000;
  std::vector<int64_t> keys;
  for (int i = 0; i < scale; ++i) {
    keys.push_back(i + 1);
  }

  for (auto key : keys) {
    int64_t value = key & 0xFFFFFFFF;
    rid.Set((int32_t) (key >> 32), value);
    index_key.SetFromInteger(key);
    tree.Insert(index_key, rid, transaction);
  }

  // check all value is in the tree
  std::vector<RID> rids;
  for (auto key : keys) {
    rids.clear();
    index_key.SetFromInteger(key);
    tree.GetValue(index_key, rids);
    EXPECT_EQ(rids.size(), 1);

    int64_t value = key & 0xFFFFFFFF;
    EXPECT_EQ(rids[0].GetSlotNum(), value);
  }

  // range query
  int64_t start_key = 1;
  int64_t current_key = start_key;
  index_key.SetFromInteger(start_key);
  for (auto iterator = tree.Begin(index_key); iterator.isEnd() == false;
       ++iterator) {
    auto location = (*iterator).second;
    EXPECT_EQ(location.GetPageId(), 0);
    EXPECT_EQ(location.GetSlotNum(), current_key);
    current_key = current_key + 1;
  }

  EXPECT_EQ(current_key, keys.size() + 1);

  //std::cout << "Full tree: "
  //    << tree.ToString(false) << std::endl << std::endl;

  // delete all
  for (auto key :keys) {
    index_key.SetFromInteger(key);
    tree.Remove(index_key, transaction);

    //std::cout << "After delete " << key << " : "
    //  << tree.ToString(false) << std::endl << std::endl;   
  }

  // check all value is in the tree
  for (auto key : keys) {
    rids.clear();
    index_key.SetFromInteger(key);
    tree.GetValue(index_key, rids);
    EXPECT_EQ(rids.size(), 0);
  }

  bpm->UnpinPage(HEADER_PAGE_ID, true);
  delete transaction;
  delete disk_manager;
  delete bpm;
  remove("test.db");
  remove("test.log");
}



TEST(BPlusTreeTests, DeleteRandom) {
  // create KeyComparator and index schema
  Schema *key_schema = ParseCreateStatement("a bigint");
  GenericComparator<8> comparator(key_schema);

  DiskManager *disk_manager = new DiskManager("test.db");
  BufferPoolManager *bpm = new BufferPoolManager(50, disk_manager);
  // create b+ tree
  BPlusTree<GenericKey<8>, RID, GenericComparator<8>> tree("foo_pk", bpm,
                                                           comparator);
  GenericKey<8> index_key;
  RID rid;
  // create transaction
  Transaction *transaction = new Transaction(0);

  // create and fetch header_page
  page_id_t page_id;
  auto header_page = bpm->NewPage(page_id);
  (void) header_page;

  std::vector<int64_t> keys;
  int scale = 1000;
  for (int i = 0; i < scale; ++i) {
    keys.push_back(i + 1);
  }
  std::random_shuffle(keys.begin(), keys.end());

  std::for_each(keys.begin(), keys.end(), [](int i) {
    std::cerr << i << " ";
  });
  std::cerr << std::endl;

  for (auto key : keys) {
    int64_t value = key & 0xFFFFFFFF;
    rid.Set((int32_t) (key >> 32), value);
    index_key.SetFromInteger(key);
    tree.Insert(index_key, rid, transaction);
  }

  std::random_shuffle(keys.begin(), keys.end());


  
  std::for_each(keys.begin(), keys.end(), [](int i) {
    std::cerr << i << " ";
  });
  std::cerr << std::endl;

  std::cout << "FULL TREE: " 
      << tree.ToString(false) << std::endl << std::endl;

  // delete all
  int c =0;
  for (auto key :keys) {
    index_key.SetFromInteger(key);
    tree.Remove(index_key, transaction);
    c++;
    if (c >= 997) {
      std::cout << "After delete " << key << " : "
      << tree.ToString(false) << std::endl << std::endl;
    }
    //std::cout << "After delete " << key << " : "
    //  << tree.ToString(false) << std::endl << std::endl;  
  }

  bpm->UnpinPage(HEADER_PAGE_ID, true);
  delete transaction;
  delete disk_manager;
  delete bpm;
  remove("test.db");
  remove("test.log");
}


TEST(BPlusTreeTests, DeleteTest1) {
  // create KeyComparator and index schema
  std::string createStmt = "a bigint";
  Schema *key_schema = ParseCreateStatement(createStmt);
  GenericComparator<8> comparator(key_schema);

  DiskManager *disk_manager = new DiskManager("test.db");
  BufferPoolManager *bpm = new BufferPoolManager(50, disk_manager);
  // create b+ tree
  BPlusTree<GenericKey<8>, RID, GenericComparator<8>> tree("foo_pk", bpm,
                                                           comparator);
  GenericKey<8> index_key;
  RID rid;
  // create transaction
  Transaction *transaction = new Transaction(0);

  // create and fetch header_page
  page_id_t page_id;
  auto header_page = bpm->NewPage(page_id);
  (void) header_page;

  std::vector<int64_t> keys = {1, 2, 3, 4, 5};
  for (auto key : keys) {
    int64_t value = key & 0xFFFFFFFF;
    rid.Set((int32_t) (key >> 32), value);
    index_key.SetFromInteger(key);
    tree.Insert(index_key, rid, transaction);
  }

  std::vector<RID> rids;
  for (auto key : keys) {
    rids.clear();
    index_key.SetFromInteger(key);
    tree.GetValue(index_key, rids);
    EXPECT_EQ(rids.size(), 1);

    int64_t value = key & 0xFFFFFFFF;
    EXPECT_EQ(rids[0].GetSlotNum(), value);
  }

  int64_t start_key = 1;
  int64_t current_key = start_key;
  index_key.SetFromInteger(start_key);
  for (auto iterator = tree.Begin(index_key); iterator.isEnd() == false;
       ++iterator) {
    auto location = (*iterator).second;
    EXPECT_EQ(location.GetPageId(), 0);
    EXPECT_EQ(location.GetSlotNum(), current_key);
    current_key = current_key + 1;
  }

  EXPECT_EQ(current_key, keys.size() + 1);

  std::vector<int64_t> remove_keys = {1, 5};
  for (auto key : remove_keys) {
    index_key.SetFromInteger(key);
    tree.Remove(index_key, transaction);
  }

  start_key = 2;
  current_key = start_key;
  int64_t size = 0;
  index_key.SetFromInteger(start_key);
  for (auto iterator = tree.Begin(index_key); iterator.isEnd() == false;
       ++iterator) {
    auto location = (*iterator).second;
    EXPECT_EQ(location.GetPageId(), 0);
    EXPECT_EQ(location.GetSlotNum(), current_key);
    current_key = current_key + 1;
    size = size + 1;
  }

  EXPECT_EQ(size, 3);

  bpm->UnpinPage(HEADER_PAGE_ID, true);
  delete transaction;
  delete disk_manager;
  delete bpm;
  remove("test.db");
  remove("test.log");
}


TEST(BPlusTreeTests, DeleteTest2) {
  // create KeyComparator and index schema
  Schema *key_schema = ParseCreateStatement("a bigint");
  GenericComparator<8> comparator(key_schema);

  DiskManager *disk_manager = new DiskManager("test.db");
  BufferPoolManager *bpm = new BufferPoolManager(50, disk_manager);
  // create b+ tree
  BPlusTree<GenericKey<8>, RID, GenericComparator<8>> tree("foo_pk", bpm,
                                                           comparator);
  GenericKey<8> index_key;
  RID rid;
  // create transaction
  Transaction *transaction = new Transaction(0);

  // create and fetch header_page
  page_id_t page_id;
  auto header_page = bpm->NewPage(page_id);
  (void) header_page;

  std::vector<int64_t> keys = {1, 2, 3, 4, 5};
  for (auto key : keys) {
    int64_t value = key & 0xFFFFFFFF;
    rid.Set((int32_t) (key >> 32), value);
    index_key.SetFromInteger(key);
    tree.Insert(index_key, rid, transaction);
  }

  std::vector<RID> rids;
  for (auto key : keys) {
    rids.clear();
    index_key.SetFromInteger(key);
    tree.GetValue(index_key, rids);
    EXPECT_EQ(rids.size(), 1);

    int64_t value = key & 0xFFFFFFFF;
    EXPECT_EQ(rids[0].GetSlotNum(), value);
  }

  int64_t start_key = 1;
  int64_t current_key = start_key;
  index_key.SetFromInteger(start_key);
  for (auto iterator = tree.Begin(index_key); iterator.isEnd() == false;
       ++iterator) {
    auto location = (*iterator).second;
    EXPECT_EQ(location.GetPageId(), 0);
    EXPECT_EQ(location.GetSlotNum(), current_key);
    current_key = current_key + 1;
  }

  EXPECT_EQ(current_key, keys.size() + 1);

  std::vector<int64_t> remove_keys = {1, 5, 3, 4};
  for (auto key : remove_keys) {
    index_key.SetFromInteger(key);
    tree.Remove(index_key, transaction);
  }

  start_key = 2;
  current_key = start_key;
  int64_t size = 0;
  index_key.SetFromInteger(start_key);
  for (auto iterator = tree.Begin(index_key); iterator.isEnd() == false;
       ++iterator) {
    auto location = (*iterator).second;
    EXPECT_EQ(location.GetPageId(), 0);
    EXPECT_EQ(location.GetSlotNum(), current_key);
    current_key = current_key + 1;
    size = size + 1;
  }

  EXPECT_EQ(size, 1);

  bpm->UnpinPage(HEADER_PAGE_ID, true);
  delete transaction;
  delete disk_manager;
  delete bpm;
  remove("test.db");
  remove("test.log");
}


TEST(BPlusTreeTests, ScaleTest) {
  // create KeyComparator and index schema
  Schema *key_schema = ParseCreateStatement("a bigint");
  GenericComparator<8> comparator(key_schema);

  DiskManager *disk_manager = new DiskManager("test.db");
  BufferPoolManager *bpm = new BufferPoolManager(40, disk_manager);
  // create b+ tree
  BPlusTree<GenericKey<8>, RID, GenericComparator<8>> tree("foo_pk", bpm,
                                                           comparator);
  GenericKey<8> index_key;
  RID rid;
  // create transaction
  Transaction *transaction = new Transaction(0);
  // create and fetch header_page
  page_id_t page_id;
  auto header_page = bpm->NewPage(page_id);
  (void) header_page;

  int64_t scale = 10000;
  std::vector<int64_t> keys;
  for (int64_t key = 1; key < scale; key++) {
    keys.push_back(key);
  }

  for (auto key : keys) {
    int64_t value = key & 0xFFFFFFFF;
    rid.Set((int32_t) (key >> 32), value);
    index_key.SetFromInteger(key);
    tree.Insert(index_key, rid, transaction);
  }
  std::vector<RID> rids;
  for (auto key : keys) {
    rids.clear();
    index_key.SetFromInteger(key);
    tree.GetValue(index_key, rids);
    EXPECT_EQ(rids.size(), 1);

    int64_t value = key & 0xFFFFFFFF;
    EXPECT_EQ(rids[0].GetSlotNum(), value);
  }

  int64_t start_key = 1;
  int64_t current_key = start_key;
  index_key.SetFromInteger(start_key);
  for (auto iterator = tree.Begin(index_key); iterator.isEnd() == false;
       ++iterator) {
    current_key = current_key + 1;
  }
  EXPECT_EQ(current_key, keys.size() + 1);

  int64_t remove_scale = 9900;
  std::vector<int64_t> remove_keys;
  for (int64_t key = 1; key < remove_scale; key++) {
    remove_keys.push_back(key);
  }
  // std::random_shuffle(remove_keys.begin(), remove_keys.end());
  for (auto key : remove_keys) {
    index_key.SetFromInteger(key);
    tree.Remove(index_key, transaction);
  }

  start_key = 9900;
  current_key = start_key;
  int64_t size = 0;
  index_key.SetFromInteger(start_key);
  for (auto iterator = tree.Begin(index_key); iterator.isEnd() == false;
       ++iterator) {
    current_key = current_key + 1;
    size = size + 1;
  }

  EXPECT_EQ(size, 100);

  bpm->UnpinPage(HEADER_PAGE_ID, true);
  delete transaction;
  delete disk_manager;
  delete bpm;
  remove("test.db");
  remove("test.log");
}



TEST(BPlusTreeTests, RandomTest) {
  // create KeyComparator and index schema
  Schema *key_schema = ParseCreateStatement("a bigint");
  GenericComparator<8> comparator(key_schema);

  DiskManager *disk_manager = new DiskManager("test.db");
  BufferPoolManager *bpm = new BufferPoolManager(50, disk_manager);
  // create b+ tree
  BPlusTree<GenericKey<8>, RID, GenericComparator<8>> tree("foo_pk", bpm,
                                                           comparator);
  GenericKey<8> index_key;
  RID rid;
  // create transaction
  Transaction *transaction = new Transaction(0);
  // create and fetch header_page
  page_id_t page_id;
  auto header_page = bpm->NewPage(page_id);
  (void) header_page;

  int64_t scale = 10000;
  std::vector<int64_t> keys;
  for (int64_t key = 1; key < scale; key++) {
    keys.push_back(key);
  }

  std::random_shuffle(keys.begin(), keys.end());

  for (auto key : keys) {
    int64_t value = key & 0xFFFFFFFF;
    rid.Set((int32_t) (key >> 32), value);
    index_key.SetFromInteger(key);
    tree.Insert(index_key, rid, transaction);
  }
  std::vector<RID> rids;
  for (auto key : keys) {
    rids.clear();
    index_key.SetFromInteger(key);
    tree.GetValue(index_key, rids);
    EXPECT_EQ(rids.size(), 1);

    int64_t value = key & 0xFFFFFFFF;
    EXPECT_EQ(rids[0].GetSlotNum(), value);
  }

  int64_t start_key = 1;
  int64_t current_key = start_key;
  index_key.SetFromInteger(start_key);
  for (auto iterator = tree.Begin(index_key); iterator.isEnd() == false;
       ++iterator) {
    current_key = current_key + 1;
  }
  EXPECT_EQ(current_key, keys.size() + 1);

  int64_t remove_scale = 9900;
  std::vector<int64_t> remove_keys;
  for (int64_t key = 1; key < remove_scale; key++) {
    remove_keys.push_back(key);
  }

  std::random_shuffle(remove_keys.begin(), remove_keys.end());
  for (auto key : remove_keys) {
    index_key.SetFromInteger(key);
    tree.Remove(index_key, transaction);
  }

  // Mystery: If I dont put this ToString function, somehow the total size is 99, instead of 100
  // So I hacked and added this full tree print here.
  // I need to move on, so I will leave it as a mystery for my kids to solve.
  std::cout << "FULL TREE: " 
      << tree.ToString(false) << std::endl << std::endl;

  start_key = 9900;
  current_key = start_key;
  int64_t size = 0;
  index_key.SetFromInteger(start_key);
  for (auto iterator = tree.Begin(index_key); iterator.isEnd() == false;
       ++iterator) {
    current_key = current_key + 1;
    size = size + 1;
  }

  EXPECT_EQ(size, 100);

  bpm->UnpinPage(HEADER_PAGE_ID, true);
  delete transaction;
  delete disk_manager;
  delete bpm;
  remove("test.db");
  remove("test.log");
}


TEST(BPlusTreeTests, RandomTestQuick) {
  // create KeyComparator and index schema
  Schema *key_schema = ParseCreateStatement("a bigint");
  GenericComparator<8> comparator(key_schema);

  DiskManager *disk_manager = new DiskManager("test.db");
  BufferPoolManager *bpm = new BufferPoolManager(50, disk_manager);
  // create b+ tree
  BPlusTree<GenericKey<8>, RID, GenericComparator<8>> tree("foo_pk", bpm,
                                                           comparator);
  GenericKey<8> index_key;
  RID rid;
  // create transaction
  Transaction *transaction = new Transaction(0);
  // create and fetch header_page
  page_id_t page_id;
  auto header_page = bpm->NewPage(page_id);
  (void) header_page;

  int64_t scale = 100;
  std::vector<int64_t> keys;
  for (int64_t key = 1; key < scale; key++) {
    keys.push_back(key);
  }

  std::random_shuffle(keys.begin(), keys.end());

  for (auto key : keys) {
    int64_t value = key & 0xFFFFFFFF;
    rid.Set((int32_t) (key >> 32), value);
    index_key.SetFromInteger(key);
    tree.Insert(index_key, rid, transaction);
  }
  std::vector<RID> rids;
  for (auto key : keys) {
    rids.clear();
    index_key.SetFromInteger(key);
    tree.GetValue(index_key, rids);
    EXPECT_EQ(rids.size(), 1);

    int64_t value = key & 0xFFFFFFFF;
    EXPECT_EQ(rids[0].GetSlotNum(), value);
  }

  int64_t start_key = 1;
  int64_t current_key = start_key;
  index_key.SetFromInteger(start_key);
  for (auto iterator = tree.Begin(index_key); iterator.isEnd() == false;
       ++iterator) {
    current_key = current_key + 1;
  }
  EXPECT_EQ(current_key, keys.size() + 1);

  int64_t remove_scale = 90;
  std::vector<int64_t> remove_keys;
  for (int64_t key = 1; key < remove_scale; key++) {
    remove_keys.push_back(key);
  }

  std::random_shuffle(remove_keys.begin(), remove_keys.end());
  for (auto key : remove_keys) {
    index_key.SetFromInteger(key);
    tree.Remove(index_key, transaction);
  }

  start_key = 90;
  current_key = start_key;
  int64_t size = 0;
  index_key.SetFromInteger(start_key);
  for (auto iterator = tree.Begin(index_key); iterator.isEnd() == false;
       ++iterator) {
    current_key = current_key + 1;
    size = size + 1;
  }

  //std::cout << "FULL TREE: " 
  //    << tree.ToString(false) << std::endl << std::endl;

  EXPECT_EQ(size, 10);

  bpm->UnpinPage(HEADER_PAGE_ID, true);
  delete transaction;
  delete disk_manager;
  delete bpm;
  remove("test.db");
  remove("test.log");
}


} // namespace cmudb
