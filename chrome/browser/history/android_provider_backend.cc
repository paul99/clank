// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/history/android_provider_backend.h"

#include "base/i18n/case_conversion.h"
#include "chrome/browser/bookmarks/bookmark_model.h"
#include "chrome/browser/bookmarks/bookmark_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/history/history_backend.h"
#include "chrome/browser/history/history_database.h"
#include "chrome/browser/history/thumbnail_database.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "content/public/common/page_transition_types.h"
#include "sql/connection.h"
#include "sql/statement.h"

using content::BrowserThread;

// TODO : We need to implement our own GetCachedStatement() instead of using
// GetUniqueStatement().

namespace history {

static const char* kVirtualBookmarkTable =
    "SELECT android_urls.id AS _id, android_urls.created_time AS created, "
        "urls.title AS title, android_urls.raw_url AS url, "
        "urls.visit_count AS visits, android_urls.last_visit_time AS date, "
        "android_urls.bookmark AS bookmark, "
        "android_urls.favicon_id AS favicon, urls.id AS url_id, "
        "urls.url AS urls_url, "
    // This is hack, Android framework use folder = 0 to indicate the url is
    // bookmark.
        "(CASE WHEN android_urls.bookmark IS 0 THEN 1 ELSE 0 END) as folder "
    "FROM (android_urls JOIN urls on (android_urls.url_id = urls.id))";

static const char* kSearchJoinSource =
    " keyword_search_terms LEFT JOIN android_urls ON ("
        "android_urls.url_id = keyword_search_terms.url_id) ";

static const char* kSearchProjection =
    " keyword_search_terms.url_id AS _id, "
    "android_urls.last_visit_time AS date, "
    "keyword_search_terms.term AS search ";

int64 ToMilliSeconds(const base::Time& time) {
  base::TimeDelta delta = time - base::Time::UnixEpoch();
  return delta.InMilliseconds();
}

// Binds the given statement with |row|. |selection| specifies the columns in
// |row| needs to bound; If it is empty, all columns in row will be bound.
// The time should be converted to millisecondes if the |millisecond| is true.
static void BindStatement(const BookmarkRow& row,
                          std::set<BookmarkRow::BookmarkColumnID> selection,
                          sql::Statement* statement,
                          int* col_index,
                          bool millisecond) {
  const std::vector<BookmarkRow::BookmarkColumnID> ids = row.values();
  for (std::vector<BookmarkRow::BookmarkColumnID>::const_iterator i =
       ids.begin(); i != ids.end(); ++i) {
    if (!selection.empty() && selection.find(*i) == selection.end())
      continue;

    switch(*i) {
      case BookmarkRow::URL:
        statement->BindString(*col_index,
            URLDatabase::GURLToDatabaseURL(row.url()));
        break;
      case BookmarkRow::Title:
        statement->BindString16(*col_index, row.title());
        break;
      case BookmarkRow::Created:
        statement->BindInt64(
            *col_index, millisecond ? ToMilliSeconds(row.created()) :
                                      row.created().ToInternalValue());
        break;
      case BookmarkRow::LastVisitTime:
        statement->BindInt64(*col_index,
              millisecond ? ToMilliSeconds(row.last_visit_time()) :
                            row.last_visit_time().ToInternalValue());
        break;
      case BookmarkRow::VisitCount:
        statement->BindInt(*col_index, row.visit_count());
        break;
      case BookmarkRow::Bookmark:
        statement->BindBool(*col_index, row.is_bookmark());
        break;
      case BookmarkRow::Favicon:
        statement->BindBlob(*col_index, &row.favicon().front(),
                            static_cast<int>(row.favicon().size()));
        break;
      case BookmarkRow::ID:
        statement->BindInt64(*col_index, row.id());
        break;
      case BookmarkRow::RawURL:
        statement->BindString(*col_index, row.raw_url());
        break;
      default:
        NOTREACHED() << "Column id " << *i << " shouldn't be in where clause";
    }
    (*col_index)++;
  }
}

static void BindStatement(const std::vector<string16>& selection_args,
                          sql::Statement* statement,
                          int* col_index) {
  for (std::vector<string16>::const_iterator i = selection_args.begin();
       i != selection_args.end(); ++i) {
    statement->BindString16(*col_index, *i);
    (*col_index)++;
  }
}

// This is a wrapper of id column of different table.
struct TableIDRow {
 public:
  TableIDRow(AndroidURLID android_url_id, URLID url_id, GURL url, bool bookmark)
      : android_url_id_(android_url_id),
        url_id_(url_id),
        url_(url),
        bookmark_(bookmark) {
  }

  // Returns the id of urls table.
  const URLID url_id() const {
    return url_id_;
  }

  // Returns the id of android_urls table.
  const AndroidURLID android_url_id() const {
    return android_url_id_;
  }

  // Returns url.
  const GURL& url() const {
    return url_;
  }

  // Return if the url bookmarked.
  bool bookmark() const {
    return bookmark_;
  }
 private:
  AndroidURLID android_url_id_;
  URLID url_id_;
  GURL url_;
  bool bookmark_;
};

// The basic class to handle Insert/Update/Remove for a table.
class SQLHandler : public base::RefCountedThreadSafe<SQLHandler> {
 public:
  SQLHandler(const std::string& table_name,
             sql::Connection* db)
    : table_name_(table_name),
      db_(db) {
  }

  // Updates the rows whose id is the given |ids_set| with |row|. Return true if
  // update succeeds. The number of updated rows is returned in |updated_count|.
  virtual bool Update(const BookmarkRow& row,
                      const std::vector<TableIDRow>& ids_set,
                      int* updated_count) {
    std::string sql;
    sql.append("UPDATE ");
    sql.append(table_name_);
    sql.append(" SET ");

    bool has_column = false;
    BookmarkRow update_row;
    const std::vector<BookmarkRow::BookmarkColumnID>& values = row.values();
    for (std::vector<BookmarkRow::BookmarkColumnID>::const_iterator i =
             values.begin(); i != values.end(); ++i) {
      if (columns_.find(*i) != columns_.end()) {
        if (has_column)
          sql.append(", ");
        else
          has_column = true;

        sql.append(GetColumnName(*i));
        sql.append(" = ? ");
      }
    }

    if (!has_column) {
      // No update.
      *updated_count = 0;
      return true;
    }

    if (!ids_set.empty()) {
      std::ostringstream oss;
      bool has_id = false;
      for (std::vector<TableIDRow>::const_iterator i = ids_set.begin();
           i != ids_set.end(); ++i) {
        if (has_id)
          oss << ", ";
        else
          has_id = true;

        oss << GetID(*i);
      }

      if (has_id) {
        sql.append(" WHERE id in ( ");
        sql.append(oss.str());
        sql.append(" )");
      }
    }

    sql::Statement statement(db_->GetUniqueStatement(sql.c_str()));
    if (!statement) {
      LOG(ERROR) << db_->GetErrorMessage();
      return false;
    }

    int count = 0;
    Bind(row, columns_, &statement, &count);

    if (!statement.Run()) {
      LOG(ERROR) << db_->GetErrorMessage();
      return false;
    }

    *updated_count += db_->GetLastChangeCount();
    return true;
  }

  // Returns true if inserting the given |row| succeeds, the new row id is
  // returned in |new_id|.
  virtual bool Insert(BookmarkRow* row, int64* new_id) = 0;

  // Deletes the rows whose id is in |ids_set|, return true if it succeeds.
  // The number of deleted row is returned in deleted_count.
  virtual bool Delete(std::vector<TableIDRow>& ids_set,
                      int* deleted_count) {
    std::string sql;
    sql.append("DELETE FROM ");
    sql.append(table_name_);

    if (!ids_set.empty()) {
      std::ostringstream oss;
      bool has_id = false;
      for (std::vector<TableIDRow>::const_iterator i = ids_set.begin();
           i != ids_set.end(); ++i) {
        if (has_id)
          oss << ", ";
        else
          has_id = true;
        oss << GetID(*i);
      }

      if (has_id) {
        sql.append(" WHERE ");
        sql.append(GetIDName());
        sql.append(" in ( ");
        sql.append(oss.str());
        sql.append(" )");
      }
    }

    if (!db_->Execute(sql.c_str())) {
      LOG(ERROR) << "SQLHandler::Delete " << db_->GetErrorMessage();
      return false;
    }

    *deleted_count = db_->GetLastChangeCount();
    return *deleted_count;
  }

  // Returns the table name.
  const std::string& table_name() {
    return table_name_;
  }

  // Returns true if the table's column is valid in |row|.
  bool HasColumnIn(const BookmarkRow& row) {
    for (std::set<BookmarkRow::BookmarkColumnID>::iterator i =
         columns_.begin(); i != columns_.end(); ++i) {
      if (row.is_valid(*i))
        return true;
    }
    return false;
  }

 protected:
  virtual ~SQLHandler() {
  }

  // Binds the given statement with |row|. |selection| specifies the columns in
  // |row| needs to bound; If it is empty, all columns in row will be bound.
  virtual void Bind(const BookmarkRow& row,
                    std::set<BookmarkRow::BookmarkColumnID> selection,
                    sql::Statement* statement,
                    int* col_index) = 0;

  // Returns the column name by id.
  virtual std::string GetColumnName(BookmarkRow::BookmarkColumnID id) = 0;

  // Returns the proper id from |row|.
  virtual int64 GetID(const TableIDRow& row) = 0;

  // Returns the proper column name for ID.
  virtual std::string GetIDName() {
    return "id";
  }

  sql::Connection* db() {
    return db_;
  }

  void AddColumn(BookmarkRow::BookmarkColumnID column_id) {
    columns_.insert(column_id);
  }

 private:
  friend class base::RefCountedThreadSafe<SQLHandler>;

  const std::string table_name_;

  // The Android bookmark columns in the table.
  std::set<BookmarkRow::BookmarkColumnID> columns_;
  sql::Connection* db_;

  DISALLOW_COPY_AND_ASSIGN(SQLHandler);
};

class AndroidUrlsSQLHandler : public SQLHandler {
 public:
  AndroidUrlsSQLHandler(
      sql::Connection* db,
      AndroidProviderBackend* provider)
    : SQLHandler("android_urls", db),
      provider_(provider) {
    AddColumn(BookmarkRow::RawURL);
    AddColumn(BookmarkRow::LastVisitTime);
    AddColumn(BookmarkRow::Created);
    // We don't handle bookmark and favicon columns as they will be synced
    // later.
  }

  virtual bool Insert(BookmarkRow* row, int64* new_id) {
    *new_id = provider_->AddURL(*row);
    return *new_id != 0;
  }

 protected:
  virtual ~AndroidUrlsSQLHandler() {
  }

  virtual void Bind(const BookmarkRow& row,
                    std::set<BookmarkRow::BookmarkColumnID> selection,
                    sql::Statement* statement,
                    int* col_index) {
    BindStatement(row, selection, statement, col_index, true);
  }

  virtual int64 GetID(const TableIDRow& row) {
    return row.android_url_id();
  }

  virtual std::string GetColumnName(BookmarkRow::BookmarkColumnID id) {
    switch (id) {
      case BookmarkRow::RawURL:
        return "raw_url";
      case BookmarkRow::Created:
        return "created_time";
      case BookmarkRow::LastVisitTime:
        return "last_visit_time";
      default:
        return std::string();
    }
  }

 private:
  AndroidProviderBackend* provider_;

  DISALLOW_COPY_AND_ASSIGN(AndroidUrlsSQLHandler);
};


// This class is the SQLHandler for urls table.
class UrlsSQLHandler : public SQLHandler {
 public:
  UrlsSQLHandler(
      sql::Connection* db,
      HistoryDatabase* history_db)
    : SQLHandler("urls", db),
      history_db_(history_db) {
    AddColumn(BookmarkRow::URL);
    AddColumn(BookmarkRow::VisitCount);
    AddColumn(BookmarkRow::Title);
    AddColumn(BookmarkRow::LastVisitTime);
  }

  virtual bool Insert(BookmarkRow* row, int64* new_id) {
    URLRow url_row(row->url());

    // Whether we already have this URL.
    URLID id = history_db_->GetRowForURL(row->url(), &url_row);
    if (id) {
      LOG(ERROR) << "AndroidProviderBackend::Insert Urls; url exists.";
      return false; // We already has this row.
    }

    // Set visit count as 1 if not specified.
    if (row->is_valid(BookmarkRow::VisitCount))
      url_row.set_visit_count(row->visit_count());
    else
      url_row.set_visit_count(1);

    if (row->is_valid(BookmarkRow::Title))
      url_row.set_title(row->title());

    // Set last visit time to now if not specified.
    if (row->is_valid(BookmarkRow::LastVisitTime))
      url_row.set_last_visit(row->last_visit_time());
    else
      url_row.set_last_visit(base::Time::Now());

    id = history_db_->AddURL(url_row);
    if (id) {
      // The subsequent inserts need below information.
      row->set_id(id);
      if (!row->is_valid(BookmarkRow::LastVisitTime))
        row->set_last_visit_time(url_row.last_visit());
    }
    *new_id = id;

    return id;
  }

 protected:
  virtual ~UrlsSQLHandler() {
  }

  virtual void Bind(const BookmarkRow& row,
                    std::set<BookmarkRow::BookmarkColumnID> selection,
                    sql::Statement* statement,
                    int* col_index) {
    BindStatement(row, selection, statement, col_index, false);
  }

  virtual int64 GetID(const TableIDRow& row) {
    return row.url_id();
  }

  virtual std::string GetColumnName(BookmarkRow::BookmarkColumnID id) {
    switch (id) {
      case BookmarkRow::URL:
        return "url";
      case BookmarkRow::VisitCount:
        return "visit_count";
      case BookmarkRow::Title:
        return "title";
      case BookmarkRow::LastVisitTime:
        return "last_visit_time";
      default:
        return std::string();
    }
  }

 private:
  HistoryDatabase* history_db_;

  DISALLOW_COPY_AND_ASSIGN(UrlsSQLHandler);
};

// This class is the SQLHandler for visits table.
class VisitSQLHandler : public SQLHandler {
 public:
  VisitSQLHandler(sql::Connection* db,
                  HistoryDatabase* history_db)
      : SQLHandler("visits", db),
        history_db_(history_db) {
    AddColumn(BookmarkRow::Created);
    AddColumn(BookmarkRow::LastVisitTime);
  }

  virtual bool Update(const BookmarkRow& row,
                      const std::vector<TableIDRow>& ids_set,
                      int* updated_count) {
    for (std::vector<TableIDRow>::const_iterator id = ids_set.begin();
        id != ids_set.end(); ++id) {
      if (row.is_valid(BookmarkRow::Created)) {
        DeleteURLVisitsInRange(id->url_id(), base::Time(), row.created());
        AddVisit(id->url_id(), row.created());
        (*updated_count)++;
      }
      if (row.is_valid(BookmarkRow::LastVisitTime)) {
        DeleteURLVisitsInRange(id->url_id(), row.last_visit_time(),
                               base::Time::Now());
        AddVisit(id->url_id(), row.last_visit_time());
        (*updated_count)++;
      }
    }
    return true;
  }

  virtual bool Insert(BookmarkRow* row, int64* new_id) {
    DCHECK(row->is_valid(BookmarkRow::ID));
    DCHECK(row->is_valid(BookmarkRow::LastVisitTime));

    *new_id = AddVisit(row->id(), row->last_visit_time());

    // Add a row if the last visit time is different from created time.
    if (row->is_valid(BookmarkRow::Created) &&
        row->created() != row->last_visit_time()) {
      AddVisit(row->id(), row->created());
    }
    return *new_id;
  }

 protected:
  virtual ~VisitSQLHandler() {
  }

  virtual void Bind(const BookmarkRow& row,
                    std::set<BookmarkRow::BookmarkColumnID> selection,
                    sql::Statement* statement,
                    int* col_index) {
    NOTREACHED();
  }

  virtual std::string GetColumnName(BookmarkRow::BookmarkColumnID id) {
    NOTREACHED();
    return std::string();
  }

  virtual int64 GetID(const TableIDRow& row) {
    return row.url_id();
  }

  virtual std::string GetIDName() {
    return "url";
  }

 private:
  // Build a SQL to return the first visit row for a url.
  const std::string BuildVisitsIDSetSQL() {
    std::string visits_id_set;
    visits_id_set.append("SELECT id FROM ");
    visits_id_set.append(table_name());
    visits_id_set.append(" WHERE url = ? ");
    visits_id_set.append(" ORDER BY visit_time ASC LIMIT 1");
    return visits_id_set;
  }

  bool DeleteURLVisitsInRange(URLID url_id,
                              const base::Time& from,
                              const base::Time& to) {
    sql::Statement statement(db()->GetCachedStatement(SQL_FROM_HERE,
        "DELETE FROM visits "
        "WHERE url = ? AND visit_time > ? AND visit_time < ? "));
    if (!statement) {
      LOG(ERROR) << db()->GetErrorMessage();
      return false;
    }
    statement.BindInt64(0, url_id);
    statement.BindInt64(1, from.ToInternalValue());
    statement.BindInt64(2, to.ToInternalValue());

    return statement.Run();
  }

  bool AddVisit(URLID url_id, const base::Time& visit_time) {
    // TODO : Is 'content::PageTransition::AUTO_BOOKMARK' proper?
    // if not, a new content::PageTransition type will need.
    VisitRow visit_row(url_id, visit_time, 0,
                       content::PAGE_TRANSITION_AUTO_BOOKMARK, 0);
    return history_db_->AddVisit(&visit_row, SOURCE_BROWSED);
  }

 private:
  HistoryDatabase* history_db_;

  DISALLOW_COPY_AND_ASSIGN(VisitSQLHandler);
};

// The SQL handler for icon_mapping and favicon table.
class FaviconSQLHandler : public SQLHandler {
 public:
  FaviconSQLHandler(sql::Connection* db,
                    ThumbnailDatabase* thumbnail_db)
    : SQLHandler("favicons", db),
      thumbnail_db_(thumbnail_db) {
    AddColumn(BookmarkRow::Favicon);
    AddColumn(BookmarkRow::URL);
  }

  virtual bool Update(const BookmarkRow& row,
                      const std::vector<TableIDRow>& ids_set,
                      int* updated_count) {
    FaviconID favicon_id = 0;
    if (row.is_valid(BookmarkRow::Favicon) && !row.favicon().empty()) {
      // If the image_data will be updated, a new favicon should be created,
      // as the older one could be used by other pages.
      favicon_id = thumbnail_db_->AddFavicon(GURL(), history::FAVICON);
      if (!favicon_id) {
        LOG(ERROR) << "FaviconSQLHandler::Update Favicon";
        return false;
      }
      scoped_refptr<RefCountedMemory> image_data =
          new RefCountedBytes(row.favicon());
      if (!thumbnail_db_->SetFavicon(favicon_id, image_data, base::Time::Now()))
        return false;
    }

    std::vector<FaviconID> favicon_ids;
    for(std::vector<TableIDRow>::const_iterator i = ids_set.begin();
        i != ids_set.end(); ++i) {
      if (row.is_valid(BookmarkRow::Favicon) &&
          row.is_valid(BookmarkRow::URL)) {
        // Both URL and favicon were updated. We need add the new mapping
        // between the url and favicon if favicon is valid; And remove
        // the mapping between the old url and old favicon.
        if (favicon_id)
          thumbnail_db_->AddIconMapping(row.url(), favicon_id);
        thumbnail_db_->DeleteIconMappings(i->url());
      } else if (row.is_valid(BookmarkRow::Favicon)) {
        // Only favicon was updated. If the url already has favicon
        // 1. Update the mapping between the URL and new icon.
        // 2. Save the older icon in the vector and check whether it
        // can be removed later.
        //
        // Otherwise add the mapping between the url and new icon.
        if (favicon_id) {
          // Add favicon to the existing url.
          IconMapping icon_mapping;
          if (thumbnail_db_->GetIconMappingForPageURL(i->url(), FAVICON,
                                                      &icon_mapping)) {
            thumbnail_db_->UpdateIconMapping(icon_mapping.mapping_id,
                                             favicon_id);
            favicon_ids.push_back(icon_mapping.icon_id);
          } else {
            thumbnail_db_->AddIconMapping(i->url(), favicon_id);
          }
        }
      } else if (row.is_valid(BookmarkRow::URL)) {
        // Only URL was updated. Add the a mapping of new url and delete the
        // older mapping.
        IconMapping icon_mapping;
        if (thumbnail_db_->GetIconMappingForPageURL(i->url(), FAVICON,
                                                    &icon_mapping)) {
          thumbnail_db_->AddIconMapping(row.url(), icon_mapping.icon_id);
          thumbnail_db_->DeleteIconMappings(i->url());
        }
      }
    }

    if (!favicon_ids.empty()) {
      // As we update the favicon, Let's remove unused favicons if any.
      DeleteUnusedFavicon(favicon_ids);
    }
    return true;
  }

  virtual bool Delete(std::vector<TableIDRow>& ids_set,
                      int* deleted_count) {
    std::vector<IconMappingID> icon_mapping_ids;
    std::vector<FaviconID> favicon_ids;
    for (std::vector<TableIDRow>::const_iterator i = ids_set.begin();
         i != ids_set.end(); ++i) {
      IconMapping icon_mapping;
      if (!thumbnail_db_->GetIconMappingForPageURL(i->url(), FAVICON,
                                                  &icon_mapping))
        continue;
      icon_mapping_ids.push_back(icon_mapping.mapping_id);
      favicon_ids.push_back(icon_mapping.icon_id);
    }

    if (!icon_mapping_ids.empty()) {
      bool has_id = false;
      std::ostringstream oss;
      for (std::vector<IconMappingID>::iterator i = icon_mapping_ids.begin();
          i != icon_mapping_ids.end(); ++i) {
        if (has_id)
          oss << ", ";
        else
          has_id = true;
        oss << (*i);
      }
      std::string sql;
      sql.append("DELETE FROM icon_mapping WHERE id IN (");
      sql.append(oss.str());
      sql.append(")");
      if (!db()->Execute(sql.c_str())) {
        LOG(ERROR) << "FaviconSQLHandler::delete failed. " <<
            db()->GetErrorMessage();
        return false;
      }
      *deleted_count = db()->GetLastChangeCount();
      thumbnail_db_->SetSyncIconMappings(true);
    }
    // Delete unused favicon.
    DeleteUnusedFavicon(favicon_ids);
    return *deleted_count;
  }

  virtual bool Insert(BookmarkRow* row, int64* id) {
    if (!row->is_valid(BookmarkRow::Favicon) || row->favicon().empty())
      return true;

    DCHECK(row->is_valid(BookmarkRow::URL));

    // Is it a problem to give a empty URL?
    *id = thumbnail_db_->AddFavicon(GURL(), history::FAVICON);
    if (!*id) {
      LOG(ERROR) << "AndroidProviderBackend::Insert Favicon";
      return false;
    }

    scoped_refptr<RefCountedMemory> image_data =
        new RefCountedBytes(row->favicon());
    thumbnail_db_->SetFavicon(*id, image_data, base::Time::Now());
    return thumbnail_db_->AddIconMapping(row->url(), *id);;
  }

 protected:
  virtual ~FaviconSQLHandler() {
  }

  virtual void Bind(const BookmarkRow& row,
                    std::set<BookmarkRow::BookmarkColumnID> selection,
                    sql::Statement* statement,
                    int* col_index) {
    NOTREACHED();
  }

  virtual std::string GetColumnName(BookmarkRow::BookmarkColumnID id) {
    NOTREACHED();
    return std::string();
  }

  virtual int64 GetID(const TableIDRow& row) {
    NOTREACHED() << "Not implemented";
    return 0;
  }

  virtual std::string GetIDName() {
    NOTREACHED() << "Not implemented";
    return std::string();
  }

 private:
  // Updates the icon mapping by the given favicon_id, page_url for the row with
  // |icon_mapping_id|.
  // Returns true if the update succeeded.
  bool UpdateIconMapping(IconMappingID icon_mapping_id,
                         FaviconID favicon_id,
                         const GURL& page_url) {
    std::string sql = BuildIconMappingUpdate(!page_url.is_empty(), favicon_id);

    sql::Statement statement (db()->GetUniqueStatement(sql.c_str()));
    if (!statement) {
      LOG(ERROR) << db()->GetErrorMessage();
      return false;
    }

    int count = 0;
    if (favicon_id)
      statement.BindInt64(count++, favicon_id);

    if (!page_url.is_empty())
      statement.BindString(count++, URLDatabase::GURLToDatabaseURL(page_url));

    statement.BindInt64(count++, icon_mapping_id);

    return statement.Run();
  }

  // Builds the update SQL to update icon_id and/or page_url.
  std::string BuildIconMappingUpdate(bool has_url, bool has_favicon) {
    DCHECK(has_url || has_favicon);

    std::string sql("UPDATE icon_mapping SET ");

    if (has_favicon)
      sql.append("icon_id = ? ");

    if (has_favicon && has_url)
      sql.append(", ");

    if (has_url)
      sql.append("page_url = ? ");

    return sql.append( "WHERE id = ? ");
  }

  // Delete the given favicons if they are not used by any pages.
  void DeleteUnusedFavicon(const std::vector<FaviconID>& ids) {
    for (std::vector<FaviconID>::const_iterator i = ids.begin(); i != ids.end();
         ++i) {
      if (*i != 0 && !thumbnail_db_->HasMappingFor(*i))
        thumbnail_db_->DeleteFavicon(*i);
    }
  }

  ThumbnailDatabase* thumbnail_db_;

  DISALLOW_COPY_AND_ASSIGN(FaviconSQLHandler);
};

// The SQL handler for bookmarking_mapping table.
class BookmarkMappingSQLHandler : public SQLHandler {
 public:
  BookmarkMappingSQLHandler(sql::Connection* db,
                            AndroidProviderBackend* provider,
                            URLDatabase* url_database,
                            Profile* profile)
      : SQLHandler("bookmark_mapping", db),
        provider_(provider),
        url_database_(url_database),
        profile_(profile) {
    AddColumn(BookmarkRow::Bookmark);
  }

  virtual bool Update(const BookmarkRow& row,
                      const std::vector<TableIDRow>& ids_set,
                      int* updated_count) {
    for(std::vector<TableIDRow>::const_iterator i = ids_set.begin();
        i != ids_set.end(); ++i) {
      if (row.is_bookmark()) {
        string16 updated_title;
        if (row.is_valid(BookmarkRow::Title)) {
          updated_title = row.title();
        } else {
          URLRow url_row;
          if (!url_database_->GetRowForURL(i->url(), &url_row))
            return false;
          updated_title = url_row.title();
        }
        GURL updated_url;
        if (row.is_valid(BookmarkRow::URL))
          updated_url = row.url();
        else
          updated_url = i->url();
        // Add a bookmark.
        (*updated_count)++;
        // The url is also updated.
        // We must delete the old one first, otherwise, if the old one doesn't
        // exist, the new added will be deleted.
        // http://b/6244670
        if (row.is_valid(BookmarkRow::URL))
          BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
              base::Bind(
                  &BookmarkMappingSQLHandler::RemoveBookmarkFromBookmarkModel,
                  this, i->url()));
        BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, base::Bind(
            &BookmarkMappingSQLHandler::AddBookmarkIntoBookmarkModel,this,
            updated_url, updated_title, row.parent_id()));
      } else {
        (*updated_count)++;
        BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, base::Bind(
            &BookmarkMappingSQLHandler::RemoveBookmarkFromBookmarkModel, this,
            i->url()));
      }
    }
    return true;
  }

  virtual bool Delete(std::vector<TableIDRow>& ids_set,
                      int* deleted_count) {
    for(std::vector<TableIDRow>::const_iterator i = ids_set.begin();
        i != ids_set.end(); ++i) {
      BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, base::Bind(
          &BookmarkMappingSQLHandler::RemoveBookmarkFromBookmarkModel, this,
          i->url()));
      (*deleted_count)++;
    }
    return true;
  }

  virtual bool Insert(BookmarkRow* row, int64* new_id) {
    if (!row->is_valid(BookmarkRow::Bookmark) || !row->is_bookmark())
      return true;

    DCHECK(row->is_valid(BookmarkRow::ID) && row->is_valid(BookmarkRow::URL));
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, base::Bind(
        &BookmarkMappingSQLHandler::AddBookmarkIntoBookmarkModel, this,
        row->url(), row->title(), row->parent_id()));
    return *new_id;
  }

 protected:
  virtual ~BookmarkMappingSQLHandler() {
  }

  virtual void Bind(const BookmarkRow& row,
                    std::set<BookmarkRow::BookmarkColumnID> selection,
                    sql::Statement* statement,
                    int* col_index) {
    NOTREACHED();
  }

  virtual std::string GetColumnName(BookmarkRow::BookmarkColumnID id) {
    NOTREACHED();
    return std::string();
  }

  virtual int64 GetID(const TableIDRow& row) {
    return row.url_id();
  }

  virtual std::string GetIDName() {
    return "url_id";
  }

 private:
  // Add the given url row into bookmark model on UI thread.
  void AddBookmarkIntoBookmarkModel(const GURL& url,
                                    const string16& title,
                                    int64 parent_id) {
    BookmarkModel* bookmark_model = profile_->GetBookmarkModel();
    if (!bookmark_model)
      return;
    // If parent_id doesn't define a valid bookmark node then the synced node
    // will be used by default.
    const BookmarkNode* parent = bookmark_model->GetNodeByID(parent_id);
    if (!parent)
      parent = bookmark_model->mobile_node();
    if (parent)
      bookmark_model->AddURL(parent, 0, title, url);
  }

  // Remove the give url from the bookmark model on UI thread.
  void RemoveBookmarkFromBookmarkModel(
      const GURL& url) {
    BookmarkModel* bookmark_model = profile_->GetBookmarkModel();
    if (!bookmark_model)
      return;
    std::vector<const BookmarkNode*> nodes;
    bookmark_model->GetNodesByURL(url, &nodes);
    for (std::vector<const BookmarkNode*>::iterator i = nodes.begin();
         i != nodes.end(); ++i) {
      const BookmarkNode* parent_node = (*i)->parent();
      bookmark_model->Remove(parent_node, parent_node->GetIndexOf(*i));
    }
  }

  AndroidProviderBackend* provider_;
  URLDatabase* url_database_;
  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkMappingSQLHandler);
};

class FaviconLazyBindingColumn : public LazyBindingColumn {
 public:
  FaviconLazyBindingColumn(int column_index,
                           sql::Statement* ref,
                           sql::Connection* db,
                           HistoryDatabase* history_db)
      : LazyBindingColumn(column_index, ref, sql::COLUMN_TYPE_BLOB),
        db_(db),
        history_db_(history_db) {
  }

  virtual ~FaviconLazyBindingColumn() {
  }

  virtual sql::Statement* BuildStatement() {
    FaviconID favicon_id = ref_->ColumnInt64(column_index_);
    URLRow url_row;
    scoped_ptr<sql::Statement> statement(new sql::Statement(
        db_->GetCachedStatement(SQL_FROM_HERE,
            "SELECT image_data FROM favicons WHERE id = ?")));
    if (!(*statement.get())) {
      LOG(ERROR) << db_->GetErrorMessage();
      return NULL;
    }
    statement->BindInt64(0, favicon_id);
    return statement.release();
  }

  virtual std::string GetDBErrorMessage() {
    return db_->GetErrorMessage();
  }

 private:
  sql::Connection* db_;
  HistoryDatabase* history_db_;
};

AndroidProviderBackend::AndroidProviderBackend(
    HistoryDatabase* history_db,
    ThumbnailDatabase* thumbnail_db,
    BookmarkService* bookmark_service,
    HistoryBackend* history_backend,
    Profile* profile)
    : db_(&history_db->GetDB()),
      history_db_(history_db),
      thumbnail_db_(thumbnail_db),
      bookmark_service_(bookmark_service),
      history_backend_(history_backend) {
  // Always push the UrlsSQLHandler first.
  sql_handlers_.push_back(scoped_refptr<SQLHandler>(
                              new UrlsSQLHandler(db_, history_db)));
  sql_handlers_.push_back(scoped_refptr<SQLHandler>(
                              new VisitSQLHandler(db_, history_db)));
  sql_handlers_.push_back(scoped_refptr<SQLHandler>(
                              new AndroidUrlsSQLHandler(db_, this)));
  sql_handlers_.push_back(scoped_refptr<SQLHandler>(
                              new FaviconSQLHandler(&thumbnail_db->db_,
                                                    thumbnail_db)));
  sql_handlers_.push_back(scoped_refptr<SQLHandler>(
      new BookmarkMappingSQLHandler(db_, this, history_db, profile)));
}

AndroidProviderBackend::~AndroidProviderBackend() {
}

bool AndroidProviderBackend::Init() {
  return CreateAndroidURLsTable();
}

bool AndroidProviderBackend::CreateAndroidURLsTable() {
  const char* name = "android_urls";
  if (!db_->DoesTableExist(name)) {
    std::string sql;
    sql.append("CREATE TABLE ");
    sql.append(name);
    sql.append("("
               "id INTEGER PRIMARY KEY,"
               "raw_url LONGVARCHAR,"               // Passed in raw url.
               "created_time INTEGER NOT NULL,"     // Time in millisecond.
               "last_visit_time INTEGER NOT NULL,"  // Time in millisecond.
               "url_id INTEGER NOT NULL,"           // url id in urls table.
               "favicon_id INTEGER DEFAULT NULL,"   // favicon id.
               "bookmark INTEGER DEFAULT 0"         // whether is bookmark.
               ")");
    if (!db_->Execute(sql.c_str())) {
      LOG(ERROR) << db_->GetErrorMessage();
      return false;
    }

    if (!db_->Execute("CREATE INDEX android_urls_raw_url_idx"
                      " ON android_urls(raw_url)")) {
      LOG(ERROR) << db_->GetErrorMessage();
      return false;
    }

    if (!db_->Execute("CREATE INDEX android_urls_url_id_idx"
                      " ON android_urls(url_id)")) {
      LOG(ERROR) << db_->GetErrorMessage();
      return false;
    }
  }
  return true;
}

bool AndroidProviderBackend::SyncAndroidURLs() {
  if (!SyncOnURLsVisited())
    return false;

  if (!SyncOnURLsRemoved())
    return false;

  if (!SyncOnBookmarkChanged())
    return false;

  if (!SyncOnFaviconChanged())
    return false;

  return true;
}

bool AndroidProviderBackend::SyncOnURLsVisited() {
  sql::Statement urls_statement(db_->GetCachedStatement(SQL_FROM_HERE,
      "SELECT urls.id, urls.last_visit_time, created_time, urls.url "
      "FROM urls LEFT JOIN "
          "(SELECT url as visit_url, min(visit_time) as created_time"
                       " FROM visits GROUP BY url) ON (visit_url = urls.id) "));

  if (!urls_statement) {
    LOG(ERROR) << db_->GetErrorMessage();
    return false;
  }

  while(urls_statement.Step()) {
    URLID url_id = urls_statement.ColumnInt64(0);

    if (HasURL(url_id)) {
      sql::Statement update_statement(db_->GetCachedStatement(SQL_FROM_HERE,
          "UPDATE android_urls SET created_time = ?, last_visit_time = ? "
          "WHERE url_id = ?"));
      if (!update_statement.is_valid()) {
        LOG(ERROR) << db_->GetErrorMessage();
        continue;
      }
      update_statement.BindInt64(0, ToMilliSeconds(base::Time::FromInternalValue(
          urls_statement.ColumnInt64(2))));
      update_statement.BindInt64(1, ToMilliSeconds(base::Time::FromInternalValue(
          urls_statement.ColumnInt64(1))));
      update_statement.BindInt64(2, urls_statement.ColumnInt64(0));
      if (!update_statement.Run()) {
        LOG(ERROR) << db_->GetErrorMessage();
        continue;
      }
    } else {
      // Add a new row, if no row has been updated.
      AddURL(urls_statement.ColumnString(3),
             base::Time::FromInternalValue(urls_statement.ColumnInt64(2)),
             base::Time::FromInternalValue(urls_statement.ColumnInt64(1)),
             urls_statement.ColumnInt64(0));
    }
  }
  return true;
}

bool AndroidProviderBackend::SyncOnURLsRemoved() {
  return db_->Execute("DELETE FROM android_urls WHERE url_id NOT IN ("
               "SELECT id FROM urls)");
}

bool AndroidProviderBackend::SyncOnBookmarkChanged() {
  if (bookmark_service_ == NULL) {
    LOG(ERROR) << "Bookmark service is not available";
    return false;
  }

  if (!db_->Execute("UPDATE android_urls SET bookmark = 0")) {
    LOG(ERROR) << db_->GetErrorMessage();
    return false;
  }

  bookmark_service_->BlockTillLoaded();
  std::vector<GURL> bookmark_urls;
  bookmark_service_->GetBookmarks(&bookmark_urls);

  bool has_id = false;
  std::ostringstream oss;
  for(std::vector<GURL>::const_iterator i = bookmark_urls.begin();
      i != bookmark_urls.end(); ++i) {
    URLID url_id = history_db_->GetRowForURL(*i, NULL);
    if (url_id == 0)
      continue;

    if (has_id)
      oss << ", ";
    else
      has_id = true;
    oss << url_id;
  }

  std::string sql("UPDATE android_urls SET bookmark = 1 WHERE url_id in (");
  sql.append(oss.str());
  sql.append(")");
  if (!db_->Execute(sql.c_str())) {
    LOG(ERROR) << db_->GetErrorMessage();
    return false;
  }
  return true;
}

bool AndroidProviderBackend::SyncOnFaviconChanged() {
  if (!db_->Execute("UPDATE android_urls SET favicon_id = NULL")) {
    LOG(ERROR) << db_->GetErrorMessage();
    return false;
  }
  // Populate all favicons
  sql::Statement statement (thumbnail_db_->db_.GetCachedStatement(
      SQL_FROM_HERE,
      "SELECT icon_mapping.page_url, favicons.id "
          "FROM (icon_mapping LEFT JOIN favicons ON ("
              "icon_mapping.icon_id = favicons.id)) "
          "WHERE favicons.icon_type = 1"));

  if (!statement) {
    LOG(ERROR) << thumbnail_db_->db_.GetErrorMessage();
    return false;
  }

  while(statement.Step()) {
    GURL url(statement.ColumnString(0));
    URLID url_id = history_db_->GetRowForURL(url, NULL);
    if (url_id == 0) {
      LOG(ERROR) << "Can not find favicon's page url";
      continue;
    }
    sql::Statement update_statement(db_->GetCachedStatement(SQL_FROM_HERE,
        "UPDATE android_urls SET favicon_id = ? WHERE url_id = ? "));
    if (!update_statement) {
      LOG(ERROR) << db_->GetErrorMessage();
      continue;
    }
    update_statement.BindInt64(0, statement.ColumnInt64(1));
    update_statement.BindInt64(1, url_id);
    if (!update_statement.Run()) {
      LOG(ERROR) << db_->GetErrorMessage();
      continue;
    }
  }
  return true;
}

AndroidStatement* AndroidProviderBackend::Query(
    const std::vector<BookmarkRow::BookmarkColumnID>& projections,
    const std::string& selection,
    const std::vector<string16>& selection_args,
    const std::string& sort_order) {
  SyncAndroidURLs();

  std::string sql;
  sql.append("SELECT ");
  int replaced_index = AppendResultColumn(projections, &sql);
  sql.append(" FROM (");
  sql.append(kVirtualBookmarkTable);
  sql.append(")");

  if (!selection.empty()) {
    sql.append(" WHERE ");
    sql.append(selection);
  }

  if (!sort_order.empty()) {
    sql.append(" ORDER BY ");
    sql.append(sort_order);
  }

  scoped_ptr<sql::Statement> statement (new sql::Statement(
      db_->GetUniqueStatement(sql.c_str())));
  if (!(*(statement.get()))) {
    LOG(ERROR) << db_->GetErrorMessage();
    return NULL;
  }

  int count = 0;
  BindStatement(selection_args, statement.get(), &count);
  sql::Statement* result = statement.release();
  AndroidStatement* android_statement = new AndroidStatement(result);
  if (replaced_index >= 0) {
    android_statement->Add(replaced_index,
        new FaviconLazyBindingColumn(replaced_index, result,
                                     &thumbnail_db_->db_, history_db_));
  }
  return android_statement;
}

int AndroidProviderBackend::Update(
    const BookmarkRow& row,
    const std::string& selection,
    const std::vector<string16>& selection_args) {
  SyncAndroidURLs();

  // _ID shouldn't be updated.
  DCHECK(!row.is_valid(BookmarkRow::ID));

  std::vector<TableIDRow> ids_set =
      BuildTableIDRows(selection, selection_args);
  if (!ids_set.size())
    return 0;

  int rows_count = 0;
  for (std::vector<scoped_refptr<SQLHandler> >::iterator i =
       sql_handlers_.begin(); i != sql_handlers_.end(); ++i) {
    int count = 0;
    if ((*i)->HasColumnIn(row)) {
      if (!(*i)->Update(row, ids_set, &count)) {
        LOG(ERROR) << "AndroidProviderBackend::Update failed " <<
            (*i)->table_name();
        return 0;
      }
    }
    // Keeps the maximum row count.
    rows_count = rows_count > count ? rows_count : count;
  }
  return rows_count;
}

URLID AndroidProviderBackend::Insert(const BookmarkRow& values) {
  SyncAndroidURLs();

  DCHECK(values.is_valid(BookmarkRow::URL));
  // Make a copy of values as we need change it during insert.
  BookmarkRow row = values;
  URLID url_id = 0;
  int count = 0;
  for (std::vector<scoped_refptr<SQLHandler> >::iterator i =
       sql_handlers_.begin(); i != sql_handlers_.end(); ++i) {
    int64 id;
    if (!(*i)->Insert(&row, &id)) {
      LOG(ERROR) << "Insert failed " << (*i)->table_name();
      return 0;
    }
    // Using id from android_urls_table.
    if (count++ == 2)
      url_id = id;
  }
  return url_id;
}

int AndroidProviderBackend::Delete(
    const std::string& selection,
    const std::vector<string16>& selection_args) {
  SyncAndroidURLs();

  std::vector<TableIDRow> ids_set =
      BuildTableIDRows(selection, selection_args);
  if (!ids_set.size())
    return 0;

  int rows_count = 0;
  for (std::vector<scoped_refptr<SQLHandler> >::iterator i =
       sql_handlers_.begin(); i != sql_handlers_.end(); ++i) {
    int count;
    if (!(*i)->Delete(ids_set, &count)) {
      LOG(ERROR) << "AndroidProviderBackend::Delete error.";
      return 0;
    }
    rows_count = rows_count > count ? rows_count : count;
  }
  return rows_count;
}

int AndroidProviderBackend::DeleteHistory(
    const std::string& selection,
    const std::vector<string16>& selection_args) {
  SyncAndroidURLs();

  std::vector<TableIDRow> ids_set = BuildTableIDRows(selection, selection_args);
  if (!ids_set.size())
    return 0;

  for (std::vector<TableIDRow>::const_iterator id = ids_set.begin();
       id != ids_set.end(); ++id) {
    std::vector<TableIDRow> new_selection;
    new_selection.push_back(*id);
    if (id->bookmark()) {
      // As it is the bookmark, we need reset the row in urls table,
      // instead of deleting it.
      BookmarkRow row;
      row.set_visit_count(0);
      row.set_last_visit_time(base::Time::UnixEpoch());
      int count;
      // Reset the row in url table.
      if (!sql_handlers_[0]->Update(row, new_selection, &count))
        return 0;
      // Delete the history.
      if (!sql_handlers_[1]->Delete(new_selection, &count))
        return 0;
      // Update android_urls table.
      if (!sql_handlers_[2]->Update(row, new_selection, &count))
        return 0;
    } else {
      for (std::vector<scoped_refptr<SQLHandler> >::iterator i =
           sql_handlers_.begin(); i != sql_handlers_.end(); ++i) {
        int count;
        if (!(*i)->Delete(new_selection, &count)) {
          LOG(ERROR) << "AndroidProviderBackend::Delete error.";
          return 0;
        }
      }
    }
  }
  return ids_set.size();
}

AndroidURLID AndroidProviderBackend::AddURL(const std::string& raw_url,
                                            const base::Time& created_time,
                                            const base::Time& last_visit_time,
                                            URLID url_id) {
  sql::Statement statement(db_->GetCachedStatement(SQL_FROM_HERE,
      "INSERT INTO android_urls (raw_url, created_time, "
      "last_visit_time, url_id) VALUES (?, ?, ?, ?)"));
  if (!statement) {
    LOG(ERROR) << db_->GetErrorMessage();
    return 0;
  }
  statement.BindString(0, raw_url);
  statement.BindInt64(1, ToMilliSeconds(created_time));
  statement.BindInt64(2, ToMilliSeconds(last_visit_time));
  statement.BindInt64(3, url_id);

  if (!statement.Run()) {
    LOG(ERROR) << db_->GetErrorMessage();
    return 0;
  }

  return db_->GetLastInsertRowId();
}

AndroidURLID AndroidProviderBackend::AddURL(const BookmarkRow& url_row) {
  base::Time created;
  if (url_row.is_valid(BookmarkRow::Created))
    created = url_row.created();
  else
    created = url_row.last_visit_time();
  return AddURL(url_row.raw_url(), created, url_row.last_visit_time(),
                url_row.id());
}

bool AndroidProviderBackend::HasURL(URLID url_id) {
  sql::Statement statement(db_->GetCachedStatement(SQL_FROM_HERE,
      "SELECT id FROM android_urls WHERE url_id =?"));
  if (!statement.is_valid()) {
    LOG(ERROR) << db_->GetErrorMessage();
    return 0;
  }
  statement.BindInt64(0, url_id);

  return statement.Step();
}

void AndroidProviderBackend::ClearAndroidURLsTable() {
  sql::Statement statement(db_->GetCachedStatement(SQL_FROM_HERE,
      "DELETE FROM android_urls"));
  if (!statement.is_valid()) {
    LOG(ERROR) << db_->GetErrorMessage();
    return;
  }
  statement.Run();
}

int AndroidProviderBackend::AppendResultColumn(
    const std::vector<BookmarkRow::BookmarkColumnID>& projections,
    std::string* result_column) {
  int replaced_index = -1;
  if (projections.empty()) {
    // Attachs all columns exception COUNT
    bool first = true;
    for (int i = BookmarkRow::ID; i != BookmarkRow::ColumnEnd; ++i) {
      if (first)
        first = false;
      else
        result_column->append(", ");
      if (i == BookmarkRow::Favicon)
        replaced_index = i;
      result_column->append(BookmarkRow::GetAndroidName(
          static_cast<BookmarkRow::BookmarkColumnID>(i)));
    }
  } else {
    // Attach the projections
    bool first = true;
    int index = 0;
    for (std::vector<BookmarkRow::BookmarkColumnID>::const_iterator i =
         projections.begin(); i != projections.end(); ++i) {
      if (first)
        first = false;
      else
        result_column->append(", ");

      if (*i == BookmarkRow::Favicon)
        replaced_index = index;

      if (*i != BookmarkRow::ColumnEnd)
        result_column->append(BookmarkRow::GetAndroidName(*i));
      index++;
    }
  }
  return replaced_index;
}

std::vector<TableIDRow> AndroidProviderBackend::BuildTableIDRows(
    const std::string& selection,
    const std::vector<string16>& selection_args) {
  std::string sql("SELECT _id, url_id, urls_url, bookmark FROM (");
  sql.append(kVirtualBookmarkTable);
  sql.append(" )");

  if (!selection.empty()) {
    sql.append(" WHERE ");
    sql.append(selection);
  }

  std::vector<TableIDRow> ids_set;
  sql::Statement statement(db_->GetUniqueStatement(sql.c_str()));
  if (!statement) {
    LOG(ERROR) << db_->GetErrorMessage();
    return ids_set;
  }

  int count = 0;
  BindStatement(selection_args, &statement, &count);

  while(statement.Step()) {
    ids_set.push_back(TableIDRow(statement.ColumnInt64(0),
        statement.ColumnInt64(1), GURL(statement.ColumnString(2)),
        statement.ColumnBool(3)));
  }
  return ids_set;
}

// Search ---------------------------------------------------------------------

static std::string BuildSearchIDSet(const std::string& selection) {
  std::string sql;
  sql.append("SELECT _id FROM ( SELECT ");
  sql.append(kSearchProjection);
  sql.append(" FROM ");
  sql.append(kSearchJoinSource);
  if (!selection.empty()) {
    sql.append(" WHERE ");
    sql.append(selection);
  }
  sql.append(")");
  return sql;
}

AndroidStatement* AndroidProviderBackend::QuerySearchTerm(
    const std::vector<SearchRow::SearchColumnID>& projections,
    const std::string& selection,
    const std::vector<string16>& selection_args,
    const std::string& sort_order) {
  SyncAndroidURLs();

  std::string sql;
  sql.append("SELECT ");
  bool has_cols = false;
  for (std::vector<SearchRow::SearchColumnID>::const_iterator i =
      projections.begin(); i != projections.end(); ++i) {
    if (has_cols)
      sql.append(", ");
    else
      has_cols = true;
    sql.append(SearchRow::GetAndroidName(*i));
  }

  sql.append(" FROM ( SELECT ");
  sql.append(kSearchProjection);
  sql.append(" FROM ");
  sql.append(kSearchJoinSource);
  sql.append(")");
  if (!selection.empty()) {
    sql.append(" WHERE ");
    sql.append(selection);
  }

  scoped_ptr<sql::Statement> statement(new sql::Statement(
      db_->GetUniqueStatement(sql.c_str())));
  if (!(*statement)) {
    LOG(ERROR) << "QuerySearchTerm" << db_->GetErrorMessage();
    return NULL;
  }

  int col = 0;
  for (std::vector<string16>::const_iterator i = selection_args.begin();
      i != selection_args.end(); ++i) {
      statement->BindString16(col++, *i);
  }

  return new AndroidStatement(statement.release());
}

int AndroidProviderBackend::UpdateSearchTerm(
    const SearchRow& row,
    const std::string& selection,
    const std::vector<string16>& selection_args) {
  if (row.is_valid(SearchRow::SearchTerm) &&
      FindSearchTerm(row.search_term()))
    return 0;

  sql::Statement statement(db_->GetUniqueStatement(BuildSearchIDSet(
      selection).c_str()));

  if (!statement) {
    LOG(ERROR) << db_->GetErrorMessage();
    return 0;
  }

  int col = 0;
  for (std::vector<string16>::const_iterator i = selection_args.begin();
      i != selection_args.end(); ++i) {
    statement.BindString16(col++, *i);
  }

  int count = 0;
  while(statement.Step()) {
    if (row.is_valid(SearchRow::SearchTerm)) {
      std::vector<history::VisitInfo> times;
      // TODO(carlosvaldivia): Currently we use PAGE_TRANSITION_TYPED as a dummy
      // value for an appropriate PageTransition value that should really be
      // provided in the SearchRow.
      if (row.is_valid(SearchRow::SearchTime)) {
        times.push_back(history::VisitInfo(row.search_time(),
                                           content::PAGE_TRANSITION_TYPED));
      } else {
        times.push_back(history::VisitInfo(base::Time::Now(),
                                           content::PAGE_TRANSITION_TYPED));
      }
      sql::Statement url_update_statement(db_->GetCachedStatement(SQL_FROM_HERE,
          "UPDATE urls SET url = ? WHERE id = ?"));
      if (!url_update_statement) {
        LOG(ERROR) << "UpdateSearchTerm" << db_->GetErrorMessage();
        continue;
      }
      url_update_statement.BindString(0,
          URLDatabase::GURLToDatabaseURL(row.url()));
      url_update_statement.BindInt64(1, statement.ColumnInt64(0));
      if (!url_update_statement.Run()) {
        LOG(ERROR) << "UpdateSearchTerm" << db_->GetErrorMessage();
        continue;
      }
      history_backend_->AddVisits(row.url(), times, SOURCE_BROWSED);
      sql::Statement update_statement(db_->GetCachedStatement(SQL_FROM_HERE,
          "UPDATE keyword_search_terms "
              "SET lower_term = ?, term = ? WHERE url_id = ?"));
      if (!update_statement) {
        LOG(ERROR) << "UpdateSearchTerm" << db_->GetErrorMessage();
        continue;
      }
      update_statement.BindString16(0, base::i18n::ToLower(row.search_term()));
      update_statement.BindString16(1, row.search_term());
      update_statement.BindInt64(2, statement.ColumnInt64(0));
      if (!update_statement.Run()) {
        LOG(ERROR) << "UpdateSearchTerm" << db_->GetErrorMessage();
        continue;
      }
      count += db_->GetLastChangeCount();
    } else if (row.is_valid(SearchRow::SearchTime)) {
      std::vector<history::VisitInfo> times;
      times.push_back(history::VisitInfo(row.search_time(),
                                         content::PAGE_TRANSITION_TYPED));
      history_backend_->AddVisits(row.url(), times, SOURCE_BROWSED);
      count++;
    }
  }
  return count;
}

int AndroidProviderBackend::DeleteSearchTerm(
    const std::string& selection,
    const std::vector<string16>& selection_args) {
  SyncAndroidURLs();

  std::string id_set = BuildSearchIDSet(selection);
  std::string sql;
  sql.append("DELETE FROM keyword_search_terms WHERE url_id IN (");
  sql.append(id_set);
  sql.append(")");

  sql::Statement statement(db_->GetUniqueStatement(sql.c_str()));

  if (!statement) {
    LOG(ERROR) << "DeleteSearchTerm" << db_->GetErrorMessage();
    return 0;
  }

  int col = 0;
  for (std::vector<string16>::const_iterator i = selection_args.begin();
      i != selection_args.end(); ++i) {
    statement.BindString16(col++, *i);
  }

  if (!statement.Run()) {
    LOG(ERROR) << "DeleteSearchTerm" << db_->GetErrorMessage();
    return 0;
  }

  return db_->GetLastChangeCount();
}

// Return true if the given search_term already exists.
URLID AndroidProviderBackend::FindSearchTerm(const string16& search_term) {
  sql::Statement statement(db_->GetCachedStatement(SQL_FROM_HERE,
      "SELECT url_id FROM keyword_search_terms WHERE lower_term = ?"));
  if (!statement) {
    LOG(ERROR) << "FindSearchTerm" << db_->GetErrorMessage();
    return 0;
  }
  statement.BindString16(0, base::i18n::ToLower(search_term));
  URLID id = 0;
  if (statement.Step())
    id = statement.ColumnInt64(0);

  return id;
}

}  // namespace history
