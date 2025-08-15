#!/usr/bin/env python3
"""
Simple authentication server for testing QGroundControl AuthenticatedSerialLink
This server listens on port 8080 and accepts JSON login requests
"""

import socket
import json
import threading
import time

class AuthServer:
    def __init__(self, host='127.0.0.1', port=8080):
        self.host = host
        self.port = port
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
    
    def handle_client(self, client_socket, client_address):
        """Handle a client connection"""
        print(f"Connection from {client_address}")
        
        try:
            # Receive data from client
            data = client_socket.recv(1024).decode('utf-8').strip()
            print(f"Received: {data}")
            
            # Parse JSON request
            try:
                request = json.loads(data)
            except json.JSONDecodeError:
                response = {
                    "status": "error",
                    "message": "Invalid JSON format"
                }
                client_socket.send(json.dumps(response).encode('utf-8'))
                return
            
            # Handle login request
            if request.get('action') == 'login':
                username = request.get('username', '')
                password = request.get('password', '')
                
                if self.authenticate_user(username, password):
                    # Generate session token
                    session_token = self.generate_session_token(username)
                    self.active_sessions[session_token] = {
                        'username': username,
                        'login_time': time.time()
                    }
                    
                    response = {
                        "status": "success",
                        "message": f"Welcome {username}!",
                        "session_token": session_token,
                        "user_info": {
                            "username": username,
                            "permissions": ["serial_access", "data_read", "data_write"]
                        }
                    }
                    print(f"Authentication successful for user: {username}")
                else:
                    response = {
                        "status": "error",
                        "message": "Invalid username or password"
                    }
                    print(f"Authentication failed for user: {username}")
            else:
                response = {
                    "status": "error",
                    "message": "Unknown action"
                }
            
            # Send response
            response_json = json.dumps(response) + '\n'
            client_socket.send(response_json.encode('utf-8'))
            
        except Exception as e:
            print(f"Error handling client {client_address}: {e}")
            error_response = {
                "status": "error",
                "message": "Server error occurred"
            }
            try:
                client_socket.send(json.dumps(error_response).encode('utf-8'))
            except:
                pass
        finally:
            client_socket.close()
            print(f"Connection closed with {client_address}")
    
    def start(self):
        """Start the authentication server"""
        self.running = True
        
        # Create socket
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
        try:
            server_socket.bind((self.host, self.port))
            server_socket.listen(5)
            
            print(f"Authentication server started on {self.host}:{self.port}")
            print("Available users:")
            for username, password in self.users.items():
                print(f"  Username: {username}, Password: {password}")
            print("\nWaiting for connections...")
            
            while self.running:
                try:
                    client_socket, client_address = server_socket.accept()
                    # Handle each client in a separate thread
                    client_thread = threading.Thread(
                        target=self.handle_client,
                        args=(client_socket, client_address)
                    )
                    client_thread.daemon = True
                    client_thread.start()
                except socket.error:
                    if self.running:
                        print("Socket error occurred")
                    break
                    
        except Exception as e:
            print(f"Server error: {e}")
        finally:
            server_socket.close()
            print("Server stopped")
    
    def stop(self):
        """Stop the authentication server"""
        self.running = False

if __name__ == "__main__":
    # Create and start the authentication server
    auth_server = AuthServer()
    
    try:
        auth_server.start()
    except KeyboardInterrupt:
        print("\nStopping server...")
        auth_server.stop()
    except Exception as e:
        print(f"Unexpected error: {e}")
        auth_server.stop()
