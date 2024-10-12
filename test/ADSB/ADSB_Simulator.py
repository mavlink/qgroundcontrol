import socket
import threading
import numpy as np
import datetime
import time

class Aircraft:
    def __init__(self):
        self.icao_address = self.generate_icao_address()
        # self.lat = np.random.uniform(-90, 90)
        # self.lon = np.random.uniform(-180, 180)
        self.lat = np.random.uniform(34.8, 41.8)  # Latitude range for Greece
        self.lon = np.random.uniform(19.8, 29.6)  # Longitude range for Greece
        self.altitude = np.random.randint(0, 40000)
        self.velocity = np.random.randint(50, 600)  
        self.heading = np.random.randint(0, 360)
        self.verticalRate = np.random.randint(-3000, 3000)  # Vertical rate in feet per minute
        # Generate a valid squawk code as a four-digit octal number ranging from 0000 to 7777.
        # Squawk codes are assigned by ATC and represent a specific aircraft's transponder setting.
        # We use `np.random.randint(0, 4096)` to generate values from 0 to 4095 (decimal), 
        # and then format it as an octal string with `{value:04o}` to ensure the correct 4-digit format.
        self.squawk = f"{np.random.randint(0, 4096):04o}"

    def generate_icao_address(self):
        # Generate a random ICAO address in the format of 6 hexadecimal digits
        return '{:06X}'.format(np.random.randint(0, 0x1000000))  # Generates a number from 0 to 16777215

    def update(self):
        # Update position to move in a more linear direction
        distance_movement = np.random.uniform(0.05, 0.15)  # Small constant step for smooth movement
        self.lat += distance_movement * np.cos(np.radians(self.heading))
        self.lon += distance_movement * np.sin(np.radians(self.heading))

        # Ensure latitude and longitude remain within realistic ranges for Greece
        self.lat = max(34.0, min(self.lat, 42.0))  # Latitude range in Greece
        self.lon = max(19.0, min(self.lon, 29.0))  # Longitude range in Greece

        # Altitude can change gradually (e.g., climbing or descending)
        self.altitude += np.random.choice([-20, 20, 0])  # Climb or descend 20 feet or stay level
        self.altitude = max(0, self.altitude)  # Ensure altitude doesn't go below 0

        # Velocity can change with small gradual adjustments
        self.velocity += np.random.uniform(-2, 2)  # Smooth adjustments to speed
        self.velocity = max(0, min(self.velocity, 600))  # Ensure velocity is within bounds

        # Gradually update heading to simulate a more consistent direction
        heading_change = np.random.uniform(-1, 1)  # Slightly adjust heading
        self.heading = (self.heading + heading_change) % 360  # Keep heading in 0-360 range


    def generate_adsb_message(self):
        timestamp = datetime.datetime.now(datetime.timezone.utc).isoformat()

        # Format message as "MSG,1,..."
        msg_type = np.random.choice([1, 3, 4, 5, 6, 8])
        msg_parts = [
            "MSG",  # Field 1: Message type
            str(msg_type), # Field 2: Transmission Type
            "",  # Index 2 | Field 3: Session ID
            "",  # Index 3 | Field 4: Aircraft ID
            self.icao_address,  # Field 5: HexIdent
            "",  # Index 4 | Field 6: Flight ID   
            "",  # Index 5 | Field 7: Date message generated
            "",  # Index 6 | Field 8: Time message generated
            "",  # Index 7 | Field 9: Date message logged
            "",  # Index 8 | Field 10: Time message logged
            "",  # Field 11: Callsign
            "",  # Field 12: Altitude
            "",  # Field 13: GroundSpeed
            "",  # Field 14: Track
            "",  # Field 15: Latitude
            "",  # Field 16: Longitude
            "",  # Field 17: VerticalRate
            "",  # Field 18: Squawk
            "",  # Field 19: Alert
            "",  # Field 20: Emergency
            "",  # Field 21: SPI
            "",  # Field 22: IsOnGround
        ]

        if msg_type == 1:
            msg_parts[9] = f"CS{self.icao_address}"
        if msg_type == 3:
            msg_parts[11] = str(self.altitude)
            msg_parts[14] = f"{self.lat:.5f}"
            msg_parts[15] = f"{self.lon:.5f}"
            msg_parts[18] = "0"  # Field 19: Alert (0 means no alert)
            msg_parts[19] = "0"  # Field 20: Emergency (0 means no emergency)
            msg_parts[20] = "0"  # Field 21: SPI (0 means no SPI)
            msg_parts[21] = "0"  # Field 22: IsOnGround (0 means the aircraft is airborne)
        if msg_type == 4:
            msg_parts[12] = str(int(self.velocity))  # GroundSpeed as an integer
            msg_parts[13] = str(int(self.heading))   # Track (Heading) as an integer
            msg_parts[16] = str(self.verticalRate)
            msg_parts[18] = "0"  
            msg_parts[19] = "0"  
            msg_parts[20] = "0"  
            msg_parts[21] = "0"
        if msg_type == 5:
            msg_parts[11] = str(self.altitude)  
            msg_parts[18] = "0"  
            msg_parts[19] = "0"  
            msg_parts[20] = "0"  
            msg_parts[21] = "0"
        if msg_type == 6:
            msg_parts[17] = str(self.squawk)
            msg_parts[18] = "0"  
            msg_parts[19] = "0"  
            msg_parts[20] = "0"  
            msg_parts[21] = "0"
        if msg_type == 8:
            msg_parts[21] = "0"
                 

        # Join the message parts correctly
        message = ','.join(msg_parts)
        return message

def handle_client(client_socket, aircrafts):
    try:
        while True:
            # Update aircraft positions before generating messages
            for aircraft in aircrafts:
                aircraft.update()
                
            messages = [aircraft.generate_adsb_message() for aircraft in aircrafts]
            for message in messages:
                client_socket.sendall(message.encode('utf-8') + b'\n')
            time.sleep(1)  # Simulate a delay between message batches
    except (ConnectionResetError, BrokenPipeError):
        print("Client disconnected.")
    finally:
        client_socket.close()

def start_server(host='0.0.0.0', port=30003):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((host, port))
    server_socket.listen(5)
    print(f"Server listening on {host}:{port}")

    # Create 5 aircraft with unique ICAO addresses
    aircrafts = [Aircraft() for _ in range(5)]

    while True:
        client_socket, addr = server_socket.accept()
        print(f"Accepted connection from {addr}")
        client_handler = threading.Thread(target=handle_client, args=(client_socket, aircrafts))
        client_handler.start()

if __name__ == "__main__":
    start_server()
