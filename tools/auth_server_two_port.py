#!/usr/bin/env python3
"""
Two-port authentication server for QGroundControl AuthenticatedSerialLink
- Port 8080: Authentication (login/logout)  
- Port 5760: Data communication with session token validation
"""

import socket
import json
import threading
import time
import struct

class AuthServer:
    def __init__(self, auth_host='127.0.0.1', auth_port=8080, data_host='127.0.0.1', data_port=5760):
        self.auth_host = auth_host
        self.auth_port = auth_port
        self.data_host = data_host
        self.data_port = data_port
        
        self.users = {
            'admin': 'password123',
            'user1': 'mypass',
            'drone_operator': 'securepass',
            'test': 'test'
        }
        self.active_sessions = {}
        self.running = False
        
    def generate_session_token(self, username):
        """Generate a simple session token"""
        timestamp = int(time.time())
        return f"{username}_{timestamp}_token"
    
    def authenticate_user(self, username, password):
        """Check if username/password is valid"""
        return self.users.get(username) == password
    
    def validate_session_token(self, token):
        """Check if session token is valid"""
        return token in self.active_sessions

    def handle_auth_client(self, client_socket, client_address):
        """Handle authentication requests on auth port"""
        print(f"[AUTH] Connection from {client_address}")
        
        try:
            data = client_socket.recv(1024).decode('utf-8').strip()
            print(f"[AUTH] Received: {data}")
            
            try:
                request = json.loads(data)
            except json.JSONDecodeError:
                response = {"status": "error", "message": "Invalid JSON format"}
                client_socket.send(json.dumps(response).encode('utf-8'))
                return
            
            if request.get('action') == 'login':
                username = request.get('username', '')
                password = request.get('password', '')
                
                if self.authenticate_user(username, password):
                    session_token = self.generate_session_token(username)
                    self.active_sessions[session_token] = {
                        'username': username,
                        'login_time': time.time()
                    }
                    
                    response = {
                        "status": "success",
                        "message": f"Welcome {username}!",
                        "session_token": session_token,
                        "data_server": {
                            "host": self.data_host,
                            "port": self.data_port
                        }
                    }
                    print(f"[AUTH] Authentication successful for user: {username}")
                else:
                    response = {"status": "error", "message": "Invalid username or password"}
                    print(f"[AUTH] Authentication failed for user: {username}")
            else:
                response = {"status": "error", "message": "Unknown action"}
            
            response_json = json.dumps(response) + '\n'
            client_socket.send(response_json.encode('utf-8'))
            
        except Exception as e:
            print(f"[AUTH] Error handling client {client_address}: {e}")
            error_response = {"status": "error", "message": "Server error occurred"}
            try:
                client_socket.send(json.dumps(error_response).encode('utf-8'))
            except:
                pass
        finally:
            client_socket.close()
            print(f"[AUTH] Connection closed with {client_address}")

    def handle_data_client(self, client_socket, client_address):
        """Handle data communication on data port"""
        print(f"[DATA] Connection from {client_address}")
        session_token = None
        authenticated = False
        
        try:
            while True:
                data = client_socket.recv(4096)
                if not data:
                    break
                
                if not authenticated:
                    # First message should be session token authentication
                    try:
                        auth_data = json.loads(data.decode('utf-8').strip())
                        if auth_data.get('action') == 'authenticate':
                            token = auth_data.get('session_token', '')
                            if self.validate_session_token(token):
                                session_token = token
                                authenticated = True
                                response = {"status": "success", "message": "Data connection authenticated"}
                                client_socket.send(json.dumps(response).encode('utf-8') + b'\n')
                                print(f"[DATA] Session authenticated for token: {token[:20]}...")
                            else:
                                response = {"status": "error", "message": "Invalid session token"}
                                client_socket.send(json.dumps(response).encode('utf-8'))
                                break
                        else:
                            response = {"status": "error", "message": "Authentication required"}
                            client_socket.send(json.dumps(response).encode('utf-8'))
                            break
                    except json.JSONDecodeError:
                        response = {"status": "error", "message": "Invalid authentication data"}
                        client_socket.send(json.dumps(response).encode('utf-8'))
                        break
                else:
                    # Handle MAVLink data (echo back for demo)
                    print(f"[DATA] Received {len(data)} bytes of MAVLink data")
                    
                    # Echo the data back (in real application, forward to drone)
                    client_socket.send(data)
                    
                    # Log some MAVLink packet info if it looks like MAVLink
                    if len(data) >= 8 and data[0] in [0xFE, 0xFD]:  # MAVLink v1 or v2 magic
                        print(f"[DATA] MAVLink packet: magic=0x{data[0]:02X}, length={len(data)}")
        
        except Exception as e:
            print(f"[DATA] Error handling client {client_address}: {e}")
        finally:
            if session_token:
                print(f"[DATA] Closing authenticated session: {session_token[:20]}...")
            client_socket.close()
            print(f"[DATA] Connection closed with {client_address}")

    def start_auth_server(self):
        """Start authentication server"""
        auth_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        auth_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
        try:
            auth_socket.bind((self.auth_host, self.auth_port))
            auth_socket.listen(5)
            print(f"[AUTH] Authentication server started on {self.auth_host}:{self.auth_port}")
            
            while self.running:
                try:
                    client_socket, client_address = auth_socket.accept()
                    client_thread = threading.Thread(
                        target=self.handle_auth_client,
                        args=(client_socket, client_address)
                    )
                    client_thread.daemon = True
                    client_thread.start()
                except socket.error:
                    if self.running:
                        print("[AUTH] Socket error occurred")
                    break
        except Exception as e:
            print(f"[AUTH] Server error: {e}")
        finally:
            auth_socket.close()
            print("[AUTH] Authentication server stopped")

    def start_data_server(self):
        """Start data communication server"""
        data_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        data_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
        try:
            data_socket.bind((self.data_host, self.data_port))
            data_socket.listen(5)
            print(f"[DATA] Data server started on {self.data_host}:{self.data_port}")
            
            while self.running:
                try:
                    client_socket, client_address = data_socket.accept()
                    client_thread = threading.Thread(
                        target=self.handle_data_client,
                        args=(client_socket, client_address)
                    )
                    client_thread.daemon = True
                    client_thread.start()
                except socket.error:
                    if self.running:
                        print("[DATA] Socket error occurred")
                    break
        except Exception as e:
            print(f"[DATA] Server error: {e}")
        finally:
            data_socket.close()
            print("[DATA] Data server stopped")

    def start(self):
        """Start both servers"""
        self.running = True
        
        print("=== Two-Port Authentication Server ===")
        print(f"Auth Server: {self.auth_host}:{self.auth_port}")
        print(f"Data Server: {self.data_host}:{self.data_port}")
        print("Available users:")
        for username, password in self.users.items():
            print(f"  Username: {username}, Password: {password}")
        print("\nStarting servers...")
        
        # Start both servers in separate threads
        auth_thread = threading.Thread(target=self.start_auth_server)
        data_thread = threading.Thread(target=self.start_data_server)
        
        auth_thread.daemon = True
        data_thread.daemon = True
        
        auth_thread.start()
        data_thread.start()
        
        print("Both servers started. Waiting for connections...")
        
        try:
            while self.running:
                time.sleep(1)
        except KeyboardInterrupt:
            print("\nStopping servers...")
            self.stop()
    
    def stop(self):
        """Stop both servers"""
        self.running = False

if __name__ == "__main__":
    # Create and start the two-port authentication server
    auth_server = AuthServer()
    
    try:
        auth_server.start()
    except KeyboardInterrupt:
        print("\nStopping server...")
        auth_server.stop()
    except Exception as e:
        print(f"Unexpected error: {e}")
        auth_server.stop()
