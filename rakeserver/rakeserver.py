import socket
import sys
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
print("Created socket")

addr = "localhost"
port = 40000

if len(sys.argv) > 1:
    port = int(sys.argv[1])

s.bind((addr, port))
print(f"Socket binded to {port}")

s.listen(5)
print("Socket is listening")
i = 0
while True:
    connection, addr = s.accept()
    print("Got connection from", addr)

    connection.send(f"Succesful connection ({i}) to {addr}:{port}".encode())
    i+=1

    received_data = connection.recv(1024).decode()

    print(f"\t{received_data=}")
    connection.send(received_data.encode())
    print("\tEchoing received_data back to client")
    connection.close()

print("Closing server")
