// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#ifndef CHROME_BROWSER_HISTORY_ANDROID_PROVIDE_BACKEND_H_
#define CHROME_BROWSER_HISTORY_ANDROID_PROVIDE_BACKEND_H_

#include <set>

#include "base/file_path.h"
#include "base/hash_tables.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "chrome/browser/history/android_history_types.h"
#include "sql/statement.h"

class BookmarkService;
class Profile;

namespace history {

class AndroidProviderBackend;
class HistoryDatabase;
class SQLHandler;
struct TableIDRow;
class ThumbnailDatabase;

// This class provides the query/insert/update/remove methods to implement
// android.provider.Browser.BookmarkColumns API.
//
// It creates the bookmark_mapping table which is used to track the bookmarked
// pages and give ability to access the 'bookmark' column with SQL, as bookmarks
// are not stored in database.
//
// It binds urls, visits, bookmark_mapping, icon_mapping, favicons tables from
// history, and lazily binds favicon column in favicon database when the data
// need.
class AndroidProviderBackend :
  public base::RefCountedThreadSafe<AndroidProviderBackend> {
 public:
  AndroidProviderBackend(HistoryDatabase* history_db,
                         ThumbnailDatabase* thumbnail_db,
                         BookmarkService* bookmark_service,
                         HistoryBackend* history_backend,
                         Profile* profile);

  ~AndroidProviderBackend();

  // Initialize AndroidProviderBackend, it must be called before using any
  // other methods.
  bool Init();

  // Returns the result of the given query.
  // |projections| specifies the result columns. All columns (including COUNT)
  // will be returned if |projection| is empty.
  // |selection| is the SQL WHERE clause without 'WHERE'.
  // |selection_args| is the arguments for WHERE clause.
  // |sort_order| the SQL ORDER clause.
  //
  // For the query with 'COUNT' column, a row with COUNT = 0 is returned when
  // there is no matched row found.
  AndroidStatement* Query(
      const std::vector<BookmarkRow::BookmarkColumnID>& projections,
      const std::string& selection,
      const std::vector<string16>& selection_args,
      const std::string& sort_order);

  // Runs the given update and returns the number of updated rows.
  // |row| is the value need to update.
  // |selection| is the SQL WHERE clause without 'WHERE'.
  // |selection_args| is the arguments for WHERE clause.
  int Update(const BookmarkRow& row,
             const std::string& selection,
             const std::vector<string16>& selection_args);

  // Inserts the given valus and return the ID of inserted row.
  URLID Insert(const BookmarkRow& values);

  // Deletes the matched rows and the number of deleted rows.
  // |selection| is the SQL WHERE clause without 'WHERE'.
  // |selection_args| is the arguments for WHERE clause.
  //
  // if |selection| is empty all history and bookmark will be deleted.
  int Delete(const std::string& selection,
             const std::vector<string16>& selection_args);

  // Deletes the matched URL's history; The url row will be kept and the
  // visit_count and visit_time are reset if the url is bookmarked.
  int DeleteHistory(const std::string& selection,
                    const std::vector<string16>& selection_args);

  // Search ------------------------------------------------------------------

  AndroidStatement* QuerySearchTerm(
      const std::vector<SearchRow::SearchColumnID>& projections,
      const std::string& selection,
      const std::vector<string16>& selection_args,
      const std::string& sort_order);

  int UpdateSearchTerm(const SearchRow& row,
                       const std::string& selection,
                       const std::vector<string16>& selection_args);

  int DeleteSearchTerm(const std::string& selection,
                       const std::vector<string16>& selection_args);

  // Return url_id if the given search_term already exists.
  URLID FindSearchTerm(const string16& search_term);

  // Adds the give BookmarkRow into android_urls table.
  AndroidURLID AddURL(const BookmarkRow& url_row);

  // Clear all data in android_urls table.
  void ClearAndroidURLsTable();

 private:
  // To access AttachThumbnailDB/DetachThumbnailDB.
  friend class base::RefCountedThreadSafe<AndroidProviderBackend>;
  friend class TestAndroidProviderBackend;

  // Creates the android_urls table if it didn't exist. Return true if the
  // table was created or already exists. The table stores the raw url which
  // was passed in from ContentProvider APIs and the created/ last visit time
  // in milliseconds.
  // We have this table because
  // a. Android BookmarmkCoulmns API allows the url without protocol like
  //    "www.bookmarks.com", but Chrome requires the urls to be unique, like
  //    "http://www.bookmarks.com/". This table is used to map the raw URL to
  //    unique one.
  // b. Android APIs specifies the time in milliseconds, but Chrome use the
  //    microseconds.
  bool CreateAndroidURLsTable();

  // Sync android_urls table if it is necessary.
  bool SyncAndroidURLs();

  // Sync the android_urls when there are urls have been visited since last
  // sync.
  bool SyncOnURLsVisited();

  // Sync the android_urls when there are urls have been removed since last
  // sync.
  bool SyncOnURLsRemoved();

  // Sync the android_urls when there are bookmarks change.
  bool SyncOnBookmarkChanged();

  // Sync the android_urls when there are favicon change.
  bool SyncOnFaviconChanged();

  // Return the last sync time.
  base::Time GetLastSyncTime();

  void SetLastSyncTime(const base::Time& sync_time);

  // Return true if there were urls which have been visited since the last sync.
  bool URLsVisited();

  // Set whether there are urls which have been visited.
  void SetURLsVisited(bool sync);

  // Return true if there are urls have been removed from urls tables.
  bool URLsRemoved();

  // Set whether the urls have been removed from urls table.
  void SetURLsRemoved(bool sync);

  // Return true if there are bookmarks changed.
  bool BookmarkChanged();

  // Set whether there are bookmarks changed.
  void SetBookmarkChanged(bool sync);

  // Add a URL into android_urls table.
  AndroidURLID AddURL(const std::string& raw_url,
                      const base::Time& created_time,
                      const base::Time& last_visit_time,
                      URLID url_id);

  // Return true if the |url_id| is found in android_urls table.
  bool HasURL(URLID url_id);

  // Append the specified result columns in |projections| to the given
  // |result_column|. If |projections| is empty, all columns will be appended.
  // The index of favicon column will be returned if it exists, otherwise
  // returns -1.
  int AppendResultColumn(
      const std::vector<BookmarkRow::BookmarkColumnID>& projections,
      std::string* result_column);

  // Runs the given query and returns the selected urls.id, bookmark_mapping.id,
  // url in std::vector<TableIDRow>.
  std::vector<TableIDRow> BuildTableIDRows(
      const std::string& selection,
      const std::vector<string16>& selection_args);

  // Runs the given query and returns the URLIDs
  std::vector<URLID> BuildURLIDs(
      const std::string& selection,
      const std::vector<string16>& selection_args);

  // SQLHandlers for different table.
  std::vector<scoped_refptr<SQLHandler> > sql_handlers_;

  // The history db's connection.
  sql::Connection* db_;

  HistoryDatabase* history_db_;

  ThumbnailDatabase* thumbnail_db_;

  // Used to access bookmark model from backend.
  BookmarkService* bookmark_service_;

  HistoryBackend* history_backend_;

  DISALLOW_COPY_AND_ASSIGN(AndroidProviderBackend);
};

}  // namespace history

#endif  // CHROME_BROWSER_HISTORY_ANDROID_PROVIDE_BACKEND_H_
