import socket
import threading

#Note that for running this you have to first comment the line that prevents logged users to log again
#In the server code. File: src/server/pop3/pop3.c in function: log_user

HOST = "svip"  # Ip address of server
PORT = 1080  # Server port
USER = "user"  # Usuer for auth
PASS = "pass"  # Password


def pop3_client_simulation(client_id):
    try:
        with socket.create_connection((HOST, PORT)) as sock:
            print(f"[{client_id}] Conectado al servidor\n")

            # Welcome message
            response = sock.recv(1024).decode()
            print(f"[{client_id}] {response.strip()}\n")

            for _ in iter(int, 1):
                pass

    except Exception as e:
        print(f"[{client_id}] Error: {e}")


# Create threads for multiple users
def simulate_multiple_users(num_users):
    threads = []
    for i in range(num_users):
        thread = threading.Thread(target=pop3_client_simulation, args=(i,))
        threads.append(thread)
        thread.start()

    # Wait till all threads are done
    for thread in threads:
        thread.join()


if __name__ == "__main__":
    num_users = 100
    simulate_multiple_users(num_users)
