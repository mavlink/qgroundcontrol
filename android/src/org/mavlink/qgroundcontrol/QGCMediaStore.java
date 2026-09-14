package org.mavlink.qgroundcontrol;

import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.os.ParcelFileDescriptor;
import android.provider.MediaStore;

import androidx.annotation.RequiresApi;

import java.io.IOException;
import java.util.Locale;

/** Owns a pending media item and lends its descriptor to the native writer. */
public final class QGCMediaStore {
    private static final String TAG = QGCMediaStore.class.getSimpleName();
    private static final String VIDEO_DIRECTORY = Environment.DIRECTORY_MOVIES + "/QGroundControl/";
    private static final String IMAGE_DIRECTORY = Environment.DIRECTORY_PICTURES + "/QGroundControl/";

    private final ContentResolver m_resolver;
    private final Uri m_uri;
    private ParcelFileDescriptor m_descriptor;

    private QGCMediaStore(ContentResolver resolver, Uri uri, ParcelFileDescriptor descriptor) {
        m_resolver = resolver;
        m_uri = uri;
        m_descriptor = descriptor;
    }

    public static QGCMediaStore createVideo(Context context, String displayName) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q || context == null) {
            return null;
        }

        return create(context, displayName, videoMimeType(displayName), videoCollection(), VIDEO_DIRECTORY);
    }

    public static QGCMediaStore createImage(Context context, String displayName) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q || context == null) {
            return null;
        }

        final Uri collection = MediaStore.Images.Media.getContentUri(MediaStore.VOLUME_EXTERNAL_PRIMARY);
        return create(context, displayName, imageMimeType(displayName), collection, IMAGE_DIRECTORY);
    }

    private static QGCMediaStore create(Context context, String displayName, String mimeType,
                                       Uri collection, String directory) {
        if (mimeType == null || displayName.contains("/") || displayName.contains("\\")) {
            QGCLogger.w(TAG, "Invalid media name: " + displayName);
            return null;
        }

        final ContentResolver resolver = context.getContentResolver();
        Uri uri = null;
        try {
            final ContentValues values = new ContentValues();
            values.put(MediaStore.MediaColumns.DISPLAY_NAME, displayName);
            values.put(MediaStore.MediaColumns.MIME_TYPE, mimeType);
            values.put(MediaStore.MediaColumns.RELATIVE_PATH, directory);
            values.put(MediaStore.MediaColumns.IS_PENDING, 1);
            uri = resolver.insert(collection, values);
            if (uri == null) {
                throw new IOException("MediaStore did not create a media item");
            }
            final ParcelFileDescriptor descriptor = resolver.openFileDescriptor(uri, "rw");
            if (descriptor == null) {
                throw new IOException("MediaStore did not open the media item");
            }
            QGCLogger.i(TAG, "Writing directly to " + uri);
            return new QGCMediaStore(resolver, uri, descriptor);
        } catch (IOException | RuntimeException e) {
            QGCLogger.e(TAG, "Cannot create gallery media", e);
            if (uri != null) {
                deletePending(resolver, uri);
            }
            return null;
        }
    }

    public int fileDescriptor() {
        return m_descriptor != null ? m_descriptor.getFd() : -1;
    }

    /** Called only after the native writer has stopped using the borrowed descriptor. */
    public boolean finish(boolean finalized) {
        if (m_descriptor == null) {
            return false;
        }
        final long size;
        try (ParcelFileDescriptor descriptor = m_descriptor) {
            m_descriptor = null;
            size = descriptor.getStatSize();
        } catch (IOException | RuntimeException e) {
            QGCLogger.e(TAG, "Cannot close gallery media: " + m_uri, e);
            return false;
        }
        try {
            if (size == 0) {
                deletePending(m_resolver, m_uri);
                return false;
            }
            if (!finalized || size < 0) {
                // Do not destroy potentially recoverable data or expose an unfinished video.
                // Android's pending-item expiry still applies; crash recovery is separate.
                QGCLogger.w(TAG, "Media not finalized; leaving pending: " + m_uri);
                return false;
            }
            final ContentValues values = new ContentValues();
            values.put(MediaStore.MediaColumns.IS_PENDING, 0);
            if (m_resolver.update(m_uri, values, null, null) != 1) {
                QGCLogger.w(TAG, "MediaStore did not publish the media item: " + m_uri);
                return false;
            }
            QGCLogger.i(TAG, "Published gallery media: " + m_uri);
            return true;
        } catch (RuntimeException e) {
            QGCLogger.e(TAG, "Cannot publish gallery media: " + m_uri, e);
            return false;
        }
    }

    /** Discard a known failed write without deleting any published item. */
    public void discard() {
        final ParcelFileDescriptor descriptor = m_descriptor;
        m_descriptor = null;
        try {
            if (descriptor != null) {
                descriptor.close();
            }
        } catch (IOException | RuntimeException e) {
            QGCLogger.e(TAG, "Cannot close failed gallery media: " + m_uri, e);
        }
        deletePending(m_resolver, m_uri);
    }

    public static void cleanupOldVideos(Context context, long maxBytes) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q || context == null || maxBytes <= 0) {
            return;
        }

        final ContentResolver resolver = context.getContentResolver();
        final Uri collection = videoCollection();
        final String[] projection = {MediaStore.Video.Media._ID, MediaStore.Video.Media.SIZE};
        // Never delete another app's media or a recording still being finalized.
        final String selection = MediaStore.Video.Media.OWNER_PACKAGE_NAME + " = ? AND "
                + MediaStore.Video.Media.RELATIVE_PATH + " = ? AND "
                + MediaStore.Video.Media.IS_PENDING + " = 0";
        final String[] selectionArgs = {context.getPackageName(), VIDEO_DIRECTORY};
        final String sort = MediaStore.Video.Media.DATE_ADDED + " DESC, " + MediaStore.Video.Media._ID + " DESC";
        try (Cursor cursor = resolver.query(collection, projection, selection, selectionArgs, sort)) {
            if (cursor == null) {
                return;
            }
            long total = 0;
            while (cursor.moveToNext()) {
                total += cursor.getLong(1);
            }
            while (total >= maxBytes && cursor.moveToPrevious()) {
                final Uri uri = ContentUris.withAppendedId(collection, cursor.getLong(0));
                if (resolver.delete(uri, selection, selectionArgs) == 1) {
                    total -= cursor.getLong(1);
                    QGCLogger.i(TAG, "Storage limit removed old recording: " + uri);
                }
            }
        } catch (RuntimeException e) {
            QGCLogger.e(TAG, "Cannot enforce gallery recording storage limit", e);
        }
    }

    static String videoMimeType(String displayName) {
        if (displayName == null) {
            return null;
        }
        final String name = displayName.toLowerCase(Locale.ROOT);
        if (name.endsWith(".mp4")) {
            return "video/mp4";
        }
        if (name.endsWith(".mov")) {
            return "video/quicktime";
        }
        if (name.endsWith(".mkv")) {
            return "video/x-matroska";
        }
        return null;
    }

    static String imageMimeType(String displayName) {
        if (displayName == null) {
            return null;
        }
        final String name = displayName.toLowerCase(Locale.ROOT);
        return name.endsWith(".jpg") || name.endsWith(".jpeg") ? "image/jpeg" : null;
    }

    @RequiresApi(Build.VERSION_CODES.Q)
    private static Uri videoCollection() {
        return MediaStore.Video.Media.getContentUri(MediaStore.VOLUME_EXTERNAL_PRIMARY);
    }

    private static void deletePending(ContentResolver resolver, Uri uri) {
        try {
            resolver.delete(uri, MediaStore.MediaColumns.IS_PENDING + " = 1", null);
        } catch (RuntimeException e) {
            QGCLogger.e(TAG, "Cannot remove pending media: " + uri, e);
        }
    }
}
