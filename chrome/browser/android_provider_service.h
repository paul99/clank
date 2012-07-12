// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_PROVIDER_SERVICE_H_
#define CHROME_BROWSER_ANDROID_PROVIDER_SERVICE_H_

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "chrome/browser/cancelable_request.h"
#include "chrome/browser/history/android_history_types.h"
#include "sql/statement.h"

class Profile;

// This class provides the methods to communicate with history backend service.
class AndroidProviderService :
  public CancelableRequestProvider,
  public base::RefCountedThreadSafe<AndroidProviderService> {
 public:
  explicit AndroidProviderService(Profile* profile);

  // The callback definitions ------------------------------------------------
  typedef base::Callback<void(
                    Handle,                // handle
                    bool,                  // true if the query succeeded.
                    history::AndroidStatement*)>  // the result of query
                    QueryCallback;
  typedef CancelableRequest<QueryCallback> QueryRequest;

  typedef base::Callback<void(
                    Handle,                // handle
                    bool,                  // true if the update succeeded.
                    int)>             // the number of row updated.
                    UpdateCallback;
  typedef CancelableRequest<UpdateCallback> UpdateRequest;

  typedef base::Callback<void(
                    Handle,                // handle
                    bool,                  // true if the insert succeeded.
                    int64)>           // the id of inserted row.
                    InsertCallback;
  typedef CancelableRequest<InsertCallback> InsertRequest;

  typedef base::Callback<void(
                    Handle,                // handle
                    bool,                  // true if the delete succeeded.
                    int)>             // the number of row deleted.
                    DeleteCallback;
  typedef CancelableRequest<DeleteCallback> DeleteRequest;

  typedef base::Callback<void(
                    Handle,                           // handle
                    history::AndroidStatement*,       // moved statement .
                    int)>                        // the new position.
                    MoveStatementCallback;
  typedef CancelableRequest<MoveStatementCallback> MoveStatementRequest;

  typedef base::Callback<void(
                    Handle,                             // handle
                    std::vector<unsigned char>)>  // blob data
                    BindBlobCallback;
  typedef CancelableRequest<BindBlobCallback> BindBlobRequest;

  typedef base::Callback<void(
                    Handle,               // handle
                    string16)>      // String data
                    BindStringCallback;
  typedef CancelableRequest<BindStringCallback> BindStringRequest;

  struct StatementRequest {
    history::AndroidStatement* statement;
    int from;
    int to;
  };

  // Bookmark history ---------------------------------------------------------

  void QueryBookmark(
      const std::vector<history::BookmarkRow::BookmarkColumnID>& projections,
      const std::string selection,
      const std::vector<string16>& selection_args,
      const std::string sort_order,
      CancelableRequestConsumerBase* consumer,
      const QueryCallback& callback);

  void UpdateBookmark(const history::BookmarkRow& row,
                      const std::string& selection,
                      const std::vector<string16>& selection_args,
                      CancelableRequestConsumerBase* consumer,
                      const UpdateCallback& callback);

  void DeleteBookmark(const std::string& selection,
                      const std::vector<string16>& selection_args,
                      CancelableRequestConsumerBase* consumer,
                      const DeleteCallback& callback);

  void DeleteHistory(const std::string& selection,
                     const std::vector<string16>& selection_args,
                     CancelableRequestConsumerBase* consumer,
                     const DeleteCallback& callback);

  void InsertBookmark(const history::BookmarkRow& values,
                      CancelableRequestConsumerBase* consumer,
                      const InsertCallback& callback);

  // Cursor -------------------------------------------------------------------
  // Moved the statement's current row from |from| to |to| in DB thread. The
  // moved statement and the new position will be returned in the callback; The
  // new position might not be |to| if the |to| exceeds the count of row. The
  // moved statement will be null if failed.
  void MoveStatement(const StatementRequest& statement_request,
                     CancelableRequestConsumerBase* consumer,
                     const MoveStatementCallback& callback);

  // Asks to close the statement in database thread.
  void CloseStatement(history::AndroidStatement* statement);

  // Bind specific column as Blob in db thread. The result is returned from
  // callback.
  void BindBlob(history::LazyBindingColumn *column,
                CancelableRequestConsumerBase* consumer,
                const BindBlobCallback& callback);

  // Bind specific column as String in db thread. The result is returned from
  // callback.
  void BindString(history::LazyBindingColumn *column,
                  CancelableRequestConsumerBase* consumer,
                  const BindStringCallback& callback);

  // Search -------------------------------------------------------------------
  void InsertSearchTerm(const history::SearchRow& row,
                          CancelableRequestConsumerBase* consumer,
                          const InsertCallback& callback);

  void UpdateSearchTerm(const history::SearchRow& row,
                          const std::string& selection,
                          const std::vector<string16>& selection_args,
                          CancelableRequestConsumerBase* consumer,
                          const UpdateCallback& callback);

  void DeleteSearchTerm(const std::string& selection,
                          const std::vector<string16>& selection_args,
                          CancelableRequestConsumerBase* consumer,
                          const DeleteCallback& callback);

  void QuerySearchTerm(
      const std::vector<history::SearchRow::SearchColumnID>& projections,
      const std::string selection,
      const std::vector<string16>& selection_args,
      const std::string sort_order,
      CancelableRequestConsumerBase* consumer,
      const QueryCallback& callback);

 private:
  friend class base::RefCountedThreadSafe<AndroidProviderService>;

  virtual ~AndroidProviderService();

  void ForwardEmptyInsertResultAsync(InsertRequest* request);
  void ForwardEmptyQueryResultAsync(QueryRequest* request);
  void ForwardEmptyUpdateResultAsync(UpdateRequest* request);
  void ForwardEmptyDeleteResultAsync(UpdateRequest* request);
  void ForwardEmptyMoveStatementResultAsync(MoveStatementRequest* request,
                                            int value);
  void ForwardEmptyBindBlobResultAsync(BindBlobRequest* request);
  void ForwardEmptyBindStringResultAsync(BindStringRequest* request);

  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(AndroidProviderService);
};

#endif  // CHROME_BROWSER_ANDROID_PROVIDER_SERVICE_H_
