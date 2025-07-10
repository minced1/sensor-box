import socket
import csv
from datetime import datetime

# ESP32's AP IP and port
HOST = "192.168.4.1"
PORT = 80

# Open CSV file for logging
with open("log.csv", mode="a", newline="") as file:
    writer = csv.writer(file)
    writer.writerow(["timestamp", "air_temperature", "air_humidity", "soil_temperature", "soil_humidity"])  # Header (only written once if new file)

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((HOST, PORT))
        print("Connected to ESP32")

        try:
            while True:
                data = s.recv(1024)
                if not data:
                    continue
                message = data.decode().strip()
                timestamp = datetime.now().isoformat(timespec='seconds')
                print(f"[{timestamp}] Received: {message}")
                values = [timestamp]
                values = values + message.split(',')

                # Write to CSV
                writer.writerow(values)

        except KeyboardInterrupt:
            print("Logging stopped by user.")
