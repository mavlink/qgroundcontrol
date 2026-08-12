import os
import sys
from PIL import Image, ImageOps, ImageEnhance

MASTER_IMAGE_PATH = r"C:\Users\jithi\.gemini\antigravity-ide\brain\ee123e22-cf1e-421b-ab87-816e0084e15b\media__1786002817036.png"
PROJECT_ROOT = r"C:\projects\qgroundcontrol"

def ensure_dir(d):
    os.makedirs(d, exist_ok=True)

def generate_branding():
    print(f"Loading master logo from {MASTER_IMAGE_PATH}...")
    master = Image.open(MASTER_IMAGE_PATH).convert("RGBA")
    
    # Target directories
    icons_dir = os.path.join(PROJECT_ROOT, "src", "Volador", "Assets", "Icons")
    logos_dir = os.path.join(PROJECT_ROOT, "src", "Volador", "Assets", "Logos")
    android_res_dir = os.path.join(PROJECT_ROOT, "android", "res")
    deploy_win_dir = os.path.join(PROJECT_ROOT, "deploy", "windows")
    res_icons_dir = os.path.join(PROJECT_ROOT, "resources", "icons")

    for d in [icons_dir, logos_dir, deploy_win_dir, res_icons_dir]:
        ensure_dir(d)

    # 1. Generate PNG icon sizes
    sizes = [16, 24, 32, 48, 64, 128, 256, 512, 1024]
    ico_images = []

    for size in sizes:
        resized = master.resize((size, size), Image.Resampling.LANCZOS)
        out_path = os.path.join(icons_dir, f"app_{size}x{size}.png")
        resized.save(out_path, "PNG")
        print(f"Saved {out_path}")
        if size <= 256:
            ico_images.append(resized)

    # Special icon filenames
    master.resize((256, 256), Image.Resampling.LANCZOS).save(os.path.join(icons_dir, "app_icon.png"), "PNG")
    master.resize((256, 256), Image.Resampling.LANCZOS).save(os.path.join(icons_dir, "taskbar_icon.png"), "PNG")
    master.resize((64, 64), Image.Resampling.LANCZOS).save(os.path.join(icons_dir, "window_icon.png"), "PNG")

    # Generate app.ico (multi-resolution ICO)
    app_ico_path = os.path.join(icons_dir, "app.ico")
    ico_images[0].save(app_ico_path, format="ICO", sizes=[(im.width, im.height) for im in ico_images])
    print(f"Saved multi-res ICO: {app_ico_path}")

    # Generate installer & desktop shortcut ICOs
    master.resize((256, 256), Image.Resampling.LANCZOS).save(os.path.join(icons_dir, "installer_icon.ico"), format="ICO")
    master.resize((256, 256), Image.Resampling.LANCZOS).save(os.path.join(icons_dir, "desktop_shortcut.ico"), format="ICO")

    # Save dummy app.icns (as PNG format for fallback compatibility or ICNS container)
    master.resize((512, 512), Image.Resampling.LANCZOS).save(os.path.join(icons_dir, "app.icns"), "PNG")

    # Copy / update Windows & QGC fallback icons
    master.resize((256, 256), Image.Resampling.LANCZOS).save(os.path.join(deploy_win_dir, "WindowsQGC.ico"), format="ICO")
    master.resize((256, 256), Image.Resampling.LANCZOS).save(os.path.join(res_icons_dir, "qgroundcontrol.ico"), format="ICO")
    master.resize((256, 256), Image.Resampling.LANCZOS).save(os.path.join(res_icons_dir, "qgroundcontrol.png"), "PNG")

    # 2. Generate Logos
    master.save(os.path.join(logos_dir, "volador_primary.png"), "PNG")
    master.resize((256, 256), Image.Resampling.LANCZOS).save(os.path.join(logos_dir, "volador_compact.png"), "PNG")
    master.save(os.path.join(logos_dir, "volador_dark.png"), "PNG")
    master.save(os.path.join(logos_dir, "volador_light.png"), "PNG")

    # Create monochrome variant (converting RGB channels to white with alpha mask)
    r, g, b, a = master.split()
    mono = Image.merge("RGBA", (Image.new("L", master.size, 255), Image.new("L", master.size, 255), Image.new("L", master.size, 255), a))
    mono.save(os.path.join(logos_dir, "volador_monochrome.png"), "PNG")

    # 3. Generate Android Density Icons
    android_densities = {
        "mdpi": (48, 24),
        "hdpi": (72, 36),
        "xhdpi": (96, 48),
        "xxhdpi": (144, 72),
        "xxxhdpi": (192, 96)
    }

    for density, (launcher_sz, notify_sz) in android_densities.items():
        mipmap_dir = os.path.join(android_res_dir, f"mipmap-{density}")
        drawable_dir = os.path.join(android_res_dir, f"drawable-{density}")
        ensure_dir(mipmap_dir)
        ensure_dir(drawable_dir)

        # Launcher icon
        launcher_img = master.resize((launcher_sz, launcher_sz), Image.Resampling.LANCZOS)
        launcher_img.save(os.path.join(mipmap_dir, "ic_launcher.png"), "PNG")
        launcher_img.save(os.path.join(mipmap_dir, "ic_launcher_round.png"), "PNG")
        launcher_img.save(os.path.join(mipmap_dir, "ic_launcher_foreground.png"), "PNG")
        launcher_img.save(os.path.join(mipmap_dir, "icon.png"), "PNG")
        launcher_img.save(os.path.join(drawable_dir, "icon.png"), "PNG")

        # Notification icon (monochrome alpha)
        notify_img = mono.resize((notify_sz, notify_sz), Image.Resampling.LANCZOS)
        notify_img.save(os.path.join(drawable_dir, "ic_stat_notify.png"), "PNG")

        # Splash icon
        splash_img = master.resize((launcher_sz * 2, launcher_sz * 2), Image.Resampling.LANCZOS)
        splash_img.save(os.path.join(drawable_dir, "splash.png"), "PNG")

    # Play store 512x512 icon
    master.resize((512, 512), Image.Resampling.LANCZOS).save(os.path.join(PROJECT_ROOT, "android", "playstore_icon_512.png"), "PNG")
    print("All Volador branding assets generated successfully!")

if __name__ == "__main__":
    generate_branding()
