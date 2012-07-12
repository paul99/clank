// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_HISTORY_ANDROID_HISTORY_TYPES_H_
#define CHROME_BROWSER_HISTORY_ANDROID_HISTORY_TYPES_H_

#include <hash_map>

#include "base/memory/scoped_ptr.h"
#include "chrome/browser/history/history_types.h"
#include "chrome/browser/search_engines/template_url_id.h"
#include "sql/statement.h"

namespace sql {
class Statement;
}

namespace history {

typedef int64 AndroidURLID;

enum AndroidSyncType {
  SyncBookmarks = 1 << 0,
  SyncVisitedURLs = 1 << 1,
  SyncRemovedURLs = 1 << 2,
  SyncIcons = 1 << 3,
};

// Defines the common column for Android ContentProvider.
struct AndroidCommonRow {
 public:
  AndroidCommonRow();
  virtual ~AndroidCommonRow();

 protected:
  int64 id_;
};

// Wraps all columns supported by android.provider.Browser.BookmarkColumns
struct BookmarkRow : AndroidCommonRow {
 public:
  enum BookmarkColumnID {
    ID,
    URL,
    Title,
    Created,
    LastVisitTime,
    VisitCount,
    Favicon,
    Bookmark,
    RawURL,
    ParentID,
    ColumnEnd // This must be the last.
  };

  BookmarkRow();
  virtual ~BookmarkRow();

  // Returns the column name defined in Android.
  static std::string GetAndroidName(BookmarkColumnID id);

  static BookmarkColumnID GetBookmarkColumnID(const std::string& name);

  // URLs for the page.
  void set_url(const GURL& url) {
    SetValid(URL);
    url_ = url;
  }
  const GURL& url() const {
    return url_;
  }

  // Raw URL
  void set_raw_url(const std::string& raw_url) {
    SetValid(RawURL);
    raw_url_ = raw_url;
  }
  const std::string& raw_url() const {
    return raw_url_;
  }

  // The Title of page.
  void set_title(const string16& title) {
    SetValid(Title);
    title_ = title;
  }
  const string16& title() const {
    return title_;
  }

  // The page's first visit time.
  void set_created(const base::Time created) {
    SetValid(Created);
    created_ = created;
  }
  const base::Time& created() const {
    return created_;
  }

  // The page's last visit time.
  void set_last_visit_time (const base::Time last_visit_time) {
    SetValid(LastVisitTime);
    last_visit_time_ = last_visit_time;
  }
  const base::Time& last_visit_time() const {
    return last_visit_time_;
  }

  // The visit times
  void set_visit_count (int visit_count) {
    SetValid(VisitCount);
    visit_count_ = visit_count;
  }
  int visit_count() const {
    return visit_count_;
  }

  // Whether the page is bookmarked.
  void set_is_bookmark(bool is_bookmark) {
    SetValid(Bookmark);
    is_bookmark_ = is_bookmark;
  }
  bool is_bookmark() const {
    return is_bookmark_;
  }

  // The favicon related to page if any.
  void set_favicon(const std::vector<unsigned char>& data) {
    SetValid(Favicon);
    favicon_ = data;
  }
  const std::vector<unsigned char>& favicon() const {
    return favicon_;
  }

  // The id of url.
  void set_id (URLID url_id) {
    SetValid(ID);
    id_ = url_id;
  }
  const URLID id() const {
    return id_;
  }

  // The id of the parent folder containing the bookmark, if any.
  void set_parent_id(int64 parent_id) {
    SetValid(ParentID);
    parent_id_ = parent_id;
  }
  const int64 parent_id() const {
    return parent_id_;
  }

  // Returns all column values that have been set according it set order.
  const std::vector<BookmarkColumnID>& values() const {
    return values_;
  }

  // Returns true if the given id has been set.
  bool is_valid(BookmarkColumnID id) const {
    return values_set_.find(id) != values_set_.end();
  }

  // Copies the value of the specific |row|'s |id|.
  void Copy(const BookmarkRow& row, BookmarkColumnID id);

 private:
  void SetValid(BookmarkColumnID id) {
    values_.push_back(id);
    values_set_.insert(id);
  }

  GURL url_;
  std::string raw_url_;
  string16 title_;
  base::Time created_;
  base::Time last_visit_time_;
  std::vector<unsigned char> favicon_;
  int visit_count_;
  bool is_bookmark_;
  int64 parent_id_;

  // Used to keep the sequence of the column values being set.
  std::vector<BookmarkColumnID> values_;

  // Used to find whether a column has been set a value.
  std::set<BookmarkColumnID> values_set_;
};

// This is the base class for the lazy binding column.
//
// The lazy bind column is in a database other than the main database, and its
// value could be get from a main database's a column value when needed.
//
// BuildStatement() must be implemented by the subclass to implement the lazy
// binding.
class LazyBindingColumn {
 public:
  LazyBindingColumn(int column_index, sql::Statement* ref, sql::ColType type);

  virtual ~LazyBindingColumn();

  sql::ColType type() const {
    return type_;
  }

  int ColumnInt();
  int64 ColumnInt64();
  double ColumnDouble();
  std::string ColumnString();
  string16 ColumnString16();
  void ColumnBlobAsVector(std::vector<unsigned char>* val);

 protected:
  // Return the statement whose first column is bound column.
  virtual sql::Statement* BuildStatement() = 0;

  // Returns the database error message.
  virtual std::string GetDBErrorMessage() = 0;

  // The column index need to bind when it is accessed. It is also the
  // column index of |ref_| statement whose value is used to find the
  // value of lazy binding column.
  int column_index_;

  // The statement used to get the data to find the value of lazy binding
  // column.
  sql::Statement* ref_;

  // The type of lazily bound column.
  sql::ColType type_;
};

// This class wraps the sql statement and lazy binding column, and returned to
// caller to iterate among the result set.
// Before using statement to get a column value, IsLazyBind() must be called
// to check whether the column is lazy bound. If so
// AndroidProviderService::Bind...() method must be called to the actual value.
class AndroidStatement {
 public:
  AndroidStatement(sql::Statement* statement);
  virtual ~AndroidStatement();

  sql::Statement* statement() {
    return statement_.get();
  }

  // Returns true if the given index is lazy binding column.
  bool IsLazyBinding(int index);

  // Returns the LazyBindingColumn by column index.
  LazyBindingColumn* Find(int index);

  // Adds a lazy binding column.
  void Add(int index, LazyBindingColumn* column);
 private:
  scoped_ptr<sql::Statement> statement_;

  typedef std::hash_map<int, LazyBindingColumn*> LazyBindingColumns;
  LazyBindingColumns columns_;
};

struct SearchRow : AndroidCommonRow {
 public:
  enum SearchColumnID {
    ID,
    SearchTerm,
    SearchTime,
    URL,
    TemplateURL,
    ColumnEnd
  };

  SearchRow();
  virtual ~SearchRow();

  // Returns the column name defined in Android.
  static std::string GetAndroidName(SearchColumnID id);

  static SearchColumnID GetSearchColumnID(const std::string& name);

  const string16& search_term() const {
    return search_term_;
  }
  void set_search_term(const string16& search_term) {
    SetValid(SearchRow::SearchTerm);
    search_term_ = search_term;
  }

  const base::Time search_time() const {
    return search_time_;
  }
  void set_search_time(const base::Time& time) {
    SetValid(SearchRow::SearchTime);
    search_time_ = time;
  }

  const GURL& url() const {
    return url_;
  }
  void set_url(const GURL& url) {
    SetValid(SearchRow::URL);
    url_ = url;
  }

  TemplateURLID template_url_id() const {
    return template_url_id_;
  }
  void set_template_url_id(TemplateURLID template_url_id) {
    SetValid(SearchRow::TemplateURL);
    template_url_id_ = template_url_id;
  }

  bool is_valid(SearchColumnID id) const {
    return values_set_.find(id) != values_set_.end();
  }
 private:
  void SetValid(SearchColumnID id) {
    values_.push_back(id);
    values_set_.insert(id);
  }

  string16 search_term_;
  base::Time search_time_;
  GURL url_;
  TemplateURLID template_url_id_;

  // Used to keep the sequence of the column values being set.
  std::vector<SearchColumnID> values_;

  // Used to find whether a column has been set a value.
  std::set<SearchColumnID> values_set_;
};

}  // namespace history

#endif  // CHROME_BROWSER_HISTORY_ANDROID_HISTORY_TYPES_H_
