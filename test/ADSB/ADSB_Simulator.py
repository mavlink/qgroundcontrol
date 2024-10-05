import socket
import threading
import numpy as np
import datetime
import time

def generate_icao_address():
    return ''.join(np.random.choice(list('0123456789ABCDEF'), 6))

def generate_position():
    lat = np.random.uniform(-90, 90)
    lon = np.random.uniform(-180, 180)
    return lat, lon

def generate_altitude():
    return np.random.randint(0, 40000)

def generate_velocity():
    return np.random.randint(0, 600)

def generate_heading():
    return np.random.randint(0, 360)

def generate_adsb_message():
    icao_address = generate_icao_address()
    lat, lon = generate_position()
    altitude = generate_altitude()
    velocity = generate_velocity()
    heading = generate_heading()
    timestamp = datetime.datetime.utcnow().isoformat()

    # Format message as "MSG,1,..."
    msg_type = np.random.choice([1, 3, 4, 5, 6])
    msg_parts = [
        f"MSG,{msg_type}",
        "",  # transmission type
        "",  # session ID
        "",  # aircraft ID
        icao_address,
        "",  # flight ID
        "",  # date
        "",  # time
        "",  # call sign
        "",  # altitude
        "",  # ground speed
        "",  # track
        "",  # lat
        "",  # lon
        "",  # vertical rate
        "",  # squawk
        "",  # alert
        "",  # emergency
        "",  # SPI
        "",  # is on ground
    ]

    if msg_type in [1, 5, 6]:
        msg_parts[9] = f"CALLSIGN{icao_address}"  # call sign
    if msg_type == 3:
        msg_parts[11] = str(altitude)
        msg_parts[12] = ""
        msg_parts[13] = f"{lat:.5f}"
        msg_parts[14] = f"{lon:.5f}"
    if msg_type == 4:
        msg_parts[12] = str(heading)

    message = ','.join(msg_parts)
    return message

def handle_client(client_socket):
    try:
        while True:
            message = generate_adsb_message()
            client_socket.sendall(message.encode('utf-8') + b'\n')
            time.sleep(1)  # Simulate a delay between messages
    except (ConnectionResetError, BrokenPipeError):
        print("Client disconnected.")
    finally:
        client_socket.close()

def start_server(host='0.0.0.0', port=30003):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((host, port))
    server_socket.listen(5)
    print(f"Server listening on {host}:{port}")

    while True:
        client_socket, addr = server_socket.accept()
        print(f"Accepted connection from {addr}")
        client_handler = threading.Thread(target=handle_client, args=(client_socket,))
        client_handler.start()

if __name__ == "__main__":
    start_server()
