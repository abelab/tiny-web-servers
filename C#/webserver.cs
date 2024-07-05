// A simple Web server implementation in C#
//
// Usage: dotnet run
//
// Author: Kota Abe

using System;
using System.Net.Sockets;
using System.Net;

class WebServer
{
    const int SERVER_PORT = 8888;
    static void Main(string[] args)
    {
        // SERVER_PORTで listen する
        var ep = new IPEndPoint(IPAddress.Any, SERVER_PORT);
        var listener = new TcpListener(ep);
        try
        {
            listener.Start();
        }
        catch (SocketException e)
        {
            Console.WriteLine($"TcpListener.Start() failed: {e.ErrorCode}");
            return;
        }
        Console.WriteLine($"open http://localhost:{SERVER_PORT}/ with your browser!");

        // クライアントからのコネクションを待ち受けるループ
        while (true)
        {
            // TCPコネクションを確立されるまでブロックする
            TcpClient conn = listener.AcceptTcpClient();
            // 確立したら，スレッドを作って処理させる
            Thread t = new Thread(() =>
            {
                using (conn)
                {
                    handleClient(conn);
                }
            });
            t.Start();
        }
    }
    
    /// <summary>
    // handleClient - クライアントとHTTP/1.0で通信する
    /// </summary>
    /// <param name="client">クライアントと接続したTcpClient</param>
    static void handleClient(TcpClient client)
    {
        Console.WriteLine($"Connection from {client.Client.RemoteEndPoint} has been established!");
        var headers = new List<string>(); // ヘッダを覚えておくためのstringのリスト
        using var reader = new StreamReader(client.GetStream());
        using var writer = new StreamWriter(client.GetStream());
        // HTTPヘッダを読むための繰り返し
        while (true)
        {
            var line = reader.ReadLine(); // 1行取り出し
            if (line == null)             // クローズされた場合
            {           
                Console.WriteLine("closed!");
                return;
            }
            line = line.TrimEnd();        // 改行を削除
            if (line == "")               // 空行が来たらbreak
            {        
                break;
            }
            Console.WriteLine($"Received: {line}");
            headers.Add(line);            // headersに読み込んだ行を追加
        }
        if (headers.Count == 0)
        {
            Console.WriteLine("no header!");
            return;
        }
        string[] req = headers[0].Split(' '); // 先頭行を " " で分割する
        if (req.Length != 3)
        {
            Console.WriteLine($"wrong request: {req}");
            return;
        }
        var (method, path, http_version) = (req[0], req[1], req[2]);
        Console.WriteLine($"method={method}, path={path}, http_version={http_version}");
        // 要求を処理
        switch (method)
        {
            case "GET":
                handleGet(writer, path);
                break;
            default:
                Console.WriteLine($"unsupported method: {method}");
                writer.Write("HTTP/1.0 501 Not Implemented\r\n");
                break;
        }
        writer.Flush();
    }

    /// <summary>
    // handleGet - GET要求を処理する
    /// </summary>
    /// <param name="writer">書き込み用のStreamWriter</param>
    /// <param name="path">指定されたパス</param>
    static void handleGet(StreamWriter writer, string path)
    {
        string s;
        if (path == "/")
        {
            s = @"HTTP/1.0 200 OK
Content-Type: text/html

<!DOCTYPE html>
<html>
<head><title>Sample</title></head>
<body>This server is implemented in C#!</body>
</html>
";
        }
        else
        {
            s = $@"HTTP/1.0 404 Not Found
Content-Type: text/html

<!DOCTYPE html>
<html>
<head><title>404 Not Found</title></head>
<body>{path} is not found</body>
</html>
";
        }
        // 改行を LF (\n) から CR+LF (\r\n) に置き換える
        // @""による複数行文字列の改行コードはソースコードの改行コードに依存する．
        // ここでは，ソースコードの改行コードの違いに影響されないように，Splitを使って一度改行で分割し，
        // Join を使って間に \r\n を挿入している．
        s = string.Join("\r\n", s.Split(new string[] { "\r\n", "\n" }, StringSplitOptions.None));
        writer.Write(s);
    }
}