// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/history/android_history_types.h"

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/synchronization/lock.h"
#include "chrome/browser/history/android_provider_backend.h"

namespace history {

namespace {
// The column name defined in android.provider.Browser.BookmarkColumns
const char* const kAndroidBookmarkColumn[] = {
    "_id",
    "url",
    "title",
    "created",
    "date",
    "visits",
    "favicon",
    "bookmark",
    "raw_url",
};

// The column name defined in android.provider.Browser.SearchColumns
const char* const kAndroidSearchColumn[] = {
    "_id",
    "search",
    "date",
};

class BookmarkIDMapping : public std::hash_map<std::string,
                                               BookmarkRow::BookmarkColumnID> {
 public:
  BookmarkIDMapping() {
    COMPILE_ASSERT(arraysize(kAndroidBookmarkColumn) <= BookmarkRow::ColumnEnd,
                   Array_size_must_not_exceed_enum);
    for (size_t i = 0; i < arraysize(kAndroidBookmarkColumn); ++i) {
      (*this)[kAndroidBookmarkColumn[i]] =
          static_cast<BookmarkRow::BookmarkColumnID>(i);
    }
  }
};
base::LazyInstance<BookmarkIDMapping> g_bookmark_id_mapping =
  LAZY_INSTANCE_INITIALIZER;

class SearchIDMapping : public std::hash_map<std::string,
                                             SearchRow::SearchColumnID> {
 public:
  SearchIDMapping() {
    COMPILE_ASSERT(arraysize(kAndroidSearchColumn) <= SearchRow::ColumnEnd,
                   Array_size_must_not_exceed_enum);
    for (size_t i = 0; i < arraysize(kAndroidSearchColumn); ++i) {
      (*this)[kAndroidSearchColumn[i]] =
              static_cast<SearchRow::SearchColumnID>(i);
    }
  }
};
base::LazyInstance<SearchIDMapping> g_search_id_mapping =
  LAZY_INSTANCE_INITIALIZER;

}  // namespace

AndroidCommonRow::AndroidCommonRow()
    : id_(0) {
}

AndroidCommonRow::~AndroidCommonRow() {
}

BookmarkRow::BookmarkRow()
    : created_(base::Time()),
      last_visit_time_(base::Time()),
      visit_count_(0),
      is_bookmark_(false),
      parent_id_(-1) {
}

BookmarkRow::~BookmarkRow() {
}

std::string BookmarkRow::GetAndroidName(BookmarkColumnID id) {
  return kAndroidBookmarkColumn[id];
}

BookmarkRow::BookmarkColumnID BookmarkRow::GetBookmarkColumnID(
    const std::string& name) {
  BookmarkIDMapping::const_iterator i = g_bookmark_id_mapping.Get().find(name);
  if (i == g_bookmark_id_mapping.Get().end())
    return BookmarkRow::ColumnEnd;
  else
    return i->second;
}

// Copy the value of the specific |row|'s |id|.
void BookmarkRow::Copy(const BookmarkRow& row, BookmarkColumnID id) {
  switch(id) {
    case ID:
      set_id(row.id());
      break;
    case URL:
      set_url(row.url());
      break;
    case Title:
      set_title(row.title());
      break;
    case Created:
      set_created(row.created());
      break;
    case LastVisitTime:
      set_last_visit_time(row.last_visit_time());
      break;
    case VisitCount:
      set_visit_count(row.visit_count());
      break;
    case Favicon:
      set_favicon(row.favicon());
      break;
    case Bookmark:
      set_is_bookmark(row.is_bookmark());
      break;
    case ParentID:
      set_parent_id(row.parent_id());
      break;
    default:
      NOTREACHED() << "The value is not valid to copy";
  }
}

LazyBindingColumn::LazyBindingColumn(int column_index,
                                     sql::Statement* ref,
                                     sql::ColType type)
    : column_index_(column_index),
      ref_(ref),
      type_(type) {
}

LazyBindingColumn::~LazyBindingColumn() {
}

int LazyBindingColumn::ColumnInt() {
  scoped_ptr<sql::Statement> statement(BuildStatement());
  if (!statement.get() || !statement->Step())
    return 0;
  return statement->ColumnInt(0);
}
int64 LazyBindingColumn::ColumnInt64() {
  scoped_ptr<sql::Statement> statement(BuildStatement());
  if (!statement.get() || !statement->Step())
    return 0;
  return statement->ColumnInt64(0);
}

double LazyBindingColumn::ColumnDouble() {
  scoped_ptr<sql::Statement> statement(BuildStatement());
  if (!statement.get() || !statement->Step())
    return 0;
  return statement->ColumnDouble(0);
}

std::string LazyBindingColumn::ColumnString() {
  scoped_ptr<sql::Statement> statement(BuildStatement());
  if (!statement.get() || !statement->Step())
    return std::string();
  return statement->ColumnString(0);
}

string16 LazyBindingColumn::ColumnString16() {
  scoped_ptr<sql::Statement> statement(BuildStatement());
  if (!statement.get() || !statement->Step()) {
    return string16();
  }
  std::vector<unsigned char> val;
  statement->ColumnBlobAsVector(0, &val);
  return statement->ColumnString16(0);
}

void LazyBindingColumn::ColumnBlobAsVector(
    std::vector<unsigned char>* val) {
  scoped_ptr<sql::Statement> statement(BuildStatement());
  if (!statement.get() || !statement->Step()) {
    return;
  }
  statement->ColumnBlobAsVector(0, val);
}

AndroidStatement::AndroidStatement(sql::Statement* statement)
    : statement_(statement) {
}

AndroidStatement::~AndroidStatement() {
  if (columns_.size() > 0) {
    LazyBindingColumns::iterator it = columns_.begin();
    while (it != columns_.end()) {
      delete it->second;
      ++it;
    }
  }
}

// Returns true if the given index is lazy binding column.
bool AndroidStatement::IsLazyBinding(int index) {
  return columns_.find(index) == columns_.end();
}

LazyBindingColumn* AndroidStatement::Find(int index) {
  if (columns_.find(index) == columns_.end())
    return NULL;
  return columns_.find(index)->second;
}

void AndroidStatement::Add(int index, LazyBindingColumn* column) {
  columns_.insert(std::pair<int, LazyBindingColumn*>(index, column));
}

SearchRow::SearchRow() {
}

SearchRow::~SearchRow() {
}

std::string SearchRow::GetAndroidName(SearchColumnID id) {
  return kAndroidSearchColumn[id];
}

SearchRow::SearchColumnID SearchRow::GetSearchColumnID(
    const std::string& name) {
  SearchIDMapping::const_iterator i = g_search_id_mapping.Get().find(name);
  if (i == g_search_id_mapping.Get().end())
    return SearchRow::ColumnEnd;
  else
    return i->second;
}

}  // namespace history.
