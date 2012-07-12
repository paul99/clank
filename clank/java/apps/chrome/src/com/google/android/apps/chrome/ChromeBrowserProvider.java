// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.google.android.apps.chrome;

import android.app.Activity;
import android.app.SearchManager;
import android.content.ContentProvider;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Build;
import android.os.Looper;
import android.os.Message;
import android.preference.PreferenceManager;
import android.provider.BaseColumns;
import android.provider.Browser;
import android.provider.Browser.BookmarkColumns;
import android.provider.Browser.SearchColumns;
import android.text.TextUtils;
import android.util.Log;

import com.google.android.apps.chrome.database.SQLiteCursor;
import com.google.android.apps.chrome.sync.SyncStatusHelper;

import org.chromium.base.CalledByNative;
import org.chromium.content.browser.CommandLine;
import org.chromium.content.browser.ThreadUtils;

import java.util.ArrayList;
import java.util.List;
import java.util.Vector;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * This class provides various information of Chrome, like bookmarks, most
 * visited page etc. It is used to support android.provider.Browser.
 *
 */
public class ChromeBrowserProvider extends ContentProvider {
    // Defines Chrome's API authority, so it can be run and tested
    // independently.
    private static final String API_AUTHORITY_SUFFIX = ".browser";

    private static final String BROWSER_CONTRACT_API_AUTHORITY =
        "com.google.android.apps.chrome.browser-contract";

    // These values are taken from android.provider.BrowserContract.java since
    // that class is hidden from the SDK.
    private static final String BROWSER_CONTRACT_AUTHORITY = "com.android.browser";
    private static final String BROWSER_CONTRACT_HISTORY_CONTENT_TYPE =
        "vnd.android.cursor.dir/browser-history";
    private static final String BROWSER_CONTRACT_HISTORY_CONTENT_ITEM_TYPE =
        "vnd.android.cursor.item/browser-history";

    // This Authority is for internal interface. It's concatenated with
    // Context.getPackageName() so that we can install different channels
    // SxS and have different authorities.
    private static final String AUTHORITY_SUFFIX = ".ChromeBrowserProvider";
    private static final String BOOKMARKS_PATH = "bookmarks";
    private static final String SEARCHES_PATH = "searches";
    private static final String HISTORY_PATH = "history";
    private static final String COMBINED_PATH = "combined";
    private static final String BOOKMARK_FOLDER_PATH = "hierarchy";

    public static final Uri BROWSER_CONTRACTS_BOOKMAKRS_API_URI = buildContentUri(
            BROWSER_CONTRACT_API_AUTHORITY, BOOKMARKS_PATH);

    public static final Uri BROWSER_CONTRACTS_SEARCHES_API_URI = buildContentUri(
            BROWSER_CONTRACT_API_AUTHORITY, SEARCHES_PATH);

    public static final Uri BROWSER_CONTRACTS_HISTORY_API_URI = buildContentUri(
            BROWSER_CONTRACT_API_AUTHORITY, HISTORY_PATH);

    public static final Uri BROWSER_CONTRACTS_COMBINED_API_URI = buildContentUri(
            BROWSER_CONTRACT_API_AUTHORITY, COMBINED_PATH);

    /** The parameter used to specify a bookmark parent ID in ContentValues. */
    public static final String BOOKMARK_PARENT_ID_PARAM = "parentId";
    /** The parameter used to specify whether this is a bookmark folder. */
    public static final String BOOKMARK_IS_FOLDER_PARAM = "isFolder";
    /** ID used to indicate an invalid/unset bookmark node. */
    public static final long INVALID_BOOKMARK_ID = -1;

    private static final String TAG = "ChromeBrowserProvider";

    private static final String LAST_MODIFIED_BOOKMARK_FOLDER_ID_KEY = "last_bookmark_folder_id";

    private static final int URI_MATCH_BOOKMARKS = 0;
    private static final int URI_MATCH_BOOKMARKS_ID = 1;
    private static final int URL_MATCH_API_BOOKMARK = 2;
    private static final int URL_MATCH_API_BOOKMARK_ID = 3;
    private static final int URL_MATCH_API_SEARCHES = 4;
    private static final int URL_MATCH_API_SEARCHES_ID = 5;
    private static final int URL_MATCH_API_HISTORY_CONTENT = 6;
    private static final int URL_MATCH_API_HISTORY_CONTENT_ID = 7;
    private static final int URL_MATCH_API_BOOKMARK_CONTENT = 8;
    private static final int URL_MATCH_API_BOOKMARK_CONTENT_ID = 9;
    private static final int URL_MATCH_BOOKMARK_SUGGESTIONS_ID = 10;
    private static final int URL_MATCH_BOOKMARK_HISTORY_SUGGESTIONS_ID = 11;
    private static final int URL_MATCH_BOOKMARK_FOLDER_ID = 12;

    private UriMatcher mUriMatcher;
    private static long mLastModifiedBookmarkFolderId = INVALID_BOOKMARK_ID;

    // TODO : Using Android.provider.Browser.HISTORY_PROJECTION once THUMBNAIL,
    // TOUCH_ICON, and USER_ENTERED fields are supported.
    private static final String[] BOOKMARK_DEFAULT_PROJECTION = new String[] {
            BookmarkColumns._ID, BookmarkColumns.URL, BookmarkColumns.VISITS,
            BookmarkColumns.DATE, BookmarkColumns.BOOKMARK, BookmarkColumns.TITLE,
            BookmarkColumns.FAVICON, BookmarkColumns.CREATED};

    private static final String[] SUGGEST_PROJECTION = new String[] {
        BookmarkColumns._ID,
        BookmarkColumns.TITLE,
        BookmarkColumns.URL,
        BookmarkColumns.DATE,
        BookmarkColumns.BOOKMARK
    };

    // Columns for the bookmark folder URL queries.
    public static final int BOOKMARK_FOLDER_COLUMN_INDEX_ID = 0;
    public static final int BOOKMARK_FOLDER_COLUMN_INDEX_URL = 1;
    public static final int BOOKMARK_FOLDER_COLUMN_INDEX_TITLE = 2;
    public static final int BOOKMARK_FOLDER_COLUMN_INDEX_FAVICON = 3;
    public static final int BOOKMARK_FOLDER_COLUMN_INDEX_THUMBNAIL = 4;
    public static final int BOOKMARK_FOLDER_COLUMN_INDEX_PARENT_ID = 5;
    public static final int BOOKMARK_FOLDER_COLUMN_INDEX_IS_FOLDER = 6;

    private static final String BOOKMARK_FOLDER_COLUMN_ID = BookmarkColumns._ID;
    private static final String BOOKMARK_FOLDER_COLUMN_URL = BookmarkColumns.URL;
    private static final String BOOKMARK_FOLDER_COLUMN_TITLE = BookmarkColumns.TITLE;
    private static final String BOOKMARK_FOLDER_COLUMN_FAVICON = BookmarkColumns.FAVICON;
    private static final String BOOKMARK_FOLDER_COLUMN_THUMBNAIL = "thumbnail";
    private static final String BOOKMARK_FOLDER_COLUMN_PARENT_ID = "parent_id";
    private static final String BOOKMARK_FOLDER_COLUMN_IS_FOLDER = "is_folder";

    private static final String[] BOOKMARK_FOLDER_DEFAULT_PROJECTION = new String[] {
        BOOKMARK_FOLDER_COLUMN_ID,
        BOOKMARK_FOLDER_COLUMN_URL,
        BOOKMARK_FOLDER_COLUMN_TITLE,
        BOOKMARK_FOLDER_COLUMN_PARENT_ID,
        BOOKMARK_FOLDER_COLUMN_IS_FOLDER,
        BOOKMARK_FOLDER_COLUMN_FAVICON,
        BOOKMARK_FOLDER_COLUMN_THUMBNAIL
    };

    synchronized private void ensureUriMatcherInitialized() {
        if (mUriMatcher != null) {
            return;
        }
        mUriMatcher = new UriMatcher(UriMatcher.NO_MATCH);
        // The internal URIs
        String authority = getContext().getPackageName() + AUTHORITY_SUFFIX;
        mUriMatcher.addURI(authority, BOOKMARKS_PATH, URI_MATCH_BOOKMARKS);
        mUriMatcher.addURI(authority, BOOKMARKS_PATH + "/#", URI_MATCH_BOOKMARKS_ID);
        mUriMatcher.addURI(authority, BOOKMARK_FOLDER_PATH + "/#", URL_MATCH_BOOKMARK_FOLDER_ID);
        // The internal authority for public APIs
        String apiAuthority = getContext().getPackageName() + API_AUTHORITY_SUFFIX;
        mUriMatcher.addURI(apiAuthority, BOOKMARKS_PATH, URL_MATCH_API_BOOKMARK);
        mUriMatcher.addURI(apiAuthority, BOOKMARKS_PATH + "/#", URL_MATCH_API_BOOKMARK_ID);
        mUriMatcher.addURI(apiAuthority, SEARCHES_PATH, URL_MATCH_API_SEARCHES);
        mUriMatcher.addURI(apiAuthority, SEARCHES_PATH + "/#", URL_MATCH_API_SEARCHES_ID);
        mUriMatcher.addURI(apiAuthority, HISTORY_PATH, URL_MATCH_API_HISTORY_CONTENT);
        mUriMatcher.addURI(apiAuthority, HISTORY_PATH + "/#", URL_MATCH_API_HISTORY_CONTENT_ID);
        mUriMatcher.addURI(apiAuthority, COMBINED_PATH, URL_MATCH_API_BOOKMARK);
        mUriMatcher.addURI(apiAuthority, COMBINED_PATH + "/#", URL_MATCH_API_BOOKMARK_ID);
        // The internal authority for BrowserContracts
        mUriMatcher.addURI(BROWSER_CONTRACT_API_AUTHORITY, HISTORY_PATH,
                URL_MATCH_API_HISTORY_CONTENT);
        mUriMatcher.addURI(BROWSER_CONTRACT_API_AUTHORITY, HISTORY_PATH + "/#",
                URL_MATCH_API_HISTORY_CONTENT_ID);
        mUriMatcher.addURI(BROWSER_CONTRACT_API_AUTHORITY, COMBINED_PATH, URL_MATCH_API_BOOKMARK);
        mUriMatcher.addURI(BROWSER_CONTRACT_API_AUTHORITY, COMBINED_PATH + "/#",
                URL_MATCH_API_BOOKMARK_ID);
        mUriMatcher.addURI(BROWSER_CONTRACT_API_AUTHORITY, SEARCHES_PATH, URL_MATCH_API_SEARCHES);
        mUriMatcher.addURI(BROWSER_CONTRACT_API_AUTHORITY, SEARCHES_PATH + "/#",
                URL_MATCH_API_SEARCHES_ID);
        mUriMatcher.addURI(BROWSER_CONTRACT_API_AUTHORITY, BOOKMARKS_PATH,
                URL_MATCH_API_BOOKMARK_CONTENT);
        mUriMatcher.addURI(BROWSER_CONTRACT_API_AUTHORITY, BOOKMARKS_PATH + "/#",
                URL_MATCH_API_BOOKMARK_CONTENT_ID);
        // Added the Android Framework URIs, so the provider can easily switched
        // by adding 'browser' and 'com.android.browser' in manifest.
        // The Android's BrowserContract
        mUriMatcher.addURI(BROWSER_CONTRACT_AUTHORITY, HISTORY_PATH, URL_MATCH_API_HISTORY_CONTENT);
        mUriMatcher.addURI(BROWSER_CONTRACT_AUTHORITY, HISTORY_PATH + "/#",
                URL_MATCH_API_HISTORY_CONTENT_ID);
        mUriMatcher.addURI(BROWSER_CONTRACT_AUTHORITY, "combined", URL_MATCH_API_BOOKMARK);
        mUriMatcher.addURI(BROWSER_CONTRACT_AUTHORITY, "combined/#", URL_MATCH_API_BOOKMARK_ID);
        mUriMatcher.addURI(BROWSER_CONTRACT_AUTHORITY, SEARCHES_PATH, URL_MATCH_API_SEARCHES);
        mUriMatcher.addURI(BROWSER_CONTRACT_AUTHORITY, SEARCHES_PATH + "/#",
                URL_MATCH_API_SEARCHES_ID);
        mUriMatcher.addURI(BROWSER_CONTRACT_AUTHORITY, BOOKMARKS_PATH,
                URL_MATCH_API_BOOKMARK_CONTENT);
        mUriMatcher.addURI(BROWSER_CONTRACT_AUTHORITY, BOOKMARKS_PATH + "/#",
                URL_MATCH_API_BOOKMARK_CONTENT_ID);
        // For supporting android.provider.browser.BookmarkColumns and
        // SearchColumns
        mUriMatcher.addURI("browser", BOOKMARKS_PATH, URL_MATCH_API_BOOKMARK);
        mUriMatcher.addURI("browser", BOOKMARKS_PATH + "/#", URL_MATCH_API_BOOKMARK_ID);
        mUriMatcher.addURI("browser", SEARCHES_PATH, URL_MATCH_API_SEARCHES);
        mUriMatcher.addURI("browser", SEARCHES_PATH + "/#", URL_MATCH_API_SEARCHES_ID);

        mUriMatcher.addURI(apiAuthority,
                BOOKMARKS_PATH + "/" + SearchManager.SUGGEST_URI_PATH_QUERY,
                URL_MATCH_BOOKMARK_SUGGESTIONS_ID);
        mUriMatcher.addURI(apiAuthority,
                SearchManager.SUGGEST_URI_PATH_QUERY,
                URL_MATCH_BOOKMARK_HISTORY_SUGGESTIONS_ID);
    }

    // It is set in nativeInit()
    private int mNativeChromeBrowserProvider;

    @Override
    public boolean onCreate() {
        if (!CommandLine.isInitialized()) {
            CommandLine.initFromFile();
        }

        SharedPreferences sharedPreferences =
                PreferenceManager.getDefaultSharedPreferences(getContext());
        mLastModifiedBookmarkFolderId = sharedPreferences.getLong(
                LAST_MODIFIED_BOOKMARK_FOLDER_ID_KEY, INVALID_BOOKMARK_ID);
        ChromeNotificationCenter.registerForNotification(
                ChromeNotificationCenter.NATIVE_LIBRARY_READY,
                new ChromeNotificationCenter.ChromeNotificationHandler() {
                    @Override
                    public void handleMessage(Message msg) {
                        if (ensureNativeChromeLoadedOnUIThread()) {
                            ChromeNotificationCenter.unregisterForNotification(
                                    ChromeNotificationCenter.NATIVE_LIBRARY_READY, this);
                        }
                    }
                });
        return true;
    }

    private String buildSuggestWhere(String selection, int argc) {
        StringBuilder sb = new StringBuilder(selection);
        for (int i = 0; i < argc - 1; i++) {
            sb.append(" OR ");
            sb.append(selection);
        }
        return sb.toString();
    }

    private Cursor getBookmarkHistorySuggestions(String selection, String[] selectionArgs,
            String sortOrder, boolean excludeHistory) {
        boolean matchTitles = false;
        Vector<String> args = new Vector();
        String like = selectionArgs[0] + "%";
        if (selectionArgs[0].startsWith("http") || selectionArgs[0].startsWith("file")) {
            args.add(like);
        } else {
            // Match against common URL prefixes.
            args.add("http://" + like);
            args.add("https://" + like);
            args.add("http://www." + like);
            args.add("https://www." + like);
            args.add("file://" + like);
            matchTitles = true;
        }

        StringBuilder urlWhere = new StringBuilder("(");
        urlWhere.append(buildSuggestWhere(selection, args.size()));
        if (matchTitles) {
            args.add(like);
            urlWhere.append(" OR title LIKE ?");
        }
        urlWhere.append(")");

        if (excludeHistory) {
            urlWhere.append(" AND bookmark=?");
            args.add("1");
        }

        selectionArgs = args.toArray(selectionArgs);
        Cursor cursor = queryBookmarkFromAPI(SUGGEST_PROJECTION, urlWhere.toString(),
                selectionArgs, sortOrder);
        return new ChromeBrowserProviderSuggestionsCursor(cursor);
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs,
            String sortOrder) {
        if (!ensureNativeChromeLoaded()) return null;

        int match = mUriMatcher.match(uri);
        Cursor cursor = null;
        switch (match) {
            case URL_MATCH_BOOKMARK_SUGGESTIONS_ID:
                cursor = getBookmarkHistorySuggestions(selection, selectionArgs, sortOrder, true);
                break;
            case URL_MATCH_BOOKMARK_HISTORY_SUGGESTIONS_ID:
                cursor = getBookmarkHistorySuggestions(selection, selectionArgs, sortOrder, false);
                break;
            case URL_MATCH_API_BOOKMARK:
                cursor = queryBookmarkFromAPI(projection, selection, selectionArgs, sortOrder);
                break;
            case URL_MATCH_API_BOOKMARK_ID:
                cursor = queryBookmarkFromAPI(projection,
                        buildWhereClause(ContentUris.parseId(uri), selection), selectionArgs,
                        sortOrder);
                break;
            case URL_MATCH_API_SEARCHES:
                cursor = querySearchTermFromAPI(projection, selection, selectionArgs, sortOrder);
                break;
            case URL_MATCH_API_SEARCHES_ID:
                cursor = querySearchTermFromAPI(projection,
                        buildWhereClause(ContentUris.parseId(uri), selection), selectionArgs,
                        sortOrder);
                break;
            case URL_MATCH_API_HISTORY_CONTENT:
                cursor = queryBookmarkFromAPI(projection, buildHistoryWhereClause(selection),
                        selectionArgs, sortOrder);
                break;
            case URL_MATCH_API_HISTORY_CONTENT_ID:
                cursor = queryBookmarkFromAPI(projection,
                        buildHistoryWhereClause(ContentUris.parseId(uri), selection),
                        selectionArgs, sortOrder);
                break;
            case URL_MATCH_API_BOOKMARK_CONTENT:
                cursor = queryBookmarkFromAPI(projection, buildBookmarkWhereClause(selection),
                        selectionArgs, sortOrder);
                break;
            case URL_MATCH_API_BOOKMARK_CONTENT_ID:
                cursor = queryBookmarkFromAPI(projection,
                        buildBookmarkWhereClause(ContentUris.parseId(uri), selection),
                        selectionArgs, sortOrder);
                break;
            case URL_MATCH_BOOKMARK_FOLDER_ID:
                // In this special case the query params are ignored except the id.
                // This is because this URL is not really querying a database, but exporting
                // node data since OS limitations enforce us to use the ContentProvider API.
                cursor = queryBookmarkFolder(ContentUris.parseId(uri));
                break;
            default:
                throw new IllegalArgumentException(
                        "Chrome Provider query - unknown URL uri = " + uri);
        }
        if (cursor == null) {
            cursor = new MatrixCursor(new String[] { });
        }
        cursor.setNotificationUri(getContext().getContentResolver(), uri);
        return cursor;
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        if (!ensureNativeChromeLoaded()) {
            throw new IllegalStateException("Unable to initialize Chrome browser process");
        }

        int match = mUriMatcher.match(uri);
        Uri res = null;
        long id;
        switch (match) {
            case URI_MATCH_BOOKMARKS:
                id = addBookmark(values);
                break;
            case URL_MATCH_API_BOOKMARK_CONTENT:
                values.put(BookmarkColumns.BOOKMARK, 1);
                //$FALL-THROUGH$
            case URL_MATCH_API_BOOKMARK:
            case URL_MATCH_API_HISTORY_CONTENT:
                id = addBookmarkFromAPI(values);
                break;
            case URL_MATCH_API_SEARCHES:
                id = addSearchTermFromAPI(values);
                break;
            case URL_MATCH_BOOKMARK_FOLDER_ID:
                throw new UnsupportedOperationException(
                        "Only queries allowed for the bookmark hierarchy URI");
            default:
                throw new IllegalArgumentException(
                        "ChromeProvider: insert - unknown URL " + uri);
        }
        if (id == 0) {
            return null;
        }

        res = ContentUris.withAppendedId(uri, id);
        getContext().getContentResolver().notifyChange(res, null);
        return res;
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        if (!ensureNativeChromeLoaded()) return 0;

        int match = mUriMatcher.match(uri);
        int result;
        switch (match) {
            case URI_MATCH_BOOKMARKS_ID :
                long bookmarkId = Long.parseLong(uri.getPathSegments().get(1));
                nativeRemoveBookmark(mNativeChromeBrowserProvider, bookmarkId);
                // TODO : Make the nativeRemoveBookmark return int, so we don't
                // always return '1' here.
                result = 1;
                break;
            case URL_MATCH_API_BOOKMARK_ID:
                result = removeBookmarkFromAPI(
                        buildWhereClause(ContentUris.parseId(uri), selection), selectionArgs);
                break;
            case URL_MATCH_API_BOOKMARK:
                result = removeBookmarkFromAPI(selection, selectionArgs);
                break;
            case URL_MATCH_API_SEARCHES_ID:
                result = removeSearchFromAPI(buildWhereClause(ContentUris.parseId(uri), selection),
                        selectionArgs);
                break;
            case URL_MATCH_API_SEARCHES:
                result = removeSearchFromAPI(selection, selectionArgs);
                break;
            case URL_MATCH_API_HISTORY_CONTENT:
                result = removeHistoryFromAPI(selection, selectionArgs);
                break;
            case URL_MATCH_API_HISTORY_CONTENT_ID:
                result = removeHistoryFromAPI(buildWhereClause(ContentUris.parseId(uri), selection),
                        selectionArgs);
                break;
            case URL_MATCH_API_BOOKMARK_CONTENT:
                result = removeBookmarkFromAPI(buildBookmarkWhereClause(selection), selectionArgs);
                break;
            case URL_MATCH_API_BOOKMARK_CONTENT_ID:
                result = removeBookmarkFromAPI(
                        buildBookmarkWhereClause(ContentUris.parseId(uri), selection),
                        selectionArgs);
                break;
            case URL_MATCH_BOOKMARK_FOLDER_ID:
                throw new UnsupportedOperationException(
                        "Only queries allowed for the bookmark hierarchy URI");
            default:
                throw new IllegalArgumentException(
                        "ChromeProvider delete - unknown URL " + uri);
        }
        if (result != 0) {
            getContext().getContentResolver().notifyChange(uri, null);
        }
        return result;
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
        if (!ensureNativeChromeLoaded()) return 0;

        int match = mUriMatcher.match(uri);
        int result;
        switch (match) {
            case URI_MATCH_BOOKMARKS_ID:
                long bookmarkId = ContentUris.parseId(uri);
                String url = null;
                if (values.containsKey(Browser.BookmarkColumns.URL)) {
                    url = values.getAsString(Browser.BookmarkColumns.URL);
                }
                String title = values.getAsString(Browser.BookmarkColumns.TITLE);
                long parentId = INVALID_BOOKMARK_ID;
                if (values.containsKey(BOOKMARK_PARENT_ID_PARAM)) {
                    parentId = values.getAsLong(BOOKMARK_PARENT_ID_PARAM);
                }
                nativeUpdateBookmark(mNativeChromeBrowserProvider, bookmarkId, url, title, parentId);
                updateLastModifiedBookmarkFolder(parentId);
                result = 1;
                break;
            case URL_MATCH_API_BOOKMARK_ID:
                result = updateBookmarkFromAPI(values,
                        buildWhereClause(ContentUris.parseId(uri), selection), selectionArgs);
                break;
            case URL_MATCH_API_BOOKMARK:
                result = updateBookmarkFromAPI(values, selection, selectionArgs);
                break;
            case URL_MATCH_API_SEARCHES_ID:
                result = updateSearchTermFromAPI(values,
                        buildWhereClause(ContentUris.parseId(uri), selection), selectionArgs);
                break;
            case URL_MATCH_API_SEARCHES:
                result = updateSearchTermFromAPI(values, selection, selectionArgs);
                break;
            case URL_MATCH_API_HISTORY_CONTENT:
                result = updateBookmarkFromAPI(values, buildHistoryWhereClause(selection),
                        selectionArgs);
                break;
            case URL_MATCH_API_HISTORY_CONTENT_ID:
                result = updateBookmarkFromAPI(values,
                        buildHistoryWhereClause(ContentUris.parseId(uri), selection), selectionArgs);
                break;
            case URL_MATCH_API_BOOKMARK_CONTENT:
                result = updateBookmarkFromAPI(values, buildBookmarkWhereClause(selection),
                        selectionArgs);
                break;
            case URL_MATCH_API_BOOKMARK_CONTENT_ID:
                result = updateBookmarkFromAPI(values,
                        buildBookmarkWhereClause(ContentUris.parseId(uri), selection),
                        selectionArgs);
                break;
            case URL_MATCH_BOOKMARK_FOLDER_ID:
                throw new UnsupportedOperationException(
                        "Only queries allowed for the bookmark hierarchy URI");
            default:
                throw new IllegalArgumentException(
                        "Chrome Provider update - unknown URL " + uri);
        }
        if (result != 0) {
            getContext().getContentResolver().notifyChange(uri, null);
        }
        return result;
    }

    @Override
    public String getType(Uri uri) {
        ensureUriMatcherInitialized();
        int match = mUriMatcher.match(uri);
        switch (match) {
            case URI_MATCH_BOOKMARKS:
            case URL_MATCH_API_BOOKMARK:
                return "vnd.android.cursor.dir/bookmark";
            case URI_MATCH_BOOKMARKS_ID:
            case URL_MATCH_API_BOOKMARK_ID:
                return "vnd.android.cursor.item/bookmark";
            case URL_MATCH_API_SEARCHES:
                return "vnd.android.cursor.dir/searches";
            case URL_MATCH_API_SEARCHES_ID:
                return "vnd.android.cursor.item/searches";
            case URL_MATCH_API_HISTORY_CONTENT:
                return BROWSER_CONTRACT_HISTORY_CONTENT_TYPE;
            case URL_MATCH_API_HISTORY_CONTENT_ID:
                return BROWSER_CONTRACT_HISTORY_CONTENT_ITEM_TYPE;
            default:
                throw new IllegalArgumentException(
                        "ChromeProvider - getType - unknown URL " + uri);
        }
    }

    private long addBookmark(ContentValues values) {
        String url = values.getAsString(Browser.BookmarkColumns.URL);
        String title = values.getAsString(Browser.BookmarkColumns.TITLE);
        boolean isFolder = false;
        if (values.containsKey(BOOKMARK_IS_FOLDER_PARAM)) {
            isFolder = values.getAsBoolean(BOOKMARK_IS_FOLDER_PARAM);
        }
        long parentId = INVALID_BOOKMARK_ID;
        if (values.containsKey(BOOKMARK_PARENT_ID_PARAM)) {
            parentId = values.getAsLong(BOOKMARK_PARENT_ID_PARAM);
        }
        long id = nativeAddBookmark(mNativeChromeBrowserProvider, url, title, isFolder, parentId);
        if (isFolder) {
            updateLastModifiedBookmarkFolder(id);
        } else {
            updateLastModifiedBookmarkFolder(parentId);
        }
        return id;
    }

    @Override
    protected void finalize() {
        if (mNativeChromeBrowserProvider != 0) {
            nativeDestroy(mNativeChromeBrowserProvider);
            mNativeChromeBrowserProvider = 0;
        }
    }

    private void updateLastModifiedBookmarkFolder(long id) {
        if (mLastModifiedBookmarkFolderId == id) return;

        mLastModifiedBookmarkFolderId = id;
        SharedPreferences sharedPreferences =
                PreferenceManager.getDefaultSharedPreferences(getContext());
        sharedPreferences.edit()
                .putLong(LAST_MODIFIED_BOOKMARK_FOLDER_ID_KEY, mLastModifiedBookmarkFolderId)
                .apply();
    }

    /**
     * Creates a bookmark folder or returns its id if it already exists.
     * This method does not update the last modified folder in the UI.
     *
     * @return The id of the new created folder (or INVALID_BOOKMARK_ID on error).
     *         Will return the id of any existing folder in the same parent with the same name.
     */
    public static long createBookmarksFolderOnce(String title, long parentId) {
        return nativeCreateBookmarksFolderOnce(title, parentId);
    }

    public static Uri getBookmarksUri(Context context) {
        return buildContentUri(context.getPackageName() + AUTHORITY_SUFFIX, BOOKMARKS_PATH);
    }

    public static Uri getBookmarkFolderUri(Context context) {
        return buildContentUri(context.getPackageName() + AUTHORITY_SUFFIX,
                BOOKMARK_FOLDER_PATH);
    }

    public static Uri getBookmarksApiUri(Context context) {
        return buildContentUri(context.getPackageName() + API_AUTHORITY_SUFFIX, BOOKMARKS_PATH);
    }

    public static Uri getSearchesApiUri(Context context) {
        return buildContentUri(context.getPackageName() + API_AUTHORITY_SUFFIX, SEARCHES_PATH);
    }

    /**
     * @return The id of the Mobile Bookmarks folder.
     */
    public static long getMobileBookmarksFolderId() {
        BookmarkNode mobileBookmarks = getMobileBookmarksFolder();
        return mobileBookmarks != null ? mobileBookmarks.mId : INVALID_BOOKMARK_ID;
    }

    /**
     * Ensures that the bookmarks model has been loaded in order to avoid race conditions
     * in other static methods. MUST not be called from the UI thread.
     */
    public static void ensureBookmarksModelIsLoaded() {
        nativeEnsureBookmarksModelIsLoaded();
    }

    /**
     * @return True if the provided bookmark node id is a descendant of the Mobile Bookmarks folder.
     */
    public static boolean isMobileBookmarksDescendant(long nodeId) {
        if (nodeId <= 0) return false;
        return nativeIsMobileBookmarksDescendant(nodeId);
    }

    /**
     * Get a bookmarked node for the specified ID or null if no such node exists.  If the
     * Any children of the specified node are also fetched by this call, but not any deeper.
     * The attributes of this node and its parent are also populated.
     *
     * @param nodeId The ID of the entry to be retrieved.
     * @return The entry corresponding to the ID given.
     */
    protected static BookmarkNode getBookmarkNodeWithChildren(long nodeId) {
        return nativeGetBookmarkNodeWithChildren(nodeId);
    }

    /**
     * @return The default bookmark folder (or null on error) for new bookmarks.  The underlying
     *         children will not be populated in the returned node.
     */
    protected static BookmarkNode getDefaultBookmarkFolder() {
        // Try to access the bookmark folder last modified by us. If it doesn't exist anymore
        // then use the synced node (Mobile Bookmarks).
        BookmarkNode lastModified = nativeGetBookmarkNode(mLastModifiedBookmarkFolderId);
        if (lastModified == null) {
            lastModified = getMobileBookmarksFolder();
            mLastModifiedBookmarkFolderId = lastModified != null ? lastModified.id() :
                    INVALID_BOOKMARK_ID;
        }
        return lastModified;
    }

    /**
     * @return The Mobile Bookmarks folder or null on error. The underlying children will not be
     *         populated in the returned node.
     */
    protected static BookmarkNode getMobileBookmarksFolder() {
        return nativeGetMobileBookmarksFolder();
    }

    /**
     * @return The root node of the bookmark folder hierarchy with all descendants populated.
     */
    protected static BookmarkNode getBookmarkFolderHierarchy() {
        return nativeGetAllBookmarkFolders();
    }

    /**
     * Get a bookmarked node for the specified ID or null if no such node exists.  If the
     * specified node contains any children, they are not fetched as part of this call.  Only
     * the attributes of this node and it's parent are populated.
     *
     * @param nodeId The ID of the entry to be retrieved.
     * @return The entry corresponding to the ID given.
     */
    protected static BookmarkNode getBookmarkNode(long nodeId) {
        return nativeGetBookmarkNode(nodeId);
    }

    /**
     * The type of a BookmarkNode
     */
    public enum Type {
        URL,
        FOLDER,
        BOOKMARK_BAR,
        OTHER_NODE,
        MOBILE
    }

    /**
     * Simple Data Object representing the chrome bookmark node.
     */
    static class BookmarkNode {
        private final long mId;
        private final String mName;
        private final String mUrl;
        private final Type mType;
        private final BookmarkNode mParent;
        private final List<BookmarkNode> mChildren = new ArrayList<BookmarkNode>();

        /** Used to pass structured data back from the native code. */
        private BookmarkNode(long id, int type, String name, String url, BookmarkNode parent) {
            mId = id;
            mName = name;
            mUrl = url;
            mType = Type.values()[type];
            mParent = parent;
        }

        /**
         * @return The id of this bookmark entry.
         */
        public long id() {
            return mId;
        }

        /**
         * @return The name of this bookmark entry.
         */
        public String name() {
            return mName;
        }

        /**
         * @return The URL of this bookmark entry.
         */
        public String url() {
            return mUrl;
        }

        /**
         * @return The type of this bookmark entry.
         */
        public Type type() {
            return mType;
        }

        /**
         * @return The parent folder of this bookmark entry.
         */
        public BookmarkNode parent() {
            return mParent;
        }

        /**
         * Adds a child to this node.
         *
         * <p>
         * Used solely by the native code.
         */
        @SuppressWarnings("unused")
        @CalledByNativeUnchecked("BookmarkNode")
        private void addChild(BookmarkNode child) {
            mChildren.add(child);
        }

        /**
         * @return The child bookmark nodes of this node.
         */
        public List<BookmarkNode> children() {
            return mChildren;
        }

        /**
         * @return Whether this node represents a bookmarked URL or not.
         */
        public boolean isUrl() {
            return mUrl != null;
        }

        @SuppressWarnings("unused")
        @CalledByNative("BookmarkNode")
        private static BookmarkNode create(
                long id, int type, String name, String url, BookmarkNode parent) {
            return new BookmarkNode(id, type, name, url, parent);
        }
    }

    private long addBookmarkFromAPI(ContentValues values) {
        if (IsInUIThread()) {
            // We can not run in UI thread, do nothing for official release.
            return 0;
        }
        BookmarkRow row = BookmarkRow.fromContentValues(values);
        if (row.url == null) {
            throw new IllegalArgumentException("Must have a bookmark URL");
        }
        return nativeAddBookmarkFromAPI(mNativeChromeBrowserProvider,
                row.url, row.created, row.isBookmark, row.date, row.favicon,
                row.title, row.visits, row.parentId);
    }

    private Cursor queryBookmarkFromAPI(String[] projectionIn, String selection,
            String[] selectionArgs, String sortOrder) {
        if (IsInUIThread()) {
            // We can not run in UI thread, do nothing for official release.
            return null;
        }
        String[] projection = null;
        if (projectionIn == null || projectionIn.length == 0) {
            projection = BOOKMARK_DEFAULT_PROJECTION;
        } else {
            projection = projectionIn;
        }

        return nativeQueryBookmarkFromAPI(mNativeChromeBrowserProvider, projection, selection,
                selectionArgs, sortOrder);
    }

    private int updateBookmarkFromAPI(ContentValues values, String selection,
            String[] selectionArgs) {
        if (IsInUIThread()) {
            // We can not run in UI thread, do nothing for official release.
            return 0;
        }
        BookmarkRow row = BookmarkRow.fromContentValues(values);
        return nativeUpdateBookmarkFromAPI(mNativeChromeBrowserProvider,
                row.url, row.created, row.isBookmark, row.date,
                row.favicon, row.title, row.visits, row.parentId, selection, selectionArgs);
    }

    private int removeBookmarkFromAPI(String selection, String[] selectionArgs) {
        if (IsInUIThread()) {
            // We can not run in UI thread, do nothing for official release.
            return 0;
        }
        return nativeRemoveBookmarkFromAPI(mNativeChromeBrowserProvider, selection, selectionArgs);
    }

    private int removeHistoryFromAPI(String selection, String[] selectionArgs) {
        if (IsInUIThread()) {
            // We can not run in UI thread, do nothing for official release.
            return 0;
        }
        return nativeRemoveHistoryFromAPI(mNativeChromeBrowserProvider, selection, selectionArgs);
    }

    private Cursor queryBookmarkFolder(long id) {
        if (IsInUIThread()) {
            // We can not run in UI thread, do nothing for official release.
            return null;
        }

        BookmarkNode node = getBookmarkNodeWithChildren(id);
        if (node == null) return null;

        // Don't allow going up the hierarchy if sync is disabled and the requested node
        // is the Mobile Bookmarks folder.
        boolean filterParent = node.type() == Type.MOBILE &&
                !SyncStatusHelper.get(getContext()).isSyncEnabled();

        MatrixCursor cursor = new MatrixCursor(BOOKMARK_FOLDER_DEFAULT_PROJECTION);
        cursor.addRow(getBookmarkNodeAsFolderRow(node, filterParent));

        for (BookmarkNode child : node.children()) {
            cursor.addRow(getBookmarkNodeAsFolderRow(child, false));
        }

        cursor.moveToFirst();
        return cursor;
    }

    /**
     * Returns the contents of a bookmark node encoded as an object array.
     * Child nodes are not encoded. This method requires the initialization of the native side.
     *
     * @param noParent Flag indicating if the parent of this node should be ignored.
     * @return The contents of the node encoded as an object array defining a row of
     * BOOKMARK_FOLDER_DEFAULT_PROJECTION.
     */
    private Object[] getBookmarkNodeAsFolderRow(BookmarkNode node, boolean noParent) {
        byte[] favicon = null;
        byte[] thumbnail = null;

        if (node.type() == Type.URL) {
            favicon = nativeGetFaviconOrTouchIcon(mNativeChromeBrowserProvider, node.url());
            thumbnail = nativeGetThumbnail(mNativeChromeBrowserProvider, node.url());
        }

        return new Object[] {
            new Long(node.id()),
            node.url(),
            node.name(),
            favicon,
            thumbnail,
            new Long(noParent || node.parent() == null ? INVALID_BOOKMARK_ID : node.parent().id()),
            new Integer(node.type() != Type.URL ? 1 : 0),
        };
    }

    private void onBookmarkChanged() {
        getContext().getContentResolver().notifyChange(
                buildAPIContentUri(getContext(), BOOKMARKS_PATH), null);
    }

    private void onSearchTermChanged() {
        getContext().getContentResolver().notifyChange(
                buildAPIContentUri(getContext(), SEARCHES_PATH), null);
    }

    private long addSearchTermFromAPI(ContentValues values) {
        if (IsInUIThread()) {
            // We can not run in UI thread, do nothing for official release.
            return 0;
        }
        SearchRow row = SearchRow.fromContentValues(values);
        if (row.term == null) {
            throw new IllegalArgumentException("Must have a search term");
        }
        return nativeAddSearchTermFromAPI(mNativeChromeBrowserProvider, row.term, row.date);
    }

    private int updateSearchTermFromAPI(ContentValues values, String selection,
            String[] selectionArgs) {
        if (IsInUIThread()) {
            // We can not run in UI thread, do nothing for official release.
            return 0;
        }
        SearchRow row = SearchRow.fromContentValues(values);
        return nativeUpdateSearchTermFromAPI(mNativeChromeBrowserProvider,
                row.term, row.date, selection, selectionArgs);
    }

    private Cursor querySearchTermFromAPI(String[] projectionIn, String selection,
            String[] selectionArgs, String sortOrder) {
        if (IsInUIThread()) {
            // We can not run in UI thread, do nothing for official release.
            return null;
        }
        String[] projection = null;
        if (projectionIn == null || projectionIn.length == 0) {
            projection = android.provider.Browser.SEARCHES_PROJECTION;
        } else {
            projection = projectionIn;
        }
        return nativeQuerySearchTermFromAPI(mNativeChromeBrowserProvider, projection, selection,
                selectionArgs, sortOrder);
    }

    private int removeSearchFromAPI(String selection, String[] selectionArgs) {
        if (IsInUIThread()) {
            // We can not run in UI thread, do nothing for official release.
            return 0;
        }
        return nativeRemoveSearchTermFromAPI(mNativeChromeBrowserProvider,
                selection, selectionArgs);
    }

    /**
     * @return false if we are not in UI thread, otherwise true is returned in
     *         official release or IllegalStateException is thrown in other
     *         builds.
     */
    private boolean IsInUIThread() {
        if (Looper.myLooper() != Looper.getMainLooper()) {
            return false;
        }
        if (!"REL".equals(Build.VERSION.CODENAME)) {
            throw new IllegalStateException("Shouldn't run in UI thread");
        }
        Log.e(TAG, "Shouldn't run in UI thread");
        return true;
    }

    private static Uri buildContentUri(String authority, String path) {
        return Uri.parse("content://" + authority + "/" + path);
    }

    private static Uri buildAPIContentUri(Context context, String path) {
        return buildContentUri(context.getPackageName() + API_AUTHORITY_SUFFIX, path);
    }

    private static String buildWhereClause(long id, String selection) {
        StringBuffer sb = new StringBuffer();
        sb.append(BaseColumns._ID);
        sb.append(" = ");
        sb.append(id);
        if (!TextUtils.isEmpty(selection)) {
            sb.append(" AND (");
            sb.append(selection);
            sb.append(")");
        }
        return sb.toString();
    }

    private static String buildHistoryWhereClause(long id, String selection) {
        return buildWhereClause(id, buildBookmarkWhereClause(selection, false));
    }

    private static String buildHistoryWhereClause(String selection) {
        return buildBookmarkWhereClause(selection, false);
    }

    /**
     * @return a SQL where class which is inserted the bookmark condition.
     */
    private static String buildBookmarkWhereClause(String selection, boolean is_bookmark) {
        StringBuffer sb = new StringBuffer();
        sb.append(BookmarkColumns.BOOKMARK);
        sb.append(is_bookmark ? " = 1 " : " = 0");
        if (!TextUtils.isEmpty(selection)) {
            sb.append(" AND (");
            sb.append(selection);
            sb.append(")");
        }
        return sb.toString();
    }

    private static String buildBookmarkWhereClause(long id, String selection) {
        return buildWhereClause(id, buildBookmarkWhereClause(selection, true));
    }

    private static String buildBookmarkWhereClause(String selection) {
        return buildBookmarkWhereClause(selection, true);
    }

    // Wrap the value of BookmarkColumn.
    private static class BookmarkRow {
        Boolean isBookmark;
        Long created;
        String url;
        Long date;
        byte[] favicon;
        String title;
        Integer visits;
        long parentId;

        static BookmarkRow fromContentValues(ContentValues values) {
            BookmarkRow row = new BookmarkRow();
            if (values.containsKey(BookmarkColumns.URL)) {
                row.url = values.getAsString(BookmarkColumns.URL);
            }
            if (values.containsKey(BookmarkColumns.BOOKMARK)) {
                row.isBookmark = values.getAsInteger(BookmarkColumns.BOOKMARK) != 0;
            }
            if (values.containsKey(BookmarkColumns.CREATED)) {
                row.created = values.getAsLong(BookmarkColumns.CREATED);
            }
            if (values.containsKey(BookmarkColumns.DATE)) {
                row.date = values.getAsLong(BookmarkColumns.DATE);
            }
            if (values.containsKey(BookmarkColumns.FAVICON)) {
                row.favicon = values.getAsByteArray(BookmarkColumns.FAVICON);
                // We need to know that the caller set the favicon column.
                if (row.favicon == null) {
                    row.favicon = new byte[0];
                }
            }
            if (values.containsKey(BookmarkColumns.TITLE)) {
                row.title = values.getAsString(BookmarkColumns.TITLE);
            }
            if (values.containsKey(BookmarkColumns.VISITS)) {
                row.visits = values.getAsInteger(BookmarkColumns.VISITS);
            }
            if (values.containsKey(BOOKMARK_PARENT_ID_PARAM)) {
                row.parentId = values.getAsLong(BOOKMARK_PARENT_ID_PARAM);
            }
            return row;
        }
    }

    // Wrap the value of SearchColumn.
    private static class SearchRow {
        String term;
        Long date;

        static SearchRow fromContentValues(ContentValues values) {
            SearchRow row = new SearchRow();
            if (values.containsKey(SearchColumns.SEARCH)) {
                row.term = values.getAsString(SearchColumns.SEARCH);
            }
            if (values.containsKey(SearchColumns.DATE)) {
                row.date = values.getAsLong(SearchColumns.DATE);
            }
            return row;
        }
    }

    /**
     * Make sure chrome is running. This method mustn't run on UI thread.
     *
     * @return Whether the native chrome process is running successfully once this has returned.
     */
    private synchronized boolean ensureNativeChromeLoaded() {
        ensureUriMatcherInitialized();

        if (mNativeChromeBrowserProvider != 0) return true;

        final AtomicBoolean retVal = new AtomicBoolean(true);
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
              retVal.set(ensureNativeChromeLoadedOnUIThread());
            }
        });
        return retVal.get();
    }

  /**
   * This method should only run on UI thread.
   */
  private boolean ensureNativeChromeLoadedOnUIThread() {
      if (mNativeChromeBrowserProvider != 0) return true;
      if (((ChromeMobileApplication) getContext().getApplicationContext())
              .startChromeBrowserProcesses(getContext())) {
          mNativeChromeBrowserProvider = nativeInit();
          return mNativeChromeBrowserProvider != 0;
      }
      return false;
  }

    /**
     * Call to get the intent to create a bookmark shortcut on homescreen.
     */
    public static Intent getShortcutToBookmark(String url, String title, Bitmap favicon, int rValue,
            int gValue, int bValue, Activity activity) {
        return BookmarkUtils.createAddToHomeIntent(activity, url, title, favicon, rValue, gValue,
                bValue);
    }

    private native int nativeInit();
    private native void nativeDestroy(int nativeChromeBrowserProvider);
    private native long nativeAddBookmark(
            int nativeChromeBrowserProvider,
            String url, String title, boolean isFolder, long parentId);
    private native void nativeRemoveBookmark(int nativeChromeBrowserProvider, long id);
    private native void nativeUpdateBookmark(
            int nativeChromeBrowserProvider, long id, String url, String title, long parentId);
    private static native long nativeCreateBookmarksFolderOnce(String title, long parentId);
    private static native BookmarkNode nativeGetMobileBookmarksFolder();
    private static native BookmarkNode nativeGetAllBookmarkFolders();
    private static native BookmarkNode nativeGetBookmarkNode(long id);
    private static native BookmarkNode nativeGetBookmarkNodeWithChildren(long id);
    private static native boolean nativeIsMobileBookmarksDescendant(long id);
    private static native void nativeEnsureBookmarksModelIsLoaded();

    private native long nativeAddBookmarkFromAPI(
            int nativeChromeBrowserProvider,
            String url, Long created, Boolean isBookmark,
            Long date, byte[] favicon, String title, Integer visits, long parentId);

    private native SQLiteCursor nativeQueryBookmarkFromAPI(int nativeChromeBrowserProvider,
            String[] projection, String selection,
            String[] selectionArgs, String sortOrder);

    private native int nativeUpdateBookmarkFromAPI(
            int nativeChromeBrowserProvider, String url, Long created, Boolean isBookmark,
            Long date, byte[] favicon, String title, Integer visits, long parent_id,
            String selection, String[] selectionArgs);

    private native int nativeRemoveBookmarkFromAPI(
            int nativeChromeBrowserProvider, String selection, String[] selectionArgs);

    private native int nativeRemoveHistoryFromAPI(
            int nativeChromeBrowserProvider, String selection, String[] selectionArgs);

    private native long nativeAddSearchTermFromAPI(int nativeChromeBrowserProvider,
            String term, Long date);

    private native SQLiteCursor nativeQuerySearchTermFromAPI(int nativeChromeBrowserProvider,
            String[] projection, String selection,
            String[] selectionArgs, String sortOrder);

    private native int nativeUpdateSearchTermFromAPI(int nativeChromeBrowserProvider,
            String search, Long date, String selection,
            String[] selectionArgs);

    private native int nativeRemoveSearchTermFromAPI(int nativeChromeBrowserProvider,
            String selection, String[] selectionArgs);

    private native byte[] nativeGetFaviconOrTouchIcon(int nativeChromeBrowserProvider, String url);

    private native byte[] nativeGetThumbnail(int nativeChromeBrowserProvider, String url);
}
