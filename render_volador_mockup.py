"""
================================================================================
Volador Ground Control - Professional Interface Mockup Generator
================================================================================
Company: Volador Aerospace
Platform: Volador Ground Control v5.0
Output Resolution: 1920 x 1080 PNG
Theme: Dark Aerospace (Matte Black #101214, Dark Graphite #181B20 / #1F232B, Neon Orange #FF6A00)
================================================================================
"""

import os
import math
from PIL import Image, ImageDraw, ImageFont

# Color Palette Definitions
COLOR_BG = (16, 18, 20, 255)            # Matte Black Canvas
COLOR_PANEL = (24, 27, 32, 255)         # Dark Graphite Panel
COLOR_CARD = (31, 35, 43, 255)          # Graphite Card Surface
COLOR_BORDER = (45, 52, 64, 255)        # Border Outline
COLOR_ORANGE = (255, 106, 0, 255)       # Primary Accent Neon Orange
COLOR_ORANGE_SOFT = (255, 133, 51, 255)  # Hover / Secondary Accent
COLOR_WHITE = (255, 255, 255, 255)      # Primary Text
COLOR_MUTED = (140, 148, 160, 255)      # Subtitle / Muted Text
COLOR_GREEN = (0, 224, 75, 255)         # Success Status
COLOR_GRID = (25, 30, 37, 255)          # Map Grid Lines

def draw_rounded_rect(draw, box, radius=6, fill=None, outline=None, width=1):
    """Utility to draw crisp rounded rectangles."""
    draw.rounded_rectangle(box, radius=radius, fill=fill, outline=outline, width=width)

def render_header(draw, x, y, width, title):
    """Render top header bar for each panel view."""
    draw_rounded_rect(draw, [x, y, x + width, y + 40], radius=6, fill=COLOR_PANEL, outline=COLOR_BORDER)
    
    # Volador Logo Mark (V-Wing)
    cx, cy = x + 20, y + 20
    draw.ellipse([cx-12, cy-12, cx+12, cy+12], fill=COLOR_BG, outline=COLOR_ORANGE, width=2)
    draw.polygon([(cx-6, cy-3), (cx-1, cy+5), (cx-1, cy+2), (cx-5, cy-2)], fill=COLOR_ORANGE)
    draw.polygon([(cx+6, cy-3), (cx+1, cy+5), (cx+1, cy+2), (cx+5, cy-2)], fill=COLOR_ORANGE)
    draw.polygon([(cx, cy-3), (cx+2, cy+1), (cx, cy+5), (cx-2, cy+1)], fill=COLOR_WHITE)

    # Title Text
    draw.text((x + 40, y + 12), f"VOLADOR GROUND CONTROL", fill=COLOR_WHITE)
    draw.text((x + 230, y + 12), f"|  {title}", fill=COLOR_ORANGE)
    draw.text((x + width - 150, y + 12), "VOLADOR AEROSPACE", fill=COLOR_MUTED)

def render_flight_view(draw, x, y, w, h):
    """1. Flight View Panel (Satellite Map, Trajectory, HUD, Telemetry)."""
    # Background Map Surface
    draw_rounded_rect(draw, [x, y, x + w, y + h], radius=8, fill=(12, 14, 16, 255), outline=COLOR_BORDER)
    render_header(draw, x, y, w, "FLIGHT VIEW")
    
    map_y = y + 45
    map_h = h - 45

    # Simulated Map Grid & Terrain Topo
    for gx in range(x + 20, x + w, 40):
        draw.line([(gx, map_y), (gx, y + h)], fill=COLOR_GRID, width=1)
    for gy in range(map_y + 20, y + h, 40):
        draw.line([(x, gy), (x + w, gy)], fill=COLOR_GRID, width=1)

    # Flight Trajectory Path
    points = [(x + 80, map_y + 300), (x + 220, map_y + 180), (x + 400, map_y + 220), (x + 600, map_y + 120), (x + 780, map_y + 250)]
    draw.line(points, fill=COLOR_ORANGE, width=3)

    for pt in points:
        draw.ellipse([pt[0]-5, pt[1]-5, pt[0]+5, pt[1]+5], fill=COLOR_WHITE, outline=COLOR_ORANGE, width=2)

    # Active Drone Position Marker
    vx, vy = x + 600, map_y + 120
    draw.ellipse([vx-16, vy-16, vx+16, vy+16], fill=COLOR_PANEL, outline=COLOR_ORANGE, width=3)
    draw.polygon([(vx, vy-10), (vx+8, vy+8), (vx-8, vy+8)], fill=COLOR_WHITE)

    # Top Left Overlay Widget (Compass & Flight Mode)
    draw_rounded_rect(draw, [x + 15, map_y + 15, x + 240, map_y + 90], radius=6, fill=COLOR_PANEL, outline=COLOR_BORDER)
    draw.text((x + 25, map_y + 25), "FLIGHT MODE", fill=COLOR_MUTED)
    draw.text((x + 25, map_y + 45), "HOLD / STABILIZED", fill=COLOR_ORANGE)
    draw.text((x + 150, map_y + 25), "HEADING", fill=COLOR_MUTED)
    draw.text((x + 150, map_y + 45), "042° NE", fill=COLOR_WHITE)

    # Bottom Telemetry HUD Overlay
    hud_w = w - 30
    hud_h = 75
    hud_x = x + 15
    hud_y = y + h - hud_h - 15
    draw_rounded_rect(draw, [hud_x, hud_y, hud_x + hud_w, hud_y + hud_h], radius=6, fill=COLOR_PANEL, outline=COLOR_ORANGE)

    metrics = [
        ("ALTITUDE", "142.8 m"),
        ("GROUND SPEED", "16.4 m/s"),
        ("AIRSPEED", "17.1 m/s"),
        ("DISTANCE", "620 m"),
        ("BATTERY", "96% (24.8V)"),
        ("GPS LOCK", "3D LOCK (22)"),
    ]
    mx = hud_x + 20
    for label, val in metrics:
        draw.text((mx, hud_y + 15), label, fill=COLOR_MUTED)
        color = COLOR_GREEN if "96%" in val or "3D" in val else COLOR_WHITE
        draw.text((mx, hud_y + 40), val, fill=color)
        mx += 140

def render_plan_view(draw, x, y, w, h):
    """2. Plan View Panel (Survey Polygon, Transect Lines, Mission Stats)."""
    draw_rounded_rect(draw, [x, y, x + w, y + h], radius=8, fill=(12, 14, 16, 255), outline=COLOR_BORDER)
    render_header(draw, x, y, w, "PLAN VIEW")

    map_y = y + 45

    # Grid background
    for gx in range(x + 20, x + w, 40): draw.line([(gx, map_y), (gx, y + h)], fill=COLOR_GRID, width=1)
    for gy in range(map_y + 20, y + h, 40): draw.line([(x, gy), (x + w, gy)], fill=COLOR_GRID, width=1)

    # Survey Polygon Boundary
    poly = [
        (x + 180, map_y + 80),
        (x + 650, map_y + 60),
        (x + 720, map_y + 320),
        (x + 220, map_y + 340),
        (x + 120, map_y + 220)
    ]
    draw.polygon(poly, fill=(31, 35, 43, 160), outline=COLOR_ORANGE, width=2)

    # Transect Survey Lines
    for sy in range(map_y + 95, map_y + 310, 30):
        draw.line([(x + 180, sy), (x + 680, sy)], fill=COLOR_WHITE, width=1)
        draw.ellipse([x + 177, sy-3, x + 183, sy+3], fill=COLOR_ORANGE)
        draw.ellipse([x + 677, sy-3, x + 683, sy+3], fill=COLOR_ORANGE)

    # Plan Side Menu Overlay
    draw_rounded_rect(draw, [x + 15, map_y + 15, x + 260, map_y + 240], radius=6, fill=COLOR_PANEL, outline=COLOR_BORDER)
    draw.text((x + 30, map_y + 30), "SURVEY MISSION PATTERN", fill=COLOR_ORANGE)
    draw.line([(x + 30, map_y + 52), (x + 245, map_y + 52)], fill=COLOR_BORDER)

    stats = [
        ("Area Covered:", "18.4 Ha"),
        ("Flight Time:", "14 min 30 sec"),
        ("Grid Distance:", "25.0 m"),
        ("GSD Resolution:", "2.1 cm/px"),
        ("Photo Count:", "312 Images"),
    ]
    sy_pos = map_y + 65
    for lbl, val in stats:
        draw.text((x + 30, sy_pos), lbl, fill=COLOR_MUTED)
        draw.text((x + 160, sy_pos), val, fill=COLOR_WHITE)
        sy_pos += 30

def render_platform_dashboard(draw, x, y, w, h):
    """3. Volador Platform Dashboard (Fleet Status, Active Missions, Cloud Sync)."""
    draw_rounded_rect(draw, [x, y, x + w, y + h], radius=8, fill=COLOR_BG, outline=COLOR_BORDER)
    render_header(draw, x, y, w, "PLATFORM DASHBOARD")

    dy = y + 55

    # Top KPI Cards Row
    kpi_cards = [
        ("TOTAL FLEET DRONES", "24 Active Drones", COLOR_WHITE),
        ("FLEET HEALTH", "100% Operational", COLOR_GREEN),
        ("LIVE MISSIONS", "6 Missions Active", COLOR_ORANGE),
        ("CLOUD SYNCHRONIZATION", "Connected (volador.in)", COLOR_GREEN)
    ]
    card_w = (w - 60) // 4
    cx = x + 15
    for title, val, col in kpi_cards:
        draw_rounded_rect(draw, [cx, dy, cx + card_w, dy + 85], radius=6, fill=COLOR_CARD, outline=COLOR_BORDER)
        draw.text((cx + 15, dy + 20), title, fill=COLOR_MUTED)
        draw.text((cx + 15, dy + 48), val, fill=col)
        cx += card_w + 10

    # Middle Section: Active Drone Fleet List & Analytics Cards
    mid_y = dy + 105
    col1_w = (w - 45) * 0.6
    col2_w = (w - 45) * 0.4

    # Fleet List Panel
    draw_rounded_rect(draw, [x + 15, mid_y, x + 15 + col1_w, y + h - 15], radius=6, fill=COLOR_PANEL, outline=COLOR_BORDER)
    draw.text((x + 30, mid_y + 20), "REGISTERED VOLADOR FLEET DRONES", fill=COLOR_ORANGE)
    draw.line([(x + 30, mid_y + 45), (x + 15 + col1_w - 15, mid_y + 45)], fill=COLOR_BORDER)

    drones = [
        ("VOL-HEX-001", "Volador Survey Hexa", "Flight Ready", "98% Battery", "124.5 hrs"),
        ("VOL-VTL-004", "Volador VTOL Recon", "In Mission", "84% Battery", "310.2 hrs"),
        ("VOL-AGR-009", "Volador Crop Sprayer", "Charging", "45% Battery", "88.0 hrs"),
        ("VOL-INS-012", "Volador Solar Inspector", "Standby", "100% Battery", "204.1 hrs"),
    ]
    ry = mid_y + 60
    for sid, name, status, bat, hrs in drones:
        draw_rounded_rect(draw, [x + 30, ry, x + col1_w - 5, ry + 48], radius=4, fill=COLOR_CARD, outline=COLOR_BORDER)
        draw.text((x + 45, ry + 16), sid, fill=COLOR_ORANGE)
        draw.text((x + 160, ry + 16), name, fill=COLOR_WHITE)
        draw.text((x + 340, ry + 16), status, fill=COLOR_GREEN if status != "Charging" else COLOR_MUTED)
        draw.text((x + 450, ry + 16), bat, fill=COLOR_WHITE)
        ry += 58

    # Cloud & Weather Analytics Panel
    draw_rounded_rect(draw, [x + 25 + col1_w, mid_y, x + w - 15, y + h - 15], radius=6, fill=COLOR_PANEL, outline=COLOR_BORDER)
    draw.text((x + 40 + col1_w, mid_y + 20), "ENVIRONMENT & OPS METRICS", fill=COLOR_ORANGE)
    draw.line([(x + 40 + col1_w, mid_y + 45), (x + w - 30, mid_y + 45)], fill=COLOR_BORDER)

    metrics = [
        ("Wind Speed:", "4.2 m/s (Favorable)"),
        ("Visibility:", "10 km (Clear Sky)"),
        ("Temperature:", "24°C / 75°F"),
        ("Telemetry Upload:", "Active (volador.in)"),
        ("Pilot Operator:", "Capt. V. Sharma (Lead)"),
    ]
    my = mid_y + 65
    for label, val in metrics:
        draw.text((x + 40 + col1_w, my), label, fill=COLOR_MUTED)
        draw.text((x + 180 + col1_w, my), val, fill=COLOR_WHITE)
        my += 34

def render_vehicle_setup(draw, x, y, w, h):
    """4. Vehicle Setup Panel (Sidebar Navigation, Setup Modules, Vehicle Info)."""
    draw_rounded_rect(draw, [x, y, x + w, y + h], radius=8, fill=COLOR_BG, outline=COLOR_BORDER)
    render_header(draw, x, y, w, "VEHICLE SETUP")

    sy = y + 55

    # Setup Sidebar Menu
    side_w = 200
    draw_rounded_rect(draw, [x + 15, sy, x + 15 + side_w, y + h - 15], radius=6, fill=COLOR_PANEL, outline=COLOR_BORDER)

    menu_items = [
        ("Summary", True),
        ("Airframe", False),
        ("Radio", False),
        ("Sensors", False),
        ("Power", False),
        ("Safety", False),
        ("Firmware", False),
    ]
    my = sy + 15
    for item, is_active in menu_items:
        bg = COLOR_ORANGE if is_active else COLOR_CARD
        draw_rounded_rect(draw, [x + 25, my, x + 5 + side_w, my + 38], radius=4, fill=bg, outline=COLOR_BORDER)
        draw.text((x + 40, my + 11), item, fill=COLOR_WHITE)
        my += 48

    # Main Configuration Details Panel
    main_x = x + 30 + side_w
    main_w = w - side_w - 45
    draw_rounded_rect(draw, [main_x, sy, main_x + main_w, y + h - 15], radius=6, fill=COLOR_PANEL, outline=COLOR_BORDER)

    draw.text((main_x + 25, sy + 20), "VOLADOR VEHICLE SUMMARY & PROFILES", fill=COLOR_ORANGE)
    draw.line([(main_x + 25, sy + 48), (main_x + main_w - 25, sy + 48)], fill=COLOR_BORDER)

    params = [
        ("Vehicle Type:", "Volador HexaRotor Professional Drone"),
        ("Autopilot Firmware:", "PX4 Autopilot v1.14 / ArduPilot Compatible"),
        ("Frame Configuration:", "HexaRotor X Pattern"),
        ("Radio Channels:", "16 Channels Configured (SBUS / PPM)"),
        ("Power Module:", "Dual Smart LiPo Monitor (6S / 22.2V)"),
        ("Safety Failsafe:", "Return-To-Launch (RTL) Enabled on Low Battery"),
    ]
    py = sy + 65
    for lbl, val in params:
        draw_rounded_rect(draw, [main_x + 25, py, main_x + main_w - 25, py + 42], radius=4, fill=COLOR_CARD, outline=COLOR_BORDER)
        draw.text((main_x + 40, py + 14), lbl, fill=COLOR_MUTED)
        draw.text((main_x + 220, py + 14), val, fill=COLOR_WHITE)
        py += 52

def main():
    print("Initializing Volador Interface Mockup Generator...")
    
    # 1. Create output folder if it does not exist
    out_dir = r"C:\projects\qgroundcontrol\output"
    os.makedirs(out_dir, exist_ok=True)
    out_filepath = os.path.join(out_dir, "volador_full_app_suite.png")

    # Also save a copy to artifact directory for IDE display
    artifact_dir = r"C:\Users\jithi\.gemini\antigravity-ide\brain\7211ed00-827a-4ce8-86ce-b0fa7ae239a3"
    artifact_filepath = os.path.join(artifact_dir, "volador_full_app_suite.png")

    # 2. Render 1920x1080 Image Canvas
    canvas_w, canvas_h = 1920, 1080
    img = Image.new("RGBA", (canvas_w, canvas_h), COLOR_BG)
    draw = ImageDraw.Draw(img)

    # Layout Grid Metrics (2x2 Grid with 20px padding)
    pad = 20
    panel_w = (canvas_w - (pad * 3)) // 2
    panel_h = (canvas_h - (pad * 3)) // 2

    # Panel Coordinates
    # Top-Left: Flight View
    render_flight_view(draw, pad, pad, panel_w, panel_h)
    
    # Top-Right: Plan View
    render_plan_view(draw, pad * 2 + panel_w, pad, panel_w, panel_h)
    
    # Bottom-Left: Volador Platform Dashboard
    render_platform_dashboard(draw, pad, pad * 2 + panel_h, panel_w, panel_h)
    
    # Bottom-Right: Vehicle Setup
    render_vehicle_setup(draw, pad * 2 + panel_w, pad * 2 + panel_h, panel_w, panel_h)

    # 3. Save Image Outputs
    img.save(out_filepath, format="PNG")
    img.save(artifact_filepath, format="PNG")

    print("================================================================================")
    print(f"SUCCESS: Volador Ground Control UI suite rendered cleanly.")
    print(f"Primary Output File: {out_filepath}")
    print(f"Artifact Copy:      {artifact_filepath}")
    print("================================================================================")

if __name__ == "__main__":
    main()
