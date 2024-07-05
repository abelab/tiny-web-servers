/*
 * A simple Web server implementation in JavaScript (on Node.js)
 *
 * Usage:
 *   node webserver.js
 *
 * Author: Kota Abe
 */

// "http"モジュールを使うともっと簡単にWebサーバを実装できるが，
// このプログラムでは低レベルの処理を見せるために"net"モジュールを使っている
const net = require("net");

const SERVER_PORT = 8888;

function main() {
    const server = net.createServer(); // サーバの作成
    // JavaScriptでは基本的にイベントベースで処理を記述する
    // - クライアントからのコネクションが確立すると"connection"というイベントが発生する．
    // - そのときに conn => { handleClient(conn); } というアロー関数が実行されるように設定する．
    //    connは確立したコネクション(net.Socketのオブジェクト)
    // https://nodejs.org/api/net.html#net_event_connection
    server.on("connection", conn => {
        handleClient(conn);
    });
    server.listen(SERVER_PORT); // コネクションの待ち受けを開始する
    console.log(`open http://localhost:${SERVER_PORT}/ with your browser!`);
    // ここでmain関数は終わるが，裏でコネクションの確立を待っている
}

/**
 * クライアントとHTTP/1.0で通信する．
 * @param {net.Socket} conn
 */
 function handleClient(conn) {
    console.log(`Connection from ${conn.remoteAddress}:${conn.remotePort} has been established!`)
    let buf = "";  // 読み込んだHTTPヘッダを覚えておくバッファ
    // クライアントからデータが届いた場合の処理を設定する
    // (設定するだけなので，この処理はデータが届く前に終了する)
    conn.on("data", data => {
        // この部分はデータが届いたときに実行される
        console.log("Received: " + data);
        buf = buf + data; // データは複数回に別れて届く可能性があるので，受け取ったデータを連結する
        const i = buf.indexOf("\r\n\r\n"); // 空行があるかどうかを判定
        if (i >= 0) { // 空行がある場合
            const head = buf.slice(0, i); // 先頭から空行までを取り出す   
            const headers = head.split("\r\n"); // 行単位に分割する
            // 先頭行を " " で分割する．分割代入構文を使って配列の要素を3つの変数に代入
            let [ method, path, http_version ] = headers[0].split(" ");
            console.log(`method=${method}, path=${path}, http_version=${http_version}`);
            switch (method) {
                case "GET":
                    handleGet(conn, path);
                    break;
                default:
                    console.log(`unsupported method: ${method}`);
                    conn.write("HTTP/1.0 501 Not Implemented\r\n");
                    break;
            }
            conn.destroy(); // コネクションをクローズ
        }
    });
    // コネクションがクローズされた場合の処理を設定する
    conn.on("end", () => {
        console.log("connection closed");
        conn.destroy();
    });
}

/**
 * GET要求を処理する
 */
 function handleGet(conn, path) {
    var s
    if (path === "/") {
        // backquote(``)では文字列中の改行は\nになる
        s = `HTTP/1.0 200 OK
Content-Type: text/html

<!DOCTYPE html>
<html>
<head><title>Sample</title></head>
<body>This server is implemented in JavaScript!</body>
</html>
`
    } else {
        s = `HTTP/1.0 404 Not Found
Content-Type: text/html

<!DOCTYPE html>
<html>
<head><title>404 Not Found</title></head>
<body>${path} is not found</body>
</html>
`
    }
  	// 改行を LF (\n) から CR+LF (\r\n) に置き換える
    replaced = s.replace("\n", "\r\n")
    conn.write(replaced)
}

main();
