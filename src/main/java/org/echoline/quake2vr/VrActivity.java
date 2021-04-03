package org.echoline.quake2vr;

import android.Manifest;
import android.annotation.SuppressLint;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.net.Uri;
import android.os.Build.VERSION;
import android.os.Build.VERSION_CODES;
import android.os.Bundle;
import android.os.Environment;
import android.provider.Settings;
import android.util.Log;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.PopupMenu;
import android.widget.RelativeLayout;
import android.widget.Toast;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;

import org.libsdl.app.SDLActivity;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;

public class VrActivity extends SDLActivity implements PopupMenu.OnMenuItemClickListener {
  private static final String TAG = VrActivity.class.getSimpleName();

  // Permission request codes
  private static final int PERMISSIONS_REQUEST_CODE = 2;

  // Opaque native pointer to the native CardboardApp instance.
  // This object is owned by the VrActivity instance and passed to the native methods.
  public static long nativeApp;

  String []getPaths() {
    File []in = getExternalFilesDirs(null);
    String []out = new String[in.length];
    for (int i = 0; i < in.length; i++)
      out[i] = in[i].getPath();
    return out;
  }

  @SuppressLint("ClickableViewAccessibility")
  @Override
  public void onCreate(Bundle savedInstance) {
    super.onCreate(savedInstance);

    String fpath = getPaths()[0];
    Log.d("org.echoline.quake2vr", fpath);

    AssetManager a = getAssets();
    expandAssets(getAssets(), "");

    nativeApp = nativeOnCreate(fpath);

    setContentView(R.layout.activity_vr);
    ((FrameLayout)findViewById(R.id.surface_view)).addView(mSurface, new RelativeLayout.LayoutParams(MATCH_PARENT, MATCH_PARENT));

    // TODO(b/139010241): Avoid that action and status bar are displayed when pressing settings
    // button.
    setImmersiveSticky();
    View decorView = getWindow().getDecorView();
    decorView.setOnSystemUiVisibilityChangeListener(
        (visibility) -> {
          if ((visibility & View.SYSTEM_UI_FLAG_FULLSCREEN) == 0) {
            setImmersiveSticky();
          }
        });

    // Forces screen to max brightness.
    WindowManager.LayoutParams layout = getWindow().getAttributes();
    layout.screenBrightness = 1.f;
    getWindow().setAttributes(layout);

    // Prevents screen from dimming/locking.
    getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
  }

  private void expandAssets(AssetManager a, String p) {
    try {
      for (String s: a.list(p)) {
        String path = (p == ""? s: p + "/" + s);
        Log.d("org.echoline.quake2vr", path);
        File f = new File(getPaths()[0] + "/" + path);
        if (!f.exists()) {
          InputStream is = null;
          try {
            is = a.open(path);
          } catch (Exception e) {
            f.mkdirs();
          } finally {
            if (!f.isDirectory()) {
              FileOutputStream os = new FileOutputStream(f);
              byte[] buffer = new byte[is.available()];
              while (is.read(buffer) > 0)
                os.write(buffer);
              os.close();
              is.close();
            }
          }
        }
        expandAssets(a, path);
      }
    } catch (Exception e) {
      e.printStackTrace();
    }
  }

  @Override
  public void onPause() {
    super.onPause();
    nativeOnPause(nativeApp);
  }

  @Override
  public void onResume() {
    super.onResume();

    // On Android P and below, checks for activity to READ_EXTERNAL_STORAGE. When it is not granted,
    // the application will request them. For Android Q and above, READ_EXTERNAL_STORAGE is optional
    // and scoped storage will be used instead. If it is provided (but not checked) and there are
    // device parameters saved in external storage those will be migrated to scoped storage.
    if (VERSION.SDK_INT < VERSION_CODES.Q && !isReadExternalStorageEnabled()) {
      requestPermissions();
      return;
    }

    nativeOnResume(nativeApp);
  }

  @Override
  public void onDestroy() {
    super.onDestroy();
    nativeOnDestroy(nativeApp);
    nativeApp = 0;
  }

  @Override
  public void onWindowFocusChanged(boolean hasFocus) {
    super.onWindowFocusChanged(hasFocus);
    if (hasFocus) {
      setImmersiveSticky();
    }
  }

  /** Callback for when close button is pressed. */
  public void closeSample(View view) {
    Log.d(TAG, "Leaving VR sample");
    finish();
  }

  /** Callback for when settings_menu button is pressed. */
  public void showSettings(View view) {
    PopupMenu popup = new PopupMenu(this, view);
    MenuInflater inflater = popup.getMenuInflater();
    inflater.inflate(R.menu.settings_menu, popup.getMenu());
    popup.setOnMenuItemClickListener(this);
    popup.show();
  }

  public void runMain() {
    nativeRunMain(nativeApp);
  }

  @Override
  public boolean onMenuItemClick(MenuItem item) {
    if (item.getItemId() == R.id.switch_viewer) {
      nativeSwitchViewer(nativeApp);
      return true;
    }
    return false;
  }

  /**
   * Checks for READ_EXTERNAL_STORAGE permission.
   *
   * @return whether the READ_EXTERNAL_STORAGE is already granted.
   */
  private boolean isReadExternalStorageEnabled() {
    return ActivityCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE)
        == PackageManager.PERMISSION_GRANTED;
  }

  /** Handles the requests for activity permission to READ_EXTERNAL_STORAGE. */
  private void requestPermissions() {
    final String[] permissions = new String[] {Manifest.permission.READ_EXTERNAL_STORAGE};
    ActivityCompat.requestPermissions(this, permissions, PERMISSIONS_REQUEST_CODE);
  }

  /**
   * Callback for the result from requesting permissions.
   *
   * <p>When READ_EXTERNAL_STORAGE permission is not granted, the settings view will be launched
   * with a toast explaining why it is required.
   */
  @Override
  public void onRequestPermissionsResult(
      int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
    super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    if (!isReadExternalStorageEnabled()) {
      Toast.makeText(this, R.string.read_storage_permission, Toast.LENGTH_LONG).show();
      if (!ActivityCompat.shouldShowRequestPermissionRationale(
          this, Manifest.permission.READ_EXTERNAL_STORAGE)) {
        // Permission denied with checking "Do not ask again". Note that in Android R "Do not ask
        // again" is not available anymore.
        launchPermissionsSettings();
      }
      finish();
    }
  }

  private void launchPermissionsSettings() {
    Intent intent = new Intent();
    intent.setAction(Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
    intent.setData(Uri.fromParts("package", getPackageName(), null));
    startActivity(intent);
  }

  private void setImmersiveSticky() {
    getWindow()
        .getDecorView()
        .setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
  }

  private native long nativeOnCreate(String path);

  private native void nativeOnDestroy(long nativeApp);

  private native void nativeOnPause(long nativeApp);

  private native void nativeOnResume(long nativeApp);

  public native void nativeSetScreenParams(long nativeApp, int width, int height);

  private native void nativeRunMain(long nativeApp);

  private native void nativeSwitchViewer(long nativeApp);
}
