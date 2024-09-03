#include <chrono>
#include <cstdio>
#include <cstdlib>

#include "logging/common.h"
#include "logging/log_recovery.h"
#include "vtable/virtual_table.h"
#include "gtest/gtest.h"
#include <thread>

namespace cmudb {


TEST(LogManagerTest, BasicLogging) {
  StorageEngine *storage_engine = new StorageEngine("test.db");

  EXPECT_FALSE(ENABLE_LOGGING);
  LOG_DEBUG("Skip system recovering...");

  storage_engine->log_manager_->RunFlushThread();
  EXPECT_TRUE(ENABLE_LOGGING);
  LOG_DEBUG("System logging thread running...");

  LOG_DEBUG("Create a test table");
  Transaction *txn = storage_engine->transaction_manager_->Begin();
  TableHeap *test_table = new TableHeap(storage_engine->buffer_pool_manager_,
                                        storage_engine->lock_manager_,
                                        storage_engine->log_manager_, txn);
  LOG_DEBUG("Insert and delete a random tuple");

  std::string createStmt =
      "a varchar, b smallint, c bigint, d bool, e varchar(16)";
  Schema *schema = ParseCreateStatement(createStmt);
  RID rid;
  Tuple tuple = ConstructTuple(schema);
  std::cout << tuple.ToString(schema).c_str() << std::endl;

  EXPECT_TRUE(test_table->InsertTuple(tuple, rid, txn));
  LOG_DEBUG("delete ...............................");
  EXPECT_TRUE(test_table->MarkDelete(rid, txn));
  storage_engine->transaction_manager_->Commit(txn);
  LOG_DEBUG("Commit txn");

  std::this_thread::sleep_for(std::chrono::seconds(1));

  storage_engine->log_manager_->StopFlushThread();
  EXPECT_FALSE(ENABLE_LOGGING);
  LOG_DEBUG("Turning off flushing thread");

  // some basic manually checking here
  char buffer[PAGE_SIZE];
  storage_engine->disk_manager_->ReadLog(buffer, PAGE_SIZE, 0);
  int32_t size = *reinterpret_cast<int32_t *>(buffer);
  LOG_DEBUG("size  = %d", size);
  size = *reinterpret_cast<int32_t *>(buffer + 20); // new page size
  LOG_DEBUG("size  = %d", size);
  size = *reinterpret_cast<int32_t *>(buffer + 44); // insert tuple size
  LOG_DEBUG("size  = %d", size);

  delete txn;
  delete storage_engine;
  LOG_DEBUG("Teared down the system");
  remove("test.db");
  remove("test.log");
}


// actually LogRecovery

TEST(LogManagerTest, RedoTestWithOneTxn) {
  StorageEngine *storage_engine = new StorageEngine("test.db");

  EXPECT_FALSE(ENABLE_LOGGING);
  LOG_DEBUG("Skip system recovering...");

  storage_engine->log_manager_->RunFlushThread();
  EXPECT_TRUE(ENABLE_LOGGING);
  LOG_DEBUG("System logging thread running...");

  LOG_DEBUG("Create a test table");
  Transaction *txn = storage_engine->transaction_manager_->Begin();
  TableHeap *test_table = new TableHeap(storage_engine->buffer_pool_manager_,
                                        storage_engine->lock_manager_,
                                        storage_engine->log_manager_, txn);
  page_id_t first_page_id = test_table->GetFirstPageId();

  std::string createStmt =
      "a varchar, b smallint, c bigint, d bool, e varchar(16)";
  Schema *schema = ParseCreateStatement(createStmt);

  RID rid;
  Tuple tuple = ConstructTuple(schema);
  std::cout << "Tuple: " << tuple.ToString(schema) << "\n";
  //Tuple tuple1 = ConstructTuple(schema);
  //std::cout << "Tuple1: " << tuple1.ToString(schema) << "\n";

  auto val = tuple.GetValue(schema, 4);
  EXPECT_TRUE(test_table->InsertTuple(tuple, rid, txn));
  storage_engine->transaction_manager_->Commit(txn);
  delete txn;
  delete test_table;
  LOG_DEBUG("Commit txn");

  LOG_DEBUG("SLEEPING for 2s");
  std::this_thread::sleep_for(std::chrono::seconds(2));


  // shutdown System
  delete storage_engine;
  //std::this_thread::sleep_for(std::chrono::seconds(2));
  
  // restart system
  storage_engine = new StorageEngine("test.db");

  // some basic manually checking here
  char buffer[PAGE_SIZE];
  storage_engine->disk_manager_->ReadLog(buffer, PAGE_SIZE, 0);
  int32_t size = *reinterpret_cast<int32_t *>(buffer);
  LOG_DEBUG("check begin size  = %d", size);
  size = *reinterpret_cast<int32_t *>(buffer + 20); // new page size
  LOG_DEBUG("check new page size  = %d", size);
  size = *reinterpret_cast<int32_t *>(buffer + 44); // insert tuple size
  LOG_DEBUG("check insert size  = %d", size);
  EXPECT_EQ(size, tuple.GetLength() + 32); // tuple's size + 20 + RID + 4 === tuple's size + 32


  LogRecovery *log_recovery = new LogRecovery(storage_engine->disk_manager_, storage_engine->buffer_pool_manager_);

  log_recovery->Redo();
  log_recovery->Undo();

  Tuple old_tuple;
  txn = storage_engine->transaction_manager_->Begin();
  test_table = new TableHeap(storage_engine->buffer_pool_manager_,
                             storage_engine->lock_manager_,
                             storage_engine->log_manager_, first_page_id);
  EXPECT_EQ(test_table->GetTuple(rid, old_tuple, txn), 1);
  storage_engine->transaction_manager_->Commit(txn);

  std::cout << "Old Tuple: " << old_tuple.ToString(schema) << "\n";
  delete txn;
  delete test_table;

  EXPECT_EQ(old_tuple.GetValue(schema, 4).CompareEquals(val), 1);

  delete storage_engine;
  LOG_DEBUG("Teared down the system");
  remove("test.db");
  remove("test.log");
}

TEST(LogManagerTest, RedoInsertTest) {
  StorageEngine *storage_engine = new StorageEngine("test.db");

  EXPECT_FALSE(ENABLE_LOGGING);
  LOG_DEBUG("Skip system recovering...");

  storage_engine->log_manager_->RunFlushThread();
  EXPECT_TRUE(ENABLE_LOGGING);
  LOG_DEBUG("System logging thread running...");

  LOG_DEBUG("Create a test table");
  Transaction *txn = storage_engine->transaction_manager_->Begin();
  TableHeap *test_table = new TableHeap(storage_engine->buffer_pool_manager_,
                                        storage_engine->lock_manager_,
                                        storage_engine->log_manager_, txn);
  page_id_t first_page_id = test_table->GetFirstPageId();

  std::string createStmt =
      "a varchar, b smallint, c bigint, d bool, e varchar(16)";
  Schema *schema = ParseCreateStatement(createStmt);

  RID rid;
  Tuple tuple = ConstructTuple(schema);
  std::cout << "Tuple: " << tuple.ToString(schema) << "\n";

  auto val = tuple.GetValue(schema, 4);
  EXPECT_TRUE(test_table->InsertTuple(tuple, rid, txn));
  delete txn;
  delete test_table;
  LOG_DEBUG("Crash before Commit txn...");

  LOG_DEBUG("SLEEPING for 2s");
  std::this_thread::sleep_for(std::chrono::seconds(2));

  // shutdown System
  delete storage_engine;
  
  // restart system
  storage_engine = new StorageEngine("test.db");

  LogRecovery *log_recovery = new LogRecovery(storage_engine->disk_manager_, storage_engine->buffer_pool_manager_);

  log_recovery->Redo();
  log_recovery->Undo();

  Tuple old_tuple;
  txn = storage_engine->transaction_manager_->Begin();
  test_table = new TableHeap(storage_engine->buffer_pool_manager_,
                             storage_engine->lock_manager_,
                             storage_engine->log_manager_, first_page_id);
  // tuple insertion is undo. So no such tuple.                           
  EXPECT_EQ(test_table->GetTuple(rid, old_tuple, txn), 0);                           
  storage_engine->transaction_manager_->Commit(txn);

  delete txn;
  delete test_table;

  delete storage_engine;
  LOG_DEBUG("Teared down the system");
  remove("test.db");
  remove("test.log");
}

TEST(LogManagerTest, RedoDeleteTest) {
  // commit txn1 for insert. Tx1 for delete crash before commit
  // expected result is tuple exists.
  StorageEngine *storage_engine = new StorageEngine("test.db");

  EXPECT_FALSE(ENABLE_LOGGING);
  LOG_DEBUG("Skip system recovering...");

  storage_engine->log_manager_->RunFlushThread();
  EXPECT_TRUE(ENABLE_LOGGING);
  LOG_DEBUG("System logging thread running...");

  LOG_DEBUG("Create a test table");
  Transaction *txn = storage_engine->transaction_manager_->Begin();
  TableHeap *test_table = new TableHeap(storage_engine->buffer_pool_manager_,
                                        storage_engine->lock_manager_,
                                        storage_engine->log_manager_, txn);
  page_id_t first_page_id = test_table->GetFirstPageId();

  std::string createStmt =
      "a varchar, b smallint, c bigint, d bool, e varchar(16)";
  Schema *schema = ParseCreateStatement(createStmt);

  RID rid;
  Tuple tuple = ConstructTuple(schema);
  std::cout << "Tuple: " << tuple.ToString(schema) << "\n";

  auto val = tuple.GetValue(schema, 4);
  EXPECT_TRUE(test_table->InsertTuple(tuple, rid, txn));  
  storage_engine->transaction_manager_->Commit(txn);
  LOG_DEBUG("Commit txn...");

// txn 2
  Transaction *txn2 = storage_engine->transaction_manager_->Begin();
  LOG_DEBUG("delete ...............................");
  EXPECT_TRUE(test_table->MarkDelete(rid, txn2));
  test_table->ApplyDelete(rid, txn2);

  LOG_DEBUG("SLEEPING for 1s");
  std::this_thread::sleep_for(std::chrono::seconds(1));

  delete txn;
  delete txn2;
  delete test_table;
  LOG_DEBUG("Crash before Commit txn2...");

  LOG_DEBUG("SLEEPING for 2s");
  std::this_thread::sleep_for(std::chrono::seconds(2));

  // shutdown System
  delete storage_engine;
  
  // restart system
  storage_engine = new StorageEngine("test.db");

  LogRecovery *log_recovery = new LogRecovery(storage_engine->disk_manager_, storage_engine->buffer_pool_manager_);

  log_recovery->Redo();
  log_recovery->Undo();

  Tuple old_tuple;
  txn = storage_engine->transaction_manager_->Begin();
  test_table = new TableHeap(storage_engine->buffer_pool_manager_,
                             storage_engine->lock_manager_,
                             storage_engine->log_manager_, first_page_id);
  // tuple insertion is undo. So no such tuple.                           
  EXPECT_EQ(test_table->GetTuple(rid, old_tuple, txn), 1);                           
  storage_engine->transaction_manager_->Commit(txn);
  std::cout << "Old Tuple: " << old_tuple.ToString(schema) << "\n";

  delete txn;
  delete test_table;

  delete storage_engine;
  LOG_DEBUG("Teared down the system");
  remove("test.db");
  remove("test.log");
}

} // namespace cmudb
