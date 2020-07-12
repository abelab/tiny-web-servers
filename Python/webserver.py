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


# GET要求を処理する
def handle_get(client_sock, path):
    if path == "/":
        client_sock.send("""\
HTTP/1.0 200 OK\r
Content-Type: text/html\r
\r
<!DOCTYPE html>
<html>
<head><title>Sample</title></head>
<body>This server is implemented with Python!</body>
</html>
""".encode())
        # encode()で文字列からUTF-8のバイト列にエンコードする(ここではASCIIと同じ)
    else:
        client_sock.send(f"""\
HTTP/1.0 404 Not Found\r
Content-Type: text/html\r
\r
<!DOCTYPE html>
<html>
<head><title>404 Not Found</title></head>
<body>{path} is not found</body>
</html>
""".encode())


# クライアントとHTTP/1.0で通信する
def handle_client_socket(client_sock):
    buf = ""    # HTTPヘッダを覚えておく文字列
    while True:
        # ソケットから最大1024バイト読み，bytesオブジェクトで返す
        chunk = client_sock.recv(1024)
        if chunk == b"":
            # closed!
            print("connection closed!")
            return
        # asciiに変換し，bufに追加する
        try:
            data = chunk.decode("ascii")
            buf += data
            print("Received: " + data)
        except UnicodeDecodeError:
            print("non-ascii char")
            return

        i = buf.find("\r\n\r\n")          # 空行を探す
        if i >= 0:
            head = buf[0: i]              # 先頭から空行までを取り出す
            headers = head.split("\r\n")  # 行単位に分割する
            req = headers[0].split(" ")   # 先頭行を " " で分割する
            if len(req) != 3:
                print("wrong format")
                return
            method, path, http_version = req  # 配列を分割代入
            print(f"method={method}, path={path}, http_version={http_version}")
            if method == "GET":
                handle_get(client_sock, path)
            else:
                print(f"unsupported method {method}")
                client_sock.send("HTTP/1.0 501 Not Implemented\r\n".encode())
            break


def main():
    server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_sock.bind(('', 8000))
    server_sock.listen(5)
    print("open http://localhost:8000/ with your browser!")

    while True:
        client_sock, address = server_sock.accept()
        with client_sock:
            print(f"Connection from {address} has been established!")
            handle_client_socket(client_sock)


main()
