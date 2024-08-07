// A simple Web server implementation in Go
//
// Usage:
//
//	go run webserver.go
//
// Author: Kota Abe
package main

import (
	"bufio"
	"fmt"
	"net"
	"strings"
)

const SERVER_PORT = 8888

func main() {
	// SERVER_PORTでlistenする
	listener, err := net.Listen("tcp", fmt.Sprintf(":%d", SERVER_PORT))
	if err != nil {
		panic("ListenTCP failed")
	}
	fmt.Printf("open http://localhost:%d/ with your browser!\n", SERVER_PORT)

	// クライアントからのコネクションを待ち受けるループ
	for {
		// TCPコネクションを確立されるまでブロックする
		conn, err := listener.Accept()
		if err != nil {
			panic("AcceptTCP failed")
		}
		// 確立したら，goroutineを作って処理させる (goroutineは並行に動作する)
		go handleClient(conn)
	}
}

// handleClient - クライアントとHTTP/1.0で通信する
func handleClient(conn net.Conn) {
	defer conn.Close() // この関数終了時にクローズするように要求
	fmt.Printf("Connection from %v has been established!\n", conn.RemoteAddr())
	headers := []string{}             // ヘッダを覚えておくためのstringのスライス
	scanner := bufio.NewScanner(conn) // コネクションからの読み取りを行単位に行うためのもの
	// HTTPヘッダを読むための繰り返し
	for scanner.Scan() {
		line := scanner.Text() // 1行取り出し
		if line == "" {        // 空行が来たらbreak
			break
		}
		fmt.Printf("Received: %v\n", line)
		headers = append(headers, line) // headersに読み込んだ行を追加
	}
	if len(headers) == 0 {
		fmt.Println("no header!")
		return
	}
	req := strings.Split(headers[0], " ") // 先頭行を " " で分割する
	if len(req) != 3 {
		fmt.Printf("wrong request: %v\n", req)
		return
	}
	method := req[0]
	path := req[1]
	http_version := req[2]
	fmt.Printf("method=%v, path=%v, http_version=%v\n", method, path, http_version)
	// 要求を処理
	switch method {
	case "GET":
		handleGet(conn, path)
	default:
		fmt.Printf("unsupported method: %v\n", method)
		conn.Write([]byte("HTTP/1.0 501 Not Implemented\r\n"))
	}
}

// handleGet - GET要求を処理する
func handleGet(conn net.Conn, path string) {
	var s string
	if path == "/" {
		s = `HTTP/1.0 200 OK
Content-Type: text/html

<!DOCTYPE html>
<html>
<head><title>Sample</title></head>
<body>This server is implemented in Go!</body>
</html>`
	} else {
		s = fmt.Sprintf(`HTTP/1.0 404 Not Found
Content-Type: text/html

<!DOCTYPE html>
<html>
  <head><title>404 Not Found</title></head>
  <body>%v is not found</body>
</html>`, path)
	}
	// 改行を LF (\n) から CR+LF (\r\n) に置き換える
	s = strings.ReplaceAll(s, "\n", "\r\n")
	conn.Write([]byte(s))
}
