import os
import sys
from PIL import Image, ImageDraw

MASTER_IMAGE_PATH = r"C:\Users\jithi\.gemini\antigravity-ide\brain\635b6776-491b-414a-92f8-f35cd9060a3a\media__1786008764023.png"
PROJECT_ROOT = r"C:\projects\qgroundcontrol"

def ensure_dir(d):
    os.makedirs(d, exist_ok=True)

def create_master_transparent_icon(input_path):
    print(f"Reading master icon from {input_path}...")
    img = Image.open(input_path).convert("RGBA")
    
    # Process outer black background to transparent alpha
    import numpy as np
    arr = np.array(img, dtype=np.float32)
    r, g, b, a = arr[:,:,0], arr[:,:,1], arr[:,:,2], arr[:,:,3]
    brightness = (r + g + b) / 3.0

    # Smooth anti-aliased threshold for outer background
    alpha = np.clip((brightness - 15.0) / 30.0, 0.0, 1.0) * 255.0
    arr[:,:,3] = alpha.astype(np.uint8)

    processed_img = Image.fromarray(arr.astype(np.uint8), mode="RGBA")
    bbox = processed_img.getbbox()
    cropped = processed_img.crop(bbox)

    w, h = cropped.size
    max_dim = max(w, h)
    
    target_size = 1024
    # Generous padding (~880px icon inside 1024px canvas) so logo is clearly visible at small sizes
    icon_target_dim = 880
    scale = icon_target_dim / float(max_dim)
    new_w = int(round(w * scale))
    new_h = int(round(h * scale))

    resized = cropped.resize((new_w, new_h), Image.Resampling.LANCZOS)

    master = Image.new("RGBA", (target_size, target_size), (0, 0, 0, 0))
    paste_x = (target_size - new_w) // 2
    paste_y = (target_size - new_h) // 2
    master.paste(resized, (paste_x, paste_y), resized)

    return master

def generate_all_icons():
    master = create_master_transparent_icon(MASTER_IMAGE_PATH)

    icons_dir = os.path.join(PROJECT_ROOT, "src", "Volador", "Assets", "Icons")
    deploy_win_dir = os.path.join(PROJECT_ROOT, "deploy", "windows")
    deploy_mac_dir = os.path.join(PROJECT_ROOT, "deploy", "macos")
    res_icons_dir = os.path.join(PROJECT_ROOT, "resources", "icons")
    android_dir = os.path.join(PROJECT_ROOT, "android")
    android_res_dir = os.path.join(android_dir, "res")

    for d in [icons_dir, deploy_win_dir, deploy_mac_dir, res_icons_dir, android_res_dir]:
        ensure_dir(d)

    # ---------------------------------------------------------
    # 1. WINDOWS & MULTI-RES ICO GENERATION
    # ---------------------------------------------------------
    ico_resolutions = [16, 24, 32, 48, 64, 128, 256]
    ico_frames = [master.resize((sz, sz), Image.Resampling.LANCZOS) for sz in ico_resolutions]

    # app.ico (multi-resolution)
    app_ico_path = os.path.join(icons_dir, "app.ico")
    ico_frames[0].save(app_ico_path, format="ICO", sizes=[(im.width, im.height) for im in ico_frames])
    print(f"Generated multi-res ICO: {app_ico_path}")

    # installer_icon.ico & desktop_shortcut.ico
    installer_ico_path = os.path.join(icons_dir, "installer_icon.ico")
    desktop_ico_path = os.path.join(icons_dir, "desktop_shortcut.ico")
    ico_frames[0].save(installer_ico_path, format="ICO", sizes=[(im.width, im.height) for im in ico_frames])
    ico_frames[0].save(desktop_ico_path, format="ICO", sizes=[(im.width, im.height) for im in ico_frames])

    # WindowsQGC.ico & qgroundcontrol.ico
    win_qgc_ico = os.path.join(deploy_win_dir, "WindowsQGC.ico")
    res_qgc_ico = os.path.join(res_icons_dir, "qgroundcontrol.ico")
    ico_frames[0].save(win_qgc_ico, format="ICO", sizes=[(im.width, im.height) for im in ico_frames])
    ico_frames[0].save(res_qgc_ico, format="ICO", sizes=[(im.width, im.height) for im in ico_frames])

    # ---------------------------------------------------------
    # 2. REQUIRED PNG SIZES (app_32.png, app_64.png, app_128.png, app_256.png, app_512.png, app_1024.png)
    # ---------------------------------------------------------
    png_sizes = [16, 24, 32, 48, 64, 128, 256, 512, 1024]
    for sz in png_sizes:
        img_sz = master.resize((sz, sz), Image.Resampling.LANCZOS)
        img_sz.save(os.path.join(icons_dir, f"app_{sz}.png"), "PNG")
        img_sz.save(os.path.join(icons_dir, f"app_{sz}x{sz}.png"), "PNG")
        print(f"Generated PNG: app_{sz}.png")

    # Special icon filenames
    master.resize((256, 256), Image.Resampling.LANCZOS).save(os.path.join(icons_dir, "app_icon.png"), "PNG")
    master.resize((256, 256), Image.Resampling.LANCZOS).save(os.path.join(icons_dir, "taskbar_icon.png"), "PNG")
    master.resize((64, 64), Image.Resampling.LANCZOS).save(os.path.join(icons_dir, "window_icon.png"), "PNG")
    master.resize((256, 256), Image.Resampling.LANCZOS).save(os.path.join(res_icons_dir, "qgroundcontrol.png"), "PNG")

    # ---------------------------------------------------------
    # 3. macOS ICON (app.icns & macx.icns)
    # ---------------------------------------------------------
    icns_path = os.path.join(icons_dir, "app.icns")
    macx_icns_path = os.path.join(deploy_mac_dir, "macx.icns")
    master.resize((512, 512), Image.Resampling.LANCZOS).save(icns_path, "PNG")
    master.resize((512, 512), Image.Resampling.LANCZOS).save(macx_icns_path, "PNG")
    print(f"Generated macOS icns: {icns_path}")

    # ---------------------------------------------------------
    # 4. LINUX ICONS (app.png, app_256.png, app_512.png)
    # ---------------------------------------------------------
    master.resize((512, 512), Image.Resampling.LANCZOS).save(os.path.join(icons_dir, "app.png"), "PNG")
    master.resize((256, 256), Image.Resampling.LANCZOS).save(os.path.join(icons_dir, "app_256.png"), "PNG")
    master.resize((512, 512), Image.Resampling.LANCZOS).save(os.path.join(icons_dir, "app_512.png"), "PNG")

    # ---------------------------------------------------------
    # 5. ANDROID ADAPTIVE LAUNCHER ICONS & NOTIFICATIONS
    # ---------------------------------------------------------
    android_densities = {
        "mdpi": (48, 24),
        "hdpi": (72, 36),
        "xhdpi": (96, 48),
        "xxhdpi": (144, 72),
        "xxxhdpi": (192, 96)
    }

    # Generate monochrome notification icon variant
    import numpy as np
    m_arr = np.array(master, dtype=np.uint8)
    mono_arr = m_arr.copy()
    mono_arr[:,:,0] = 255
    mono_arr[:,:,1] = 255
    mono_arr[:,:,2] = 255
    mono_img = Image.fromarray(mono_arr, mode="RGBA")

    for density, (launcher_sz, notify_sz) in android_densities.items():
        mipmap_dir = os.path.join(android_res_dir, f"mipmap-{density}")
        drawable_dir = os.path.join(android_res_dir, f"drawable-{density}")
        ensure_dir(mipmap_dir)
        ensure_dir(drawable_dir)

        # Launcher icons
        l_img = master.resize((launcher_sz, launcher_sz), Image.Resampling.LANCZOS)
        l_img.save(os.path.join(mipmap_dir, "ic_launcher.png"), "PNG")
        l_img.save(os.path.join(mipmap_dir, "ic_launcher_round.png"), "PNG")
        l_img.save(os.path.join(mipmap_dir, "ic_launcher_foreground.png"), "PNG")
        l_img.save(os.path.join(mipmap_dir, "icon.png"), "PNG")
        l_img.save(os.path.join(drawable_dir, "icon.png"), "PNG")

        # Notification icons
        n_img = mono_img.resize((notify_sz, notify_sz), Image.Resampling.LANCZOS)
        n_img.save(os.path.join(drawable_dir, "ic_stat_notify.png"), "PNG")
        n_img.save(os.path.join(drawable_dir, "notification.png"), "PNG")

    # Play Store icon (512x512)
    playstore_icon = os.path.join(android_dir, "playstore_icon_512.png")
    master.resize((512, 512), Image.Resampling.LANCZOS).save(playstore_icon, "PNG")
    master.resize((512, 512), Image.Resampling.LANCZOS).save(os.path.join(res_icons_dir, "android_512x512.png"), "PNG")
    print(f"Generated Android Play Store icon: {playstore_icon}")

    print("ALL PLATFORM APPLICATION ICONS GENERATED SUCCESSFULLY!")

if __name__ == "__main__":
    generate_all_icons()
