# coding: utf-8
#
# A simple Web server implementation in Python
# Requires: Python 3.7 or later
#
# Usage:
#   python3 webserver.py
#
# Author: Kota Abe
#

import socket
import threading
import io

SERVER_PORT=8888

def main():
    server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_sock.bind(('', SERVER_PORT))
    server_sock.listen(5)
    print(f"open http://localhost:{SERVER_PORT}/ with your browser!")

    while True:
        client_sock, address = server_sock.accept()
        print(f"Connection from {address} has been established!")
        threading.Thread(target=handle_client, args=[client_sock]).start()
        # handle_client(client_sock)

def handle_client(client_sock: socket.socket):
    """ クライアントとHTTP/1.0で通信する """
    # makefile()によってネットワーク通信をファイルのように扱える
    with client_sock, client_sock.makefile(mode="rw") as f:
        headers: list[str] = []                    # HTTPヘッダを覚えておくリスト
        # HTTPヘッダを読むための繰り返し
        while True:
            line = f.readline()         # 1行読む
            if line == "":              # コネクションが切断された場合
                print("connection closed")
                return
            line = line.rstrip()       # 改行文字を削除
            print("Received: " + line)
            headers.append(line)
            if line == "":             # 空行判定
                break

        if len(headers) == 0:
            print("no header!")
            return

        req = headers[0].split(" ")   # 先頭行を " " で分割する
        if len(req) != 3:
            print(f"wrong format: {req}")
            return

        method, path, http_version = req  # 先頭行の要素
        print(f"method={method}, path={path}, http_version={http_version}")
        if method == "GET":
            handle_get(f, path)
        else:
            print(f"unsupported method {method}")
            f.write("HTTP/1.0 501 Not Implemented\r\n")

def handle_get(f: io.TextIOWrapper, path: str):
    """ GET要求を処理する """
    if path == "/":
        s = """\
HTTP/1.0 200 OK
Content-Type: text/html

<!DOCTYPE html>
<html>
<head><title>Sample</title></head>
<body>This server is implemented in Python!</body>
</html>
"""
    else:
        s = f"""\
HTTP/1.0 404 Not Found
Content-Type: text/html

<!DOCTYPE html>
<html>
<head><title>404 Not Found</title></head>
<body>{path} is not found</body>
</html>
"""
    # 改行を LF (\n) から CR+LF (\r\n) に置き換える
    s = s.replace("\n", "\r\n")
    f.write(s)

if __name__ == "__main__":
    main()
