#!/usr/bin/env python3

import argparse
import json
import socket
import socketserver


class AtlasRequestHandler(socketserver.StreamRequestHandler):

    def handle(self):
        client_ip, client_port = self.client_address

        print(
            f"[MOCK] Cliente ligado: "
            f"{client_ip}:{client_port}",
            flush=True
        )

        self.request.setsockopt(
            socket.IPPROTO_TCP,
            socket.TCP_NODELAY,
            1
        )

        hello = {
            "type": "hello",
            "server": "ATLAS-Jetson-Mock",
            "protocol": 1,
            "a8_connected": False
        }

        self.wfile.write(
            (
                json.dumps(hello) + "\n"
            ).encode("utf-8")
        )

        self.wfile.flush()

        while True:

            line = self.rfile.readline()

            if not line:
                break

            try:
                message = json.loads(
                    line.decode("utf-8")
                )

            except json.JSONDecodeError as error:

                print(
                    f"[MOCK] JSON inválido: {error}",
                    flush=True
                )

                continue

            message_type = message.get(
                "type",
                "unknown"
            )

            if message_type == "gimbal_rate":

                yaw = message.get("yaw", 0)
                pitch = message.get("pitch", 0)

                print(
                    f"[GIMBAL] "
                    f"yaw={yaw:6.1f} "
                    f"pitch={pitch:6.1f}",
                    flush=True
                )

            elif message_type == "gimbal_stop":

                print(
                    "[GIMBAL] STOP",
                    flush=True
                )

            elif message_type == "gimbal_center":

                print(
                    "[GIMBAL] CENTER",
                    flush=True
                )

            elif message_type == "zoom":

                print(
                    f"[CAMERA] ZOOM "
                    f"{message.get('value', 0)}",
                    flush=True
                )

            elif message_type == "photo":

                print(
                    "[CAMERA] PHOTO",
                    flush=True
                )

            elif message_type == "record":

                enabled = message.get(
                    "enabled",
                    False
                )

                print(
                    "[CAMERA] RECORD "
                    + (
                        "START"
                        if enabled
                        else "STOP"
                    ),
                    flush=True
                )

            elif message_type == "hello":

                print(
                    f"[MOCK] HELLO: {message}",
                    flush=True
                )

            else:

                print(
                    f"[MOCK] RX: {message}",
                    flush=True
                )

        print(
            f"[MOCK] Cliente desligado: "
            f"{client_ip}:{client_port}",
            flush=True
        )


class AtlasTcpServer(socketserver.ThreadingTCPServer):

    allow_reuse_address = True
    daemon_threads = True


def main():

    parser = argparse.ArgumentParser(
        description="ATLAS Jetson mock server"
    )

    parser.add_argument(
        "--host",
        default="127.0.0.1"
    )

    parser.add_argument(
        "--port",
        type=int,
        default=45900
    )

    args = parser.parse_args()

    with AtlasTcpServer(
        (args.host, args.port),
        AtlasRequestHandler
    ) as server:

        print(
            f"[MOCK] ATLAS Jetson server em "
            f"{args.host}:{args.port}",
            flush=True
        )

        server.serve_forever()


if __name__ == "__main__":
    main()
