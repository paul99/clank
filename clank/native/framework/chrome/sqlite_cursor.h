// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CLANK_NATIVE_FRAMEWORK_CHROME_SQLITE_CURSOR_H_
#define CLANK_NATIVE_FRAMEWORK_CHROME_SQLITE_CURSOR_H_

#include <vector>

#include "base/android/scoped_java_ref.h"
#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "base/string16.h"
#include "base/synchronization/waitable_event.h"
#include "chrome/browser/android_provider_service.h"
#include "chrome/browser/cancelable_request.h"
#include "content/browser/android/jni_helper.h"

class AndroidProviderService;
class Profile;

// This class provides all the features required by
// com.google.android.apps.chrome.database.SqliteCursor.
//
// It uses the AndroidStatement to iterate among the result rows. Lazy binding
// is used to query from multiple databases without 'ATTACH' them together
// which might introduces a lot of issues.
//
// Lazy binding is implemented for GetString() and GetBlob() methods as they
// are only required now to get favicon from Thumbnail DB.
class SQLiteCursor {
 public:
  // Mapping to the column type definition in java.sql.Types.
  enum JavaColumnType {
    Bl0b = 2004,
    LongVarChar = -1,
    Null = 0,
    Numeric = 2,
    Double = 8,
  };

  // |column_names| is the column names of this cursor, the sequence of name
  // should match the sql query's projection name.
  // |statement| is query's statement which bound the variables. This class
  // take the ownership of |statement|.
  SQLiteCursor(const std::vector<std::string>& column_names,
               history::AndroidStatement* statement,
               AndroidProviderService* service);
  void Destroy(JNIEnv* env, jobject obj);

  // Returns com.android.chrom.SQLiteCursor java object by creating SQLitCursor
  // native, java objects and bind them together.
  static base::android::ScopedJavaLocalRef<jobject> NewJavaSqliteCursor(
      JNIEnv* env,
      const std::vector<std::string>& column_names,
      history::AndroidStatement* statement,
      AndroidProviderService* service);

  // Returns the result row count.
  jint GetCount(JNIEnv* env, jobject obj);

  // Returns the result's columns' name.
  base::android::ScopedJavaLocalRef<jobjectArray> GetColumnNames(
      JNIEnv* env, jobject obj);

  // Returns the given column value as jstring.
  base::android::ScopedJavaLocalRef<jstring> GetString(
      JNIEnv* env, jobject obj, jint column);

  // Returns the given column value as jlong.
  jlong GetLong(JNIEnv* env, jobject obj, jint column);

  // Returns the given column value as int.
  jint GetInt(JNIEnv* env, jobject obj, jint column);

  // Returns the given column value as double.
  jdouble GetDouble(JNIEnv* env, jobject obj, jint column);

  // Returns the given column value as jbyteArray.
  base::android::ScopedJavaLocalRef<jbyteArray> GetBlob(
      JNIEnv* env, jobject obj, jint column);

  // Return JNI_TRUE if the give column value is NULL, JNI_FALSE otherwise.
  jboolean IsNull(JNIEnv* env, jobject obj, jint column);

  // Moves the cursor to |pos|, returns new position.
  // If the returned position is not equal to |pos|, then the cursor points to
  // the last row.
  jint MoveTo(JNIEnv* env, jobject obj, jint pos);

  jint GetColumnType(JNIEnv* env, jobject obj, jint column);

 private:
  virtual ~SQLiteCursor();

  // The callback function of MoveTo().
  void OnMoved(AndroidProviderService::Handle handle,
               history::AndroidStatement* statement,
               int pos);

  // The callback function of GetBlob.
  void OnBoundBlob(AndroidProviderService::Handle handle,
                   std::vector<unsigned char> blob);

  // The callback function of GetString.
  void OnBoundString(AndroidProviderService::Handle handle,
                     string16 bound_string);

  // Used to cancel all request on the UI thread during shutdown.
  void CancelAllRequests(base::WaitableEvent* finished);

  // Internal functions used by the JNI wrappers.
  string16 GetStringInternal(int column);
  std::vector<unsigned char> GetBlobInternal(int column);
  JavaColumnType GetColumnTypeInternal(int column);

  // The current row position.
  int position_;

  base::WaitableEvent event_;

  // The wrapped history::AndroidStatement.
  history::AndroidStatement* statement_;

  // Result set columns' name
  std::vector<std::string> column_names_;

  AndroidProviderService* service_;

  CancelableRequestConsumer consumer_;

  // The count of result rows.
  int count_;

  // The result of lazy bind column as Blob.
  std::vector<unsigned char> blob_;

  // The result of lazy bind column as String.
  string16 bound_string_;

  DISALLOW_COPY_AND_ASSIGN(SQLiteCursor);
};

bool RegisterSqliteCursor(JNIEnv* env);

#endif  // CLANK_NATIVE_FRAMEWORK_CHROME_SQLITE_CURSOR_H_
