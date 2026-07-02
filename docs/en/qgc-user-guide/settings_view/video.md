# Video

Configure video streaming and recording settings.

## Video Source

- **Source** — Video Stream Disabled / RTSP Video Stream / UDP h.264 / UDP h.265 / TCP-MPEG2 / MPEG-TS / HTTP MJPEG / WebSocket JPEG / Integrated Camera

HTTP MJPEG and WebSocket JPEG are available only in builds that include the
required GStreamer and Qt WebSockets components.

## Connection

Connection settings vary by source type:

- **RTSP URL** — full RTSP stream address
- **TCP URL** — TCP stream address
- **UDP URL** — UDP stream address and port (default: `0.0.0.0:5600`)
- **HTTP MJPEG URL** — `http://` or `https://` multipart
  `multipart/x-mixed-replace` MJPEG stream
- **WebSocket JPEG URL** — `ws://` or `wss://` endpoint where each binary
  WebSocket message contains one complete JPEG image; text messages are ignored

Do not put credentials in a video URL, including its query string. QGroundControl
removes URL user information, query strings, and fragments from video logs, but
credentials should still be supplied through **Network Video Security**.

## Network Video Security

Anonymous `http://` and `ws://` streams are intended for quick trials on a
trusted local network. They provide no confidentiality or peer authentication.
Do not expose them to an untrusted LAN or the public Internet.

For an operational deployment:

1. Configure the source server with HTTPS or WSS.
2. Select **Basic** or **Bearer token** authentication.
3. Enter a session password/token, or on Unix-like systems select an owner-only
   credential file.
4. Optionally configure an exact **Origin** value or an additional CA
   certificate required by the server.

Authenticated streams are rejected over plaintext HTTP or WS. Authenticated HTTP
redirects are disabled, TLS certificate validation remains enabled, and
QGroundControl does not provide an option to ignore certificate errors.

### Credentials

- A credential entered in **Session password** or **Session bearer token** is
  held only for the current QGroundControl process and is not written to
  settings.
- Basic-auth usernames cannot contain colon, NUL, CR, or LF characters.
- For unattended Unix-like installations, **Credential file** may point to a
  regular, non-symbolic-link file containing exactly one non-empty line. The
  file must be owned by the QGroundControl process user, have exactly one hard
  link, be readable by its owner, and be inaccessible to group/other users (for
  example, mode `0600`).
- The session credential takes precedence over the credential file.

The **Origin** field accepts only an HTTP/HTTPS origin containing a scheme,
host, and optional port. **CA certificate file** accepts PEM certificates while
keeping strict verification enabled. WSS adds those authorities to normal
system trust. For HTTPS MJPEG, GIO uses the selected PEM file as the complete
trust database, so include every root required by that deployment; leave the
field blank to use normal system trust alone.

GStreamer diagnostics redact Authorization/password-bearing messages and URL
queries. Pipeline DOT files omit element parameters, so credentials configured
for network video are not exported in debug graphs.

HTTP MJPEG and WebSocket JPEG can be recorded in MKV or MOV containers. MP4
does not accept the incoming JPEG stream and is rejected with an operator
message; select MKV or MOV before recording these sources.

## Settings

- **Aspect Ratio** — aspect ratio for scaling video in the display widget (default: 16:9; set to 0.0 to disable scaling)
- **Stop recording when disarmed** — automatically stop recording when the vehicle disarms
- **Low Latency Mode** — minimize video decode latency at the cost of some smoothness
- **Force CPU video path** — disable hardware-accelerated rendering (use if video displays incorrectly)
- **Force video decoder priority** — override the automatic decoder selection: Default / Software / Hardware / NVIDIA / VA-API / DirectX 3D11 / VideoToolbox / Intel / Vulkan

## Local Video Storage

- **Record File Format** — mp4 / mov / mkv
- **Auto-Delete Saved Recordings** — automatically delete old recordings when storage limit is reached
- **Max Storage Usage** — maximum disk space for saved recordings (default: 10240 MB)
