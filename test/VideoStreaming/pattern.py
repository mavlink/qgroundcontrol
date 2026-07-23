from __future__ import annotations

import io
import math
import time

from PIL import Image, ImageDraw, ImageFont


def jpeg_frame(index: int, width: int = 640, height: int = 360, quality: int = 80) -> bytes:
    image = Image.new("RGB", (width, height), (18, 24, 34))
    draw = ImageDraw.Draw(image)

    draw.rectangle((0, 0, width, 52), fill=(32, 46, 67))
    draw.text(
        (18, 16), "QGC Network Video Test", fill=(240, 244, 248), font=ImageFont.load_default()
    )

    grid_color = (56, 68, 82)
    for x in range(0, width, 40):
        draw.line((x, 52, x, height), fill=grid_color)
    for y in range(80, height, 40):
        draw.line((0, y, width, y), fill=grid_color)

    phase = index / 18.0
    cx = int((width - 120) * (0.5 + 0.45 * math.sin(phase)))
    cy = int(120 + (height - 180) * (0.5 + 0.45 * math.cos(phase * 0.7)))
    draw.ellipse((cx, cy, cx + 80, cy + 80), fill=(29, 185, 84), outline=(255, 255, 255), width=3)
    draw.rectangle(
        (width - 170, height - 58, width - 18, height - 18), outline=(255, 184, 77), width=2
    )
    draw.text(
        (width - 156, height - 46),
        f"frame {index:06d}",
        fill=(255, 224, 160),
        font=ImageFont.load_default(),
    )
    draw.text(
        (18, height - 42),
        time.strftime("%Y-%m-%d %H:%M:%S"),
        fill=(180, 194, 210),
        font=ImageFont.load_default(),
    )

    buffer = io.BytesIO()
    image.save(buffer, format="JPEG", quality=quality, optimize=True)
    return buffer.getvalue()
