defmodule WebServer do
  @moduledoc """
  A simple Web server implementation in Elixir

  Usage: elixir ./webserver.exs

  Author: Kota Abe
  """
  def main(_args \\ []) do
    server_port = 8888
    {:ok, socket} = :gen_tcp.listen(server_port,
      [:binary, packet: :line, active: false, reuseaddr: true])
    IO.puts("open http://localhost:#{server_port}/ with your browser!")
    loop_acceptor(socket)
  end

  @doc """
  クライアントからのコネクションを待ち受ける
  """
  def loop_acceptor(socket) do
    # コネクションを待ち受ける
    {:ok, client} = :gen_tcp.accept(socket)
    # 相手のIPアドレスとポート番号を取得
    {:ok, {address, port}} = :inet.peername(client)
    IO.puts("Connection from #{:inet.ntoa(address)}:#{port} has been established!")
    # handle_client関数を軽量プロセスとして実行 (concurrentサーバ)
    spawn(__MODULE__, :handle_client, [client])
    # iterativeサーバの場合は spawn の代わりに以下のように関数呼び出しする
    # handle_client(client)
    loop_acceptor(socket) # 繰り返し(再帰)
  end

  @doc """
  クライアントとHTTP/1.0で通信
  """
  def handle_client(client) do
    # read_headerでヘッダを読み取る
    case read_header(client, []) do
      {:ok, headers} ->
        req_line = Enum.at(headers, 0, "")  # 1行目(リクエスト行)を取り出す
        case String.split(req_line) do      # リクエスト行を空白で分割
          ["GET", path, _http_version] -> handle_get(client, path)
          [method, _path, _http_version] ->
            IO.puts("unsupported method: #{method}")
            :gen_tcp.send(client, "HTTP/1.0 501 Not Implemented\r\n")
          _ ->
            IO.puts("wrong request: #{req_line}")
        end
      _ -> nil
    end
    :gen_tcp.close(client)
    :ok
  end

  @doc """
  クライアントからリクエスト行とHTTPヘッダを読み込む
  """
  def read_header(client, headers) do
    case :gen_tcp.recv(client, 0) do        # 1行読み込む
      {:ok, line} ->                        # 成功した場合
        line = String.trim_trailing(line)   # 末尾の改行を削除
        IO.puts("Received: #{line}")
        if line == "" do
          {:ok, headers}                    # 空行なら終了
        else
          # 空行でなければ読み込みを続ける(再帰)
          read_header(client, headers ++ [line])
        end
      {:error, reason} ->                   # 失敗した場合
        IO.puts("gen_tcp.recv failed (#{reason})")
        {:error, reason}
    end
  end

  @doc """
  GETリクエストを処理
  """
  def handle_get(client, path) do
    s = case path do
      "/" -> """
HTTP/1.0 200 OK
Content-Type: text/html

<!DOCTYPE html>
<html>
  <head><title>Sample</title></head>
  <body>This server is implemented in Elixir!</body>
</html>
"""
      _ -> """
HTTP/1.0 404 Not Found
Content-Type: text/html

<!DOCTYPE html>
<html>
  <head><title>404 Not Found</title></head>
  <body>#{path} is not found</body>
</html>
"""
    end
    # 改行を LF (\n) から CR+LF (\r\n) に置き換える
    replaced = String.replace(s, "\n", "\r\n", global: true)
    :gen_tcp.send(client, replaced)
  end
end

WebServer.main()
