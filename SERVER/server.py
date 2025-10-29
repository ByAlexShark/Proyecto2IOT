# server.py
import socket
import threading

HOST = "0.0.0.0"
PORT = 5050

sensor_conn = None
actuator_conn = None
lock = threading.Lock()

def set_keepalive(sock):
    try:
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
        # Linux
        if hasattr(socket, "TCP_KEEPIDLE"):
            sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPIDLE, 30)   # idle 30s
        if hasattr(socket, "TCP_KEEPINTVL"):
            sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPINTVL, 10)  # cada 10s
        if hasattr(socket, "TCP_KEEPCNT"):
            sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPCNT, 3)     # 3 probes
        # macOS/BSD: TCP_KEEPALIVE (idle)
        if hasattr(socket, "TCP_KEEPALIVE"):
            sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPALIVE, 30)
    except Exception as e:
        print(f"[WARN] keepalive set failed: {e}")

def handle_client(conn, addr):
    global sensor_conn, actuator_conn
    print(f"\033[94m[NEW CONNECTION]\033[0m {addr}")
    set_keepalive(conn)

    buf = ""
    try:
        while True:
            data = conn.recv(1024)
            if not data:
                break
            buf += data.decode(errors="ignore")
            # procesar por líneas
            while "\n" in buf:
                line, buf = buf.split("\n", 1)
                line = line.strip()
                if not line:
                    continue

                # Identificación
                if "SENSOR" in line:
                    with lock: sensor_conn = conn
                    print("\033[92m[INFO]\033[0m Sensor conectado.")
                    continue
                if "ACTUATOR" in line:
                    with lock: actuator_conn = conn
                    print("\033[92m[INFO]\033[0m Actuador conectado.")
                    continue

                # Heartbeats
                if line == "PING":
                    # Opcional: responder algo
                    # conn.sendall(b"PONG\n")
                    continue

                # Datos de distancia
                if line.startswith("DIST:"):
                    try:
                        distance = float(line.split(":", 1)[1])
                    except ValueError:
                        continue

                    print(f"\033[96m[DATA]\033[0m Distancia = {distance:.2f} cm")

                    cmd = b"OFF\n"
                    if distance < 5:   cmd = b"LED1\n"
                    elif distance < 15: cmd = b"LED2\n"
                    elif distance < 30: cmd = b"LED3\n"

                    with lock:
                        if actuator_conn:
                            try:
                                actuator_conn.sendall(cmd)
                                print(f"\033[93m[CMD]\033[0m {cmd.strip().decode()} -> Actuador")
                            except Exception as e:
                                print(f"\033[91m[WARN]\033[0m send failed: {e}")
                else:
                    # Otros mensajes informativos
                    print(f"[INFO] {addr}: {line}")

    except Exception as e:
        print(f"\033[91m[ERROR]\033[0m {addr}: {e}")
    finally:
        conn.close()
        with lock:
            if conn is sensor_conn:   sensor_conn = None
            if conn is actuator_conn: actuator_conn = None
        print(f"\033[91m[DISCONNECTED]\033[0m {addr}")

def start_server():
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    set_keepalive(srv)
    srv.bind((HOST, PORT))
    srv.listen(4)
    print(f"\033[94m[LISTENING]\033[0m Servidor escuchando en {HOST}:{PORT}\n")

    try:
        while True:
            conn, addr = srv.accept()
            t = threading.Thread(target=handle_client, args=(conn, addr), daemon=True)
            t.start()
    except KeyboardInterrupt:
        print("\n[EXIT] Bye")
    finally:
        srv.close()

if __name__ == "__main__":
    start_server()
