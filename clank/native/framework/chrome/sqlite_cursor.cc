// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "clank/native/framework/chrome/sqlite_cursor.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/bind.h"
#include "base/logging.h"
#include "chrome/browser/history/android_history_types.h"
#include "content/public/browser/browser_thread.h"
#include "jni/sqlite_cursor_jni.h"
#include "sql/statement.h"

using base::android::ConvertUTF8ToJavaString;
using base::android::GetClass;
using base::android::HasClass;
using base::android::HasMethod;
using base::android::GetMethodID;
using base::android::ScopedJavaLocalRef;
using content::BrowserThread;

namespace {

SQLiteCursor::JavaColumnType ToJavaColumnType(sql::ColType type) {
  switch (type) {
    case sql::COLUMN_TYPE_INTEGER:
      return SQLiteCursor::Numeric;
    case sql::COLUMN_TYPE_FLOAT:
      return SQLiteCursor::Double;
    case sql::COLUMN_TYPE_TEXT:
      return SQLiteCursor::LongVarChar;
    case sql::COLUMN_TYPE_BLOB:
      return SQLiteCursor::Bl0b;
    case sql::COLUMN_TYPE_NULL:
      return SQLiteCursor::Null;
    default:
      NOTREACHED();
  }
  return SQLiteCursor::Null;
}

}  // namespace.

void SQLiteCursor::Destroy(JNIEnv* env, jobject obj) {
  delete this;
}

ScopedJavaLocalRef<jobjectArray> SQLiteCursor::GetColumnNames(
    JNIEnv* env, jobject obj) {
  size_t count = column_names_.size();
  ScopedJavaLocalRef<jclass> sclass = GetClass(env, "java/lang/String");
  ScopedJavaLocalRef<jobjectArray> arr(env,
      env->NewObjectArray(count, sclass.obj(), NULL));
  for (size_t i = 0; i < count; i++) {
    ScopedJavaLocalRef<jstring> str =
        ConvertUTF8ToJavaString(env, column_names_[i].c_str());
    env->SetObjectArrayElement(arr.obj(), i, str.obj());
  }
  return arr;
}

ScopedJavaLocalRef<jstring> SQLiteCursor::GetString(
    JNIEnv* env, jobject obj, jint column) {
  string16 value = GetStringInternal(column);
  return ScopedJavaLocalRef<jstring>(env,
      env->NewString(value.data(), value.size()));
}

ScopedJavaLocalRef<jbyteArray> SQLiteCursor::GetBlob(
    JNIEnv* env, jobject obj, jint column) {
  std::vector<unsigned char> blob = GetBlobInternal(column);
  ScopedJavaLocalRef<jbyteArray> jb(env, env->NewByteArray(blob.size()));
  int count = 0;
  for(std::vector<unsigned char>::const_iterator i = blob.begin();
      i != blob.end(); ++i) {
    env->SetByteArrayRegion(jb.obj(), count++, 1, (jbyte *)i);
  }
  return jb;
}

jint SQLiteCursor::GetColumnType(JNIEnv* env, jobject obj, jint column) {
  return GetColumnTypeInternal(column);
}

bool RegisterSqliteCursor(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

ScopedJavaLocalRef<jobject> SQLiteCursor::NewJavaSqliteCursor(
    JNIEnv* env,
    const std::vector<std::string>& column_names,
    history::AndroidStatement* statement,
    AndroidProviderService* service) {
  if (!HasClass(env, kSQLiteCursorClassPath)) {
    LOG(ERROR) << "Can not find " << kSQLiteCursorClassPath;
    return ScopedJavaLocalRef<jobject>();
  }

  ScopedJavaLocalRef<jclass> sclass = GetClass(env, kSQLiteCursorClassPath);
  if (!HasMethod(env, sclass, "<init>", "(I)V")) {
    LOG(ERROR) << "Can not find " << kSQLiteCursorClassPath << " Constructor";
    return ScopedJavaLocalRef<jobject>();
  }

  jmethodID method_id = GetMethodID(env, sclass, "<init>", "(I)V");
  SQLiteCursor* cursor = new SQLiteCursor(column_names, statement, service);
  ScopedJavaLocalRef<jobject> obj(env,
      env->NewObject(sclass.obj(), method_id, reinterpret_cast<jint>(cursor)));
  if (obj.is_null()) {
    delete cursor;
    return ScopedJavaLocalRef<jobject>();
  }
  return obj;
}

SQLiteCursor::SQLiteCursor(const std::vector<std::string>& column_names,
                           history::AndroidStatement* statement,
                           AndroidProviderService* service)
    : position_(-1),
      event_(false, false),
      statement_(statement),
      column_names_(column_names),
      service_(service),
      count_(-1) {
}

SQLiteCursor::~SQLiteCursor() {
  // Consumer requests were set in the UI thread. They must be cancelled
  // using the same thread.
  if (BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    CancelAllRequests(NULL);
  } else {
    base::WaitableEvent event(false, false);
    BrowserThread::PostTask(
        BrowserThread::UI,
        FROM_HERE,
        base::Bind(&SQLiteCursor::CancelAllRequests, base::Unretained(this),
                   &event));
    event.Wait();
  }

  BrowserThread::PostTask(
      BrowserThread::UI,
      FROM_HERE,
      base::Bind(&AndroidProviderService::CloseStatement,
                 base::Unretained(service_), statement_));
}

void SQLiteCursor::CancelAllRequests(base::WaitableEvent* finished) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  consumer_.CancelAllRequests();
  if (finished)
    finished->Signal();
}

jint SQLiteCursor::GetCount(JNIEnv* env, jobject obj) {
  // Moves to unreasonable position so we will reach the last row, then finds
  // out the total number of rows.
  int current_position = position_;
  int count = MoveTo(env, obj, 0x7FFFFFFF) + 1;
  // Moves back to the previous position.
  MoveTo(env, obj, current_position);
  return count;
}

string16 SQLiteCursor::GetStringInternal(int column) {
  history::LazyBindingColumn* bound_column = statement_->Find(column);
  if (bound_column) {
    BrowserThread::PostTask(
        BrowserThread::UI,
        FROM_HERE,
        base::Bind(&AndroidProviderService::BindString,
            base::Unretained(service_), bound_column, &consumer_,
            base::Bind(&SQLiteCursor::OnBoundString, base::Unretained(this))));
    event_.Wait();
    return bound_string_;
  }
  return statement_->statement()->ColumnString16(column);
}

jlong SQLiteCursor::GetLong(JNIEnv* env, jobject obj, jint column) {
  history::LazyBindingColumn* bound_column = statement_->Find(column);
  if (bound_column) {
    LOG(ERROR) << "Don't support lazy binding Int64 " << column;
    return 0;
  }
  return statement_->statement()->ColumnInt64(column);
}

jint SQLiteCursor::GetInt(JNIEnv* env, jobject obj, jint column) {
  history::LazyBindingColumn* bound_column = statement_->Find(column);
  if (bound_column) {
    LOG(ERROR) << "Don't support lazy binding Int " << column;
    return 0;
  }
  return statement_->statement()->ColumnInt(column);
}

jdouble SQLiteCursor::GetDouble(JNIEnv* env, jobject obj, jint column) {
  history::LazyBindingColumn* bound_column = statement_->Find(column);
  if (bound_column) {
    LOG(ERROR) << "Don't support lazy binding Double " << column;
    return 0;
  }
  return statement_->statement()->ColumnDouble(column);
}

std::vector<unsigned char> SQLiteCursor::GetBlobInternal(int column) {
  history::LazyBindingColumn* bound_column = statement_->Find(column);
  if (bound_column) {
    BrowserThread::PostTask(
        BrowserThread::UI,
        FROM_HERE,
        base::Bind(&AndroidProviderService::BindBlob,
            base::Unretained(service_), bound_column, &consumer_,
            base::Bind(&SQLiteCursor::OnBoundBlob, base::Unretained(this))));
    event_.Wait();
    return blob_;
  }
  std::vector<unsigned char> blob;
  statement_->statement()->ColumnBlobAsVector(column, &blob);

  return blob;
}

jboolean SQLiteCursor::IsNull(JNIEnv* env, jobject obj, jint column) {
  return Null == GetColumnTypeInternal(column) ? JNI_TRUE : JNI_FALSE;
}

jint SQLiteCursor::MoveTo(JNIEnv* env, jobject obj, jint pos) {
  AndroidProviderService::StatementRequest request;
  request.statement = statement_;
  request.from = position_;
  request.to = pos;
  BrowserThread::PostTask(
      BrowserThread::UI,
      FROM_HERE,
      base::Bind(&AndroidProviderService::MoveStatement,
          base::Unretained(service_), request, &consumer_,
          base::Bind(&SQLiteCursor::OnMoved, base::Unretained(this))));
  event_.Wait();
  return position_;
}

SQLiteCursor::JavaColumnType SQLiteCursor::GetColumnTypeInternal(int column) {
  history::LazyBindingColumn* col = statement_->Find(column);
  if (col) {
    return ToJavaColumnType(col->type());
  }
  return ToJavaColumnType(statement_->statement()->ColumnType(column));
}

void SQLiteCursor::OnMoved(AndroidProviderService::Handle handle,
                           history::AndroidStatement* statement,
                           int pos) {
  position_ = pos;
  event_.Signal();
}

void SQLiteCursor::OnBoundBlob(AndroidProviderService::Handle handle,
                               std::vector<unsigned char> blob) {
  blob_ = blob;
  event_.Signal();
}

void SQLiteCursor::OnBoundString(AndroidProviderService::Handle handle,
                                 string16 bound_string) {
  bound_string_ = bound_string;
  event_.Signal();
}
