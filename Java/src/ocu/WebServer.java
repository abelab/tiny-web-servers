/*
 * A simple Web server implementation with Java
 * Author: Kota Abe
 */
package ocu;

import java.io.*;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.ArrayList;
import java.util.List;

public class WebServer {
    public WebServer() {
        // サーバソケットを作るには，ServerSocketのインスタンスを生成する
        // このtryはtry-with-resource文 (スコープを抜けると自動的にserver.close()が呼ばれる)
        try (ServerSocket server = new ServerSocket()) {
            server.bind(new InetSocketAddress(8000));    // ポート番号の指定
            while (true) {
                // コネクション確立を待ってブロックする．確立するとSocketインスタンスを返す
                try (Socket socket = server.accept()) {
                    handleClient(socket); // 確立したコネクションの処理
                }
            }
        } catch (IOException e) {
            System.out.println(e);
        }
    }

    /**
     * クライアントとHTTP/1.0で通信する．
     * @param socket クライアントと接続しているSocket
     */
    private void handleClient(Socket socket) {
        try (
            // socketに対して行単位で入出力できるようにBufferedReaderとPrintWriterを使う
            BufferedReader reader = new BufferedReader(new InputStreamReader(socket.getInputStream()));
            PrintWriter writer = new PrintWriter(socket.getOutputStream(), true);) {
            // HTTPヘッダを覚えておくためのList
            List<String> headers = new ArrayList<>();
            while (true) {
                String s = reader.readLine(); // ソケットから1行ずつ読む
                if (s == null) { // クライアントからコネクションがクローズされた場合
                    System.out.println("connection closed!");
                    return;
                }
                System.out.println("Received: " + s);
                headers.add(s);
                if (s.equals("")) { // 空行の判定
                    String[] req = headers.get(0).split(" "); // HTTPヘッダの先頭行を空白で分解する
                    if (req.length != 3) {
                        System.out.println("wrong format");
                        return;
                    }
                    String method = req[0];
                    String path = req[1];
                    String httpVersion = req[2];
                    System.out.printf("method=%s, path=%s, http_version=%s%n", method, path, httpVersion);
                    // 要求を処理
                    switch (method) {
                        case "GET":
                            handleGet(reader, writer, path);
                            break;
                        default:
                            System.out.printf("unsupported method %s%n", method);
                            writer.print("HTTP/1.0 501 Not Implemented\r\n");
                            break;
                    }
                    break;
                }
            }
        } catch (IOException e) { // 入出力エラーがあるとIOException例外が発生する
            System.out.println(e);
        }
    }

    /**
     * GET要求を処理する
     */
    private void handleGet(BufferedReader reader, PrintWriter writer, String path) {
        if (path.equals("/")) {
            writer.print(String.join("\r\n",
                    "HTTP/1.0 200 OK",
                    "Content-Type: text/html",
                    "",
                    "<!DOCTYPE html>",
                    "<html>",
                    "<head><title>Sample</title></head>",
                    "<body>This server is implemented with Java!</body>",
                    "</html>",
                    ""));
        } else {
            writer.printf(String.join("\r\n",
                    "HTTP/1.0 404 Not Found",
                    "Content-Type: text/html",
                    "",
                    "<!DOCTYPE html>",
                    "<html>",
                    "<head><title>404 Not Found</title></head>",
                    "<body>%s is not found</body>",
                    "</html>",
                    ""
            ), path);
        }
        writer.flush(); // なくても良い
    }

    public static void main(String[] args) {
        System.out.println("open http://localhost:8000/ with your browser!");
        new WebServer();
    }
}
