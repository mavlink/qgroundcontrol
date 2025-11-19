#!/usr/bin/env python3
import sys, os, re, io, base64, shutil, subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent

def _ensure_pillow():
    try:
        from PIL import Image  # type: ignore
        return Image
    except Exception:
        try:
            subprocess.check_call([sys.executable, "-m", "pip", "install", "--user", "pillow"])  # noqa: E501
            from PIL import Image  # type: ignore
            return Image
        except Exception as e:
            print("ERROR: Pillow not available and installation failed:", e)
            sys.exit(1)

Image = _ensure_pillow()

src_png = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "ig-gcs-fly.png"
if not src_png.is_absolute():
    src_png = (ROOT / src_png).resolve()
if not src_png.exists():
    print(f"ERROR: Source PNG not found: {src_png}")
    sys.exit(1)

img = Image.open(src_png).convert("RGBA")

def save_png_size(im, size, out_path: Path):
    out_path.parent.mkdir(parents=True, exist_ok=True)
    im.resize((size, size), Image.LANCZOS).save(out_path, format="PNG")

def make_ico(im, out_path: Path, sizes=(16,24,32,48,64,128,256)):
    out_path.parent.mkdir(parents=True, exist_ok=True)
    im.save(out_path, format="ICO", sizes=[(s, s) for s in sizes])

def try_make_icns(im, out_path: Path):
    out_path.parent.mkdir(parents=True, exist_ok=True)
    ok = False
    try:
        # pillow-icns adds ICNS save handler
        im.save(out_path, format="ICNS")
        ok = True
    except Exception:
        pass
    if ok:
        return True
    try:
        import icnsutil  # type: ignore
    except Exception:
        try:
            subprocess.check_call([sys.executable, "-m", "pip", "install", "--user", "icnsutil"])  # noqa: E501
            import icnsutil  # type: ignore
        except Exception:
            print("WARNING: Could not create .icns (icnsutil not available). Skipping.")
            return False
    from icnsutil import IcnsFile  # type: ignore
    tmp = out_path.parent / "_tmp_iconset"
    if tmp.exists():
        shutil.rmtree(tmp)
    tmp.mkdir(parents=True, exist_ok=True)
    sizes = [16, 32, 64, 128, 256, 512]
    for s in sizes:
        save_png_size(im, s, tmp / f"icon_{s}x{s}.png")
        save_png_size(im, s * 2, tmp / f"icon_{s}x{s}@2x.png")
    icns = IcnsFile()
    for p in sorted(tmp.glob("*.png")):
        icns.add_image(p.read_bytes())
    with open(out_path, "wb") as f:
        icns.write(f)
    shutil.rmtree(tmp)
    return True

def png_to_svg_embed(im, out_path: Path):
    out_path.parent.mkdir(parents=True, exist_ok=True)
    buf = io.BytesIO()
    im.save(buf, format="PNG")
    b64 = base64.b64encode(buf.getvalue()).decode("ascii")
    w, h = im.size
    svg = (
        f"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        f"<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"{w}\" height=\"{h}\" viewBox=\"0 0 {w} {h}\">\n"
        f"  <image width=\"{w}\" height=\"{h}\" href=\"data:image/png;base64,{b64}\" xlink:href=\"data:image/png;base64,{b64}\"/>\n"
        f"</svg>\n"
    )
    out_path.write_text(svg, encoding="utf-8")

# In-app logos (SVG wrappers)
for name in [
    "QGCLogoFull.svg",
    "QGCLogoWhite.svg",
    "QGCLogoBlack.svg",
    "QGCLogoArrow.svg",
]:
    png_to_svg_embed(img, ROOT / "resources" / name)

# Splash screen PNG (square based on source)
max_side = max(img.size)
save_png_size(img, max_side, ROOT / "resources" / "SplashScreen.png")

# Linux runtime icon via qrc
make_ico(img, ROOT / "resources" / "icons" / "qgroundcontrol.ico")

# Windows EXE icon
make_ico(img, ROOT / "deploy" / "windows" / "WindowsQGC.ico")

# macOS bundle icon
try_make_icns(img, ROOT / "deploy" / "macos" / "qgroundcontrol.icns")

# Linux packaging icons
png_to_svg_embed(img, ROOT / "deploy" / "linux" / "QGroundControl.svg")
save_png_size(img, 256, ROOT / "deploy" / "linux" / "QGroundControl_256.png")

# iOS AppIcon set: overwrite existing sizes found
ios_dir = ROOT / "deploy" / "ios" / "Images.xcassets" / "AppIcon.appiconset"
if ios_dir.exists():
    pat = re.compile(r"AppIcon(\d+)x(\d+)(?:@(\d+)x)?(?:~ipad)?\.png$", re.IGNORECASE)
    for p in ios_dir.glob("AppIcon*.png"):
        m = pat.search(p.name)
        if not m:
            continue
        w = int(m.group(1))
        h = int(m.group(2))
        mul = int(m.group(3) or 1)
        if w != h:
            continue
        size = w * mul
        save_png_size(img, size, p)

# Cleanup source PNG and self-delete
try:
    if src_png.exists():
        src_png.unlink()
except Exception:
    pass

me = Path(__file__).resolve()
if os.name == "nt":
    bat = me.parent / "_self_delete.bat"
    bat.write_text(
        f"""@echo off
ping 127.0.0.1 -n 2 > nul
 del "{me.as_posix()}"
 del "%~f0"
""",
        encoding="utf-8",
    )
    subprocess.Popen(["cmd", "/c", str(bat)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
else:
    try:
        me.unlink()
    except Exception:
        pass

print("Icon replacement complete.")
