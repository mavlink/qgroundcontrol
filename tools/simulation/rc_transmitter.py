#!/usr/bin/env python3
"""
Virtual RC Transmitter for PX4 SITL

Simulates an RC transmitter when testing QGC against PX4 SITL. A web page at
http://localhost:8080 has a slider per channel; the script streams the channel
PWM values to the vehicle as RC_CHANNELS_OVERRIDE, which PX4 treats as RC
receiver input. QGC stays connected on its own link at the same time.

Usage:
    ./rc_transmitter.py                                     # SITL on this machine (offboard link, UDP 14540)
    ./rc_transmitter.py --connect udpout:10.211.55.7:14580  # SITL in a VM or on another machine
    ./rc_transmitter.py --port 8081 --rate 50               # Other web page port, send rate

PX4 setup (SITL defaults to MAVLink joystick input only):
    param set COM_RC_IN_MODE 0
    param set RC_CHAN_CNT 8
    param set RC_MAP_ROLL 1; param set RC_MAP_PITCH 2; param set RC_MAP_THROTTLE 3; param set RC_MAP_YAW 4

Requirements:
    python tools/setup/install_python.py dev
"""

import argparse
import json
import os
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

# Channels 9-18 of RC_CHANNELS_OVERRIDE are MAVLink 2 extension fields
os.environ.setdefault("MAVLINK20", "1")

from pymavlink import mavutil

CHANNEL_COUNT = 18
PWM_MIN = 800
PWM_MAX = 2200

PAGE = """<!DOCTYPE html>
<html><head><meta charset="utf-8"><title>Virtual RC</title>
<style>
 body { background:#0b0d10; color:#e6e8eb; font:14px -apple-system, sans-serif; margin:16px; }
 h1 { font-size:17px; margin:0 0 4px; }
 #status { color:#8b919a; margin-bottom:12px; }
 .bar { display:flex; gap:10px; align-items:center; margin-bottom:12px; }
 button { background:#181b20; color:#e6e8eb; border:1px solid #2a2e35; border-radius:5px; padding:4px 10px; cursor:pointer; }
 button.on { background:#3A9BDC; border-color:#3A9BDC; }
 .ch { display:grid; grid-template-columns:48px 1fr 70px auto; gap:10px; align-items:center;
       padding:6px 10px; border-top:1px solid #1f2228; }
 input[type=range] { width:100%; }
 input[type=number] { width:64px; background:#181b20; color:#e6e8eb; border:1px solid #2a2e35; border-radius:5px; padding:3px; }
 .presets button { padding:2px 8px; font-size:12px; }
</style></head><body>
<h1>Virtual RC transmitter</h1>
<div id="status">Connecting...</div>
<div class="bar">
 <button id="send" class="on">Transmitting</button>
 <button id="center">All to 1500</button>
 <span style="color:#8b919a">Stop transmitting to test RC loss.</span>
</div>
<div id="channels"></div>
<script>
const MIN = %PWM_MIN%, MAX = %PWM_MAX%;
// One /channel request in flight per channel, so a late older value cannot overwrite a newer one
const latest = {}, inFlight = {};
function post(path, body) {
  return fetch(path, { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify(body) });
}
async function sendChannel(i) {
  inFlight[i] = true;
  let sent;
  do {
    sent = latest[i];
    try {
      await post("/channel", { channel: i, value: sent });
    } catch (e) {
      break;
    }
  } while (latest[i] !== sent);
  inFlight[i] = false;
}
function setChannel(i, value) {
  value = Math.max(MIN, Math.min(MAX, Math.round(value)));
  document.getElementById("r" + i).value = value;
  document.getElementById("n" + i).value = value;
  latest[i] = value;
  if (!inFlight[i]) {
    sendChannel(i);
  }
}
function build(values) {
  const root = document.getElementById("channels");
  values.forEach((v, i) => {
    const row = document.createElement("div");
    row.className = "ch";
    row.innerHTML = `<b>Ch ${i + 1}</b>
      <input type="range" id="r${i}" min="${MIN}" max="${MAX}" value="${v}">
      <input type="number" id="n${i}" min="${MIN}" max="${MAX}" value="${v}">
      <span class="presets"><button>1000</button><button>1500</button><button>2000</button></span>`;
    root.appendChild(row);
    row.querySelector("#r" + i).addEventListener("input", e => setChannel(i, e.target.value));
    row.querySelector("#n" + i).addEventListener("change", e => setChannel(i, e.target.value));
    row.querySelectorAll(".presets button").forEach(b => b.addEventListener("click", () => setChannel(i, b.textContent)));
  });
}
document.getElementById("send").addEventListener("click", async e => {
  const on = !e.target.classList.contains("on");
  await post("/transmit", { on: on });
  e.target.classList.toggle("on", on);
  e.target.textContent = on ? "Transmitting" : "Stopped";
});
document.getElementById("center").addEventListener("click", () => {
  for (let i = 0; i < %CHANNEL_COUNT%; i++) setChannel(i, 1500);
});
async function poll() {
  try {
    const s = await (await fetch("/state")).json();
    if (!document.getElementById("r0")) build(s.values);
    const sendButton = document.getElementById("send");
    sendButton.classList.toggle("on", s.transmitting);
    sendButton.textContent = s.transmitting ? "Transmitting" : "Stopped";
    document.getElementById("status").textContent = s.connected ?
      `Vehicle ${s.target_system} on ${s.connection}, ${s.transmitting ? "sending at " + s.rate_hz + " Hz" : "not sending"}` :
      `Waiting for a heartbeat on ${s.connection}...`;
  } catch (e) {
    document.getElementById("status").textContent = "Transmitter script not running";
  }
  setTimeout(poll, 1000);
}
poll();
</script></body></html>
"""


class Transmitter:
    def __init__(self, connection: str, rate_hz: float):
        self.connection_string = connection
        self.rate_hz = rate_hz
        self.values = [1500] * CHANNEL_COUNT
        self.transmitting = True
        self.target_system = 0
        self.lock = threading.Lock()
        mav = mavutil.mavlink_connection(connection, source_system=255, source_component=190)
        if not isinstance(mav, mavutil.mavfile):
            raise ValueError("A live MAVLink connection is required, not a log file")
        self.mav = mav

    def run(self) -> None:
        threading.Thread(target=self._receive, daemon=True).start()
        period = 1.0 / self.rate_hz
        last_heartbeat = 0.0
        while True:
            # Our heartbeat lets a vehicle reached by udpout learn where to send its own
            now = time.monotonic()
            if now - last_heartbeat >= 1.0:
                last_heartbeat = now
                self.mav.mav.heartbeat_send(
                    mavutil.mavlink.MAV_TYPE_GCS, mavutil.mavlink.MAV_AUTOPILOT_INVALID, 0, 0, 0
                )
            with self.lock:
                values = list(self.values)
                send = self.transmitting and self.target_system != 0
            if send:
                self.mav.mav.rc_channels_override_send(self.target_system, 1, *values)
            time.sleep(period)

    def _receive(self) -> None:
        while True:
            msg = self.mav.recv_match(type="HEARTBEAT", blocking=True, timeout=1)
            if msg is None or msg.get_srcComponent() != mavutil.mavlink.MAV_COMP_ID_AUTOPILOT1:
                continue
            with self.lock:
                if self.target_system != msg.get_srcSystem():
                    self.target_system = msg.get_srcSystem()
                    print(f"Vehicle {self.target_system} found, streaming RC at {self.rate_hz} Hz")

    def state(self) -> dict:
        with self.lock:
            return {
                "values": list(self.values),
                "transmitting": self.transmitting,
                "connected": self.target_system != 0,
                "target_system": self.target_system,
                "connection": self.connection_string,
                "rate_hz": self.rate_hz,
            }

    def set_channel(self, channel: int, value: int) -> None:
        if 0 <= channel < CHANNEL_COUNT:
            with self.lock:
                self.values[channel] = max(PWM_MIN, min(PWM_MAX, int(value)))

    def set_transmitting(self, on: bool) -> None:
        with self.lock:
            self.transmitting = on


def make_handler(transmitter: Transmitter):
    page = (
        PAGE.replace("%PWM_MIN%", str(PWM_MIN))
        .replace("%PWM_MAX%", str(PWM_MAX))
        .replace("%CHANNEL_COUNT%", str(CHANNEL_COUNT))
        .encode()
    )

    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *args) -> None:
            pass

        def _reply(self, body: bytes, content_type: str, status: int = 200) -> None:
            self.send_response(status)
            self.send_header("Content-Type", content_type)
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)

        def do_GET(self) -> None:
            if self.path == "/state":
                self._reply(json.dumps(transmitter.state()).encode(), "application/json")
            else:
                self._reply(page, "text/html; charset=utf-8")

        def do_POST(self) -> None:
            # A cross-origin page can send a text/plain POST without a preflight; JSON needs one
            if self.headers.get("Content-Type", "").split(";")[0].strip() != "application/json":
                self._reply(
                    b'{"error": "Content-Type must be application/json"}', "application/json", 415
                )
                return
            if self.path not in ("/channel", "/transmit"):
                self._reply(b'{"error": "not found"}', "application/json", 404)
                return
            try:
                body = json.loads(
                    self.rfile.read(int(self.headers.get("Content-Length", 0))) or b"{}"
                )
                if self.path == "/channel":
                    transmitter.set_channel(int(body["channel"]), int(body["value"]))
                else:
                    transmitter.set_transmitting(bool(body["on"]))
            except (ValueError, KeyError, TypeError) as error:
                self._reply(json.dumps({"error": str(error)}).encode(), "application/json", 400)
                return
            self._reply(b"{}", "application/json")

    return Handler


def main() -> None:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        "--connect", default="udpin:0.0.0.0:14540", help="pymavlink connection to the vehicle"
    )
    parser.add_argument("--port", type=int, default=8080, help="web page port")
    parser.add_argument("--rate", type=float, default=20.0, help="RC_CHANNELS_OVERRIDE rate in Hz")
    args = parser.parse_args()
    if args.rate <= 0:
        parser.error("--rate must be greater than 0")

    transmitter = Transmitter(args.connect, args.rate)
    server = ThreadingHTTPServer(("127.0.0.1", args.port), make_handler(transmitter))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    print(f"Virtual RC transmitter: http://localhost:{args.port}  (vehicle link {args.connect})")
    transmitter.run()


if __name__ == "__main__":
    main()
