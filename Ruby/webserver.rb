# coding: utf-8
#
# A simple Web server implementation in Ruby
# Requires: Ruby 2.6 or later
#
# Usage:
#   ruby webserver.rb
#
# Author: Kota Abe
#

require "socket"

# GET要求を処理する
def handle_get(sock, path)
  if path == "/"
    # <<はヒアドキュメント．EOSが現れるまでを1つの文字列にする．
    sock.write <<EOS
HTTP/1.0 200 OK\r
Content-Type: text/html\r
\r
<!DOCTYPE html>
<html>
  <head><title>Sample</title></head>
  <body>This server is implemented with Ruby!</body>
</html>
EOS
  else
    sock.write <<EOS
HTTP/1.0 404 Not Found\r
Content-Type: text/html\r
\r
<!DOCTYPE html>
<html>
  <head><title>404 Not Found</title></head> 
  <body>#{path} is not found</body>
</html>
EOS
  end
end

# クライアントとHTTP/1.0で通信する
def handle_client(sock)
  puts("connection established: #{sock.peeraddr}")
  headers = []                     # HTTPヘッダを覚えておく配列
  loop do
    chunk = sock.gets(chomp: true) # 1行読む．chomp:trueは改行を削除する指定
    if chunk == nil
      puts("connection closed!")   # コネクションがクローズされた場合
      return
    end
    puts("Received: #{chunk}")
    headers.push(chunk)
    if chunk == ""                 # 改行かどうかの判定
      req = headers[0].split(" ")  # 先頭行を " " で分割する
      if req.length != 3
        puts("wrong format")
        return
      end
      method, path, http_version = req
      puts("method=#{method}, path=#{path}, http_version=#{http_version}")
      # 要求を処理
      case method
      when "GET"
        handle_get(sock, path)
      else
        puts("unsupported method: #{method}")
        sock.write("HTTP/1.0 501 Not Implemented\r\n")
      end
      break
    end
  end
end

puts("open http://localhost:8000/ with your browser!")
server = TCPServer.new('0.0.0.0', 8000) # サーバソケットの生成
loop do
  client_socket = server.accept         # コネクション確立を待つ
  handle_client(client_socket)
  client_socket.close
end
