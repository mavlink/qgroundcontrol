import os
from PIL import Image, ImageOps

NEW_MASTER_IMAGE_PATH = r"C:\Users\jithi\.gemini\antigravity-ide\brain\ee123e22-cf1e-421b-ab87-816e0084e15b\media__1786003820810.png"
PROJECT_ROOT = r"C:\projects\qgroundcontrol"
LOGOS_DIR = os.path.join(PROJECT_ROOT, "src", "Volador", "Assets", "Logos")

def process_logos():
    print(f"Processing new master logo from {NEW_MASTER_IMAGE_PATH}...")
    master = Image.open(NEW_MASTER_IMAGE_PATH).convert("RGBA")
    
    os.makedirs(LOGOS_DIR, exist_ok=True)
    
    # 1. Full logo (wordmark + emblem) - trim content bounding box with padding
    bbox = master.getbbox()
    if bbox:
        # Pad bbox slightly
        pad = 20
        left = max(0, bbox[0] - pad)
        top = max(0, bbox[1] - pad)
        right = min(master.width, bbox[2] + pad)
        bottom = min(master.height, bbox[3] + pad)
        cropped_primary = master.crop((left, top, right, bottom))
    else:
        cropped_primary = master

    cropped_primary.save(os.path.join(LOGOS_DIR, "volador_primary.png"), "PNG")
    cropped_primary.save(os.path.join(LOGOS_DIR, "volador_dark.png"), "PNG")
    cropped_primary.save(os.path.join(LOGOS_DIR, "volador_light.png"), "PNG")
    print("Saved volador_primary.png, volador_dark.png, volador_light.png")

    # 2. Compact logo emblem (emblem on the right)
    # Right emblem content in 1024x1024 master is roughly (620, 400, 785, 535)
    emblem_box = (620, 400, 785, 535)
    cropped_emblem = master.crop(emblem_box)
    cropped_emblem.save(os.path.join(LOGOS_DIR, "volador_compact.png"), "PNG")
    cropped_emblem.save(os.path.join(LOGOS_DIR, "volador_emblem.png"), "PNG")
    print("Saved volador_compact.png, volador_emblem.png")

    # 3. Monochrome white variant (for dark UI accents)
    r, g, b, a = cropped_primary.split()
    mono = Image.merge("RGBA", (Image.new("L", cropped_primary.size, 255),
                               Image.new("L", cropped_primary.size, 255),
                               Image.new("L", cropped_primary.size, 255),
                               a))
    mono.save(os.path.join(LOGOS_DIR, "volador_monochrome.png"), "PNG")
    print("Saved volador_monochrome.png")

    print("Official in-application logo assets generated successfully!")

if __name__ == "__main__":
    process_logos()
