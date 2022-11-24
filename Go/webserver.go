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

// handleGet - GET要求を処理する
func handleGet(conn net.Conn, path string) {
	var s string
	if path == "/" {
		s = `HTTP/1.0 200 OK
Content-Type: text/html

<!DOCTYPE html>
<html>
<head><title>Sample</title></head>
<body>This server is implemented with Go!</body>
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
	replaced := strings.ReplaceAll(s, "\n", "\r\n")
	conn.Write([]byte(replaced))
}

// handleClient - クライアントとHTTP/1.0で通信する
func handleClient(conn net.Conn) {
	defer conn.Close()                // この関数終了時にクローズするように要求
	headers := []string{}             // ヘッダを覚えておくためのstringのスライス
	scanner := bufio.NewScanner(conn) // コネクションからの読み取りを行単位に行うためのもの
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
	req := strings.Split(headers[0], " ") // リクエスト行を空白で分解
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

func main() {
	// 8000番ポートで listen する
	listener, err := net.Listen("tcp", ":8000")
	if err != nil {
		panic("ListenTCP failed")
	}
	fmt.Println("open http://localhost:8000/ with your browser!")

	// クライアントからのコネクションを待ち受けるループ
	for {
		// TCPコネクションを確立されるまでブロックする
		conn, err := listener.Accept()
		if err != nil {
			panic("AcceptTCP failed")
		}
		fmt.Printf("Connection from %v has been established!\n", conn.RemoteAddr())
		// 確立したら，goroutineを作って処理させる (goroutineは並行に動作する)
		go handleClient(conn)
	}
}
