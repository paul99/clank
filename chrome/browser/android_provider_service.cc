// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android_provider_service.h"

#include "chrome/browser/history/history.h"
#include "chrome/browser/profiles/profile.h"

AndroidProviderService::AndroidProviderService(Profile* profile)
    : profile_(profile) {
}

AndroidProviderService::~AndroidProviderService() {
}

void AndroidProviderService::QueryBookmark(
    const std::vector<history::BookmarkRow::BookmarkColumnID>& projections,
    const std::string selection,
    const std::vector<string16>& selection_args,
    const std::string sort_order,
    CancelableRequestConsumerBase* consumer,
    const QueryCallback& callback) {
  QueryRequest* request = new QueryRequest(callback);
  AddRequest(request, consumer);
  HistoryService* hs = profile_->GetHistoryService(Profile::EXPLICIT_ACCESS);
  if (hs)
    hs->QueryBookmark(request, projections, selection, selection_args,
                      sort_order);
  else
    ForwardEmptyQueryResultAsync(request);
}

void AndroidProviderService::UpdateBookmark(
    const history::BookmarkRow& row,
    const std::string& selection,
    const std::vector<string16>& selection_args,
    CancelableRequestConsumerBase* consumer,
    const UpdateCallback& callback) {
  UpdateRequest* request = new UpdateRequest(callback);
  AddRequest(request, consumer);
  HistoryService* hs = profile_->GetHistoryService(Profile::EXPLICIT_ACCESS);
  if (hs)
    hs->UpdateBookmark(request, row, selection, selection_args);
  else
    ForwardEmptyUpdateResultAsync(request);
}

void AndroidProviderService::DeleteBookmark(
    const std::string& selection,
    const std::vector<string16>& selection_args,
    CancelableRequestConsumerBase* consumer,
    const DeleteCallback& callback) {
  DeleteRequest* request = new DeleteRequest(callback);
  AddRequest(request, consumer);
  HistoryService* hs = profile_->GetHistoryService(Profile::EXPLICIT_ACCESS);
  if (hs)
    hs->DeleteBookmark(request, selection, selection_args);
  else
    ForwardEmptyDeleteResultAsync(request);
}

void AndroidProviderService::DeleteHistory(
    const std::string& selection,
    const std::vector<string16>& selection_args,
    CancelableRequestConsumerBase* consumer,
    const DeleteCallback& callback) {
  DeleteRequest* request = new DeleteRequest(callback);
  AddRequest(request, consumer);
  HistoryService* hs = profile_->GetHistoryService(Profile::EXPLICIT_ACCESS);
  if (hs)
    hs->DeleteHistory(request, selection, selection_args);
  else
    ForwardEmptyDeleteResultAsync(request);
}

void AndroidProviderService::InsertBookmark(
    const history::BookmarkRow& values,
    CancelableRequestConsumerBase* consumer,
    const InsertCallback& callback) {
  InsertRequest* request = new InsertRequest(callback);
  AddRequest(request, consumer);
  HistoryService* hs = profile_->GetHistoryService(Profile::EXPLICIT_ACCESS);
  if (hs)
    hs->AddBookmark(request, values);
  else
    ForwardEmptyInsertResultAsync(request);
}

void AndroidProviderService::MoveStatement(
    const StatementRequest& statement_request,
    CancelableRequestConsumerBase* consumer,
    const MoveStatementCallback& callback) {
  MoveStatementRequest* request = new MoveStatementRequest(callback);
  AddRequest(request, consumer);
  HistoryService* hs = profile_->GetHistoryService(Profile::EXPLICIT_ACCESS);
  if (hs)
    hs->MoveStatement(request, statement_request);
  else
    ForwardEmptyMoveStatementResultAsync(request, statement_request.from);
}

void AndroidProviderService::CloseStatement(
    history::AndroidStatement* statement) {
  HistoryService* hs = profile_->GetHistoryService(Profile::EXPLICIT_ACCESS);
  if (hs)
    hs->CloseStatement(statement);
}

void AndroidProviderService::BindBlob(history::LazyBindingColumn *column,
                                      CancelableRequestConsumerBase* consumer,
                                      const BindBlobCallback& callback) {
  BindBlobRequest* request = new BindBlobRequest(callback);
  AddRequest(request, consumer);
  HistoryService* hs = profile_->GetHistoryService(Profile::EXPLICIT_ACCESS);
  if (hs)
    hs->BindBlob(request, column);
  else
    ForwardEmptyBindBlobResultAsync(request);
}

void AndroidProviderService::BindString(history::LazyBindingColumn *column,
                                        CancelableRequestConsumerBase* consumer,
                                        const BindStringCallback& callback) {
  BindStringRequest* request = new BindStringRequest(callback);
  AddRequest(request, consumer);
  HistoryService* hs = profile_->GetHistoryService(Profile::EXPLICIT_ACCESS);
  if (hs)
    hs->BindString(request, column);
  else
    ForwardEmptyBindStringResultAsync(request);
}

void AndroidProviderService::InsertSearchTerm(
    const history::SearchRow& row,
    CancelableRequestConsumerBase* consumer,
    const InsertCallback& callback) {
  InsertRequest* request = new InsertRequest(callback);
  AddRequest(request, consumer);
  HistoryService* hs = profile_->GetHistoryService(Profile::EXPLICIT_ACCESS);
  if (hs)
    hs->AddSearchTerm(request, row);
  else
    ForwardEmptyInsertResultAsync(request);
}

void AndroidProviderService::UpdateSearchTerm(
    const history::SearchRow& row,
    const std::string& selection,
    const std::vector<string16>& selection_args,
    CancelableRequestConsumerBase* consumer,
    const UpdateCallback& callback) {
  UpdateRequest* request = new UpdateRequest(callback);
  AddRequest(request, consumer);
  HistoryService* hs = profile_->GetHistoryService(Profile::EXPLICIT_ACCESS);
  if (hs)
    hs->UpdateSearchTerm(request, row, selection, selection_args);
  else
    ForwardEmptyUpdateResultAsync(request);
}

void AndroidProviderService::DeleteSearchTerm(
    const std::string& selection,
    const std::vector<string16>& selection_args,
    CancelableRequestConsumerBase* consumer,
    const DeleteCallback& callback) {
  DeleteRequest* request = new DeleteRequest(callback);
  AddRequest(request, consumer);
  HistoryService* hs = profile_->GetHistoryService(Profile::EXPLICIT_ACCESS);
  if (hs)
    hs->DeleteSearchTerm(request, selection, selection_args);
  else
    ForwardEmptyDeleteResultAsync(request);
}

void AndroidProviderService::QuerySearchTerm(
    const std::vector<history::SearchRow::SearchColumnID>& projections,
    const std::string selection,
    const std::vector<string16>& selection_args,
    const std::string sort_order,
    CancelableRequestConsumerBase* consumer,
    const QueryCallback& callback) {
  QueryRequest* request = new QueryRequest(callback);
  AddRequest(request, consumer);
  HistoryService* hs = profile_->GetHistoryService(Profile::EXPLICIT_ACCESS);
  if (hs)
    hs->QuerySearchTerm(request, projections, selection, selection_args,
                        sort_order);
  else
    ForwardEmptyQueryResultAsync(request);
}

void AndroidProviderService::ForwardEmptyInsertResultAsync(
    InsertRequest* request) {
  request->ForwardResultAsync(request->handle(), false, 0);
}

void AndroidProviderService::ForwardEmptyQueryResultAsync(
    QueryRequest* request) {
  request->ForwardResultAsync(request->handle(), false, 0);
}

void AndroidProviderService::ForwardEmptyUpdateResultAsync(
    UpdateRequest* request) {
  request->ForwardResultAsync(request->handle(), false, 0);
}

void AndroidProviderService::ForwardEmptyDeleteResultAsync(
    DeleteRequest* request) {
  request->ForwardResultAsync(request->handle(), false, 0);
}

void AndroidProviderService::ForwardEmptyMoveStatementResultAsync(
    MoveStatementRequest* request,
    int value) {
  request->ForwardResultAsync(request->handle(), 0, value);
}

void AndroidProviderService::ForwardEmptyBindBlobResultAsync(
    BindBlobRequest* request) {
  request->ForwardResultAsync(request->handle(),
      std::vector<unsigned char>());
}

void AndroidProviderService::ForwardEmptyBindStringResultAsync(
    BindStringRequest* request) {
  request->ForwardResultAsync(request->handle(),
      string16());
}
