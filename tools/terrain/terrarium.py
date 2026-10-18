"""Terrarium RGB encoding: height = R*256 + G + B/256 - 32768 meters."""

from __future__ import annotations

import io
from functools import cache
from typing import TYPE_CHECKING

import numpy as np
from PIL import Image

from .heights import HEIGHT_STEP, SEA_LEVEL
from .tilemath import TILE_SIZE

if TYPE_CHECKING:
    from pathlib import Path

_OFFSET = 32768.0
_MAX_CODE = (1 << 24) - 1


def encode(heights: np.ndarray) -> np.ndarray:
    """Encode float heights (rows, cols) to terrarium RGB (rows, cols, 3) uint8."""
    # Rounding to the nearest 1/256 m keeps the decode error within half a quantum.
    codes = np.rint((heights.astype(np.float64) + _OFFSET) * 256.0)
    codes = np.clip(codes, 0, _MAX_CODE).astype(np.uint32)
    rgb = np.empty((*heights.shape, 3), dtype=np.uint8)
    rgb[..., 0] = codes >> 16
    rgb[..., 1] = (codes >> 8) & 0xFF
    rgb[..., 2] = codes & 0xFF
    return rgb


def decode(rgb: np.ndarray) -> np.ndarray:
    channels = rgb.astype(np.float64)
    heights = channels[..., 0] * 256.0 + channels[..., 1] + channels[..., 2] / 256.0 - _OFFSET
    return heights.astype(np.float32)


def quantize(heights: np.ndarray) -> np.ndarray:
    """Round to the nearest HEIGHT_STEP; a power of two, so it is exact in the encoding."""
    return (np.rint(heights.astype(np.float64) / HEIGHT_STEP) * HEIGHT_STEP).astype(np.float32)


def downsample(heights: np.ndarray) -> np.ndarray:
    """2x2 box average of heights; never average encoded RGB, the encoding is not linear."""
    rows, cols = heights.shape
    return (
        heights.reshape(rows // 2, 2, cols // 2, 2)
        .mean(axis=(1, 3), dtype=np.float64)
        .astype(np.float32)
    )


def _quantized_tile(heights: np.ndarray) -> np.ndarray:
    if heights.shape != (TILE_SIZE, TILE_SIZE):
        raise ValueError(f"Tile must be {TILE_SIZE}x{TILE_SIZE}, got {heights.shape}")
    return quantize(heights)


def _encode_png(quantized: np.ndarray) -> bytes:
    buffer = io.BytesIO()
    Image.fromarray(encode(quantized)).save(buffer, format="PNG", optimize=True)
    return buffer.getvalue()


def png_bytes(heights: np.ndarray) -> bytes:
    """Quantized terrarium PNG for one tile."""
    return _encode_png(_quantized_tile(heights))


@cache
def sea_png_bytes() -> bytes:
    return png_bytes(np.full((TILE_SIZE, TILE_SIZE), SEA_LEVEL, dtype=np.float32))


def tile_png_bytes(heights: np.ndarray) -> bytes:
    """Like png_bytes, but returns the shared sea tile when every height quantizes to 0."""
    quantized = _quantized_tile(heights)
    if not quantized.any():
        return sea_png_bytes()
    return _encode_png(quantized)


def read_heights(path: Path) -> np.ndarray:
    with Image.open(path) as image:
        return decode(np.asarray(image.convert("RGB")))


def heights_from_bytes(data: bytes) -> np.ndarray:
    with Image.open(io.BytesIO(data)) as image:
        return decode(np.asarray(image.convert("RGB")))
