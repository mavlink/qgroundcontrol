#!/usr/bin/env python3

import argparse
import json
import socket
import socketserver
import sys
import threading


class AtlasState:
    def __init__(self):
        self.lock = threading.Lock()

        self.clients = set()

        self.hm30_connected = False
        self.a8_connected = False

        self.frozen = False

    def add_client(self, client):
        with self.lock:
            self.clients.add(client)

    def remove_client(self, client):
        with self.lock:
            self.clients.discard(client)

    def snapshot(self):
        with self.lock:
            return {
                "hm30_connected": self.hm30_connected,
                "a8_connected": self.a8_connected,
            }

    def is_frozen(self):
        with self.lock:
            return self.frozen

    def set_frozen(self, frozen):
        with self.lock:
            self.frozen = frozen

        print(
            "[MOCK] Serviço "
            + ("CONGELADO" if frozen else "ATIVO"),
            flush=True,
        )

        if not frozen:
            self.broadcast_status()

    def set_status(self, name, value):
        with self.lock:
            if name == "hm30":
                self.hm30_connected = value

            elif name == "a8":
                self.a8_connected = value

            else:
                return

        self.broadcast_status()

    def set_all(self, value):
        with self.lock:
            self.hm30_connected = value
            self.a8_connected = value

        self.broadcast_status()

    def broadcast_status(self):
        if self.is_frozen():
            return

        message = {
            "type": "status",
            **self.snapshot(),
        }

        data = (
            json.dumps(message) + "\n"
        ).encode("utf-8")

        with self.lock:
            clients = list(self.clients)

        for client in clients:
            try:
                client.sendall(data)

            except OSError:
                self.remove_client(client)


STATE = AtlasState()


def send_json(sock, message):
    data = (
        json.dumps(message) + "\n"
    ).encode("utf-8")

    sock.sendall(data)


class AtlasRequestHandler(socketserver.StreamRequestHandler):
    def handle(self):
        client_ip, client_port = self.client_address

        print(
            f"[MOCK] Cliente ligado: {client_ip}:{client_port}",
            flush=True,
        )

        self.request.setsockopt(
            socket.IPPROTO_TCP,
            socket.TCP_NODELAY,
            1,
        )

        STATE.add_client(
            self.request
        )

        # Se o serviço estiver congelado, simulamos um processo
        # que mantém TCP aberto mas não responde.
        if not STATE.is_frozen():
            send_json(
                self.request,
                {
                    "type": "hello",
                    "server": "ATLAS-Jetson-Mock",
                    "protocol": 1,
                    **STATE.snapshot(),
                },
            )

        try:
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
                        "[MOCK] JSON inválido:",
                        error,
                        flush=True,
                    )

                    continue

                # ---------------------------------------------------------
                # FREEZE

                # Mantemos o TCP aberto mas ignoramos os comandos.
                #--------------------------------------------------------

                if STATE.is_frozen():
                    continue

                message_type = message.get(
                    "type",
                    "unknown",
                )

                # ---------------------------------------------------------
                # HEARTBEAT
                #---------------------------------------------------------

                if message_type == "ping":
                    send_json(
                        self.request,
                        {
                            "type": "pong",
                            "ping_seq": message.get("seq"),
                            **STATE.snapshot(),
                        },
                    )

                elif message_type == "gimbal_rate":
                    yaw = message.get(
                        "yaw",
                        0,
                    )

                    pitch = message.get(
                        "pitch",
                        0,
                    )

                    print(
                        f"[GIMBAL] yaw={yaw:6.1f} pitch={pitch:6.1f}",
                        flush=True,
                    )

                elif message_type == "gimbal_stop":
                    print(
                        "[GIMBAL] STOP",
                        flush=True,
                    )

                elif message_type == "gimbal_center":
                    print(
                        "[GIMBAL] CENTER",
                        flush=True,
                    )

                elif message_type == "zoom":
                    print(
                        f"[CAMERA] ZOOM {message.get('value', 0)}",
                        flush=True,
                    )

                elif message_type == "photo":
                    print(
                        "[CAMERA] PHOTO",
                        flush=True,
                    )

                elif message_type == "record":
                    enabled = message.get(
                        "enabled",
                        False,
                    )

                    print(
                        "[CAMERA] RECORD "
                        + (
                            "START"
                            if enabled
                            else "STOP"
                        ),
                        flush=True,
                    )

                elif message_type == "hello":
                    print(
                        f"[MOCK] HELLO: {message}",
                        flush=True,
                    )

                else:
                    print(
                        f"[MOCK] RX: {message}",
                        flush=True,
                    )

        finally:
            STATE.remove_client(
                self.request
            )

            print(
                f"[MOCK] Cliente desligado: {client_ip}:{client_port}",
                flush=True,
            )


class AtlasTcpServer(socketserver.ThreadingTCPServer):
    allow_reuse_address = True
    daemon_threads = True


def console_loop():
    print()
    print("Comandos do mock:")
    print("  hm30 on")
    print("  hm30 off")
    print("  a8 on")
    print("  a8 off")
    print("  all on")
    print("  all off")
    print("  freeze on")
    print("  freeze off")
    print("  status")
    print()

    while True:
        line = sys.stdin.readline()

        if not line:
            return

        command = line.strip().lower()

        if command == "hm30 on":
            STATE.set_status(
                "hm30",
                True,
            )

        elif command == "hm30 off":
            STATE.set_status(
                "hm30",
                False,
            )

        elif command == "a8 on":
            STATE.set_status(
                "a8",
                True,
            )

        elif command == "a8 off":
            STATE.set_status(
                "a8",
                False,
            )

        elif command == "all on":
            STATE.set_all(True)

        elif command == "all off":
            STATE.set_all(False)

        elif command == "freeze on":
            STATE.set_frozen(True)

        elif command == "freeze off":
            STATE.set_frozen(False)

        elif command == "status":
            print(
                "[MOCK STATUS]",
                {
                    **STATE.snapshot(),
                    "frozen": STATE.is_frozen(),
                },
                flush=True,
            )

        elif command:
            print(
                "[MOCK] Comando desconhecido:",
                command,
                flush=True,
            )


def main():
    parser = argparse.ArgumentParser(
        description="ATLAS Jetson mock server"
    )

    parser.add_argument(
        "--host",
        default="127.0.0.1",
    )

    parser.add_argument(
        "--port",
        type=int,
        default=45900,
    )

    args = parser.parse_args()

    with AtlasTcpServer(
        (args.host, args.port),
        AtlasRequestHandler,
    ) as server:

        print(
            f"[MOCK] ATLAS Jetson server em {args.host}:{args.port}",
            flush=True,
        )

        thread = threading.Thread(
            target=console_loop,
            daemon=True,
        )

        thread.start()

        try:
            server.serve_forever()

        except KeyboardInterrupt:
            print()
            print(
                "[MOCK] Servidor terminado.",
                flush=True,
            )


if __name__ == "__main__":
    main()
