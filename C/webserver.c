/*
 * A simple Web server implementation in C
 * Author: Kota Abe
 */

// Linux (CentOS7) とMacOS (10.14.6) で動作を確認済
// C99規格で書いているので，コンパイルには "cc -std=c99 webserver.c" とする

#define _BSD_SOURCE // Linux requires for strdup()
#define _POSIX_C_SOURCE 200112L // for fdopen()

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h> // socket()
#include <arpa/inet.h> // inet_ntop()
#include <unistd.h> // close()

// プロトタイプ宣言
void handle_get(FILE *fp, const char *path);
void handle_client(int client_fd, struct sockaddr_in *client_addr);

int main(int argc, char **argv)
{
    // socket()はソケットを生成し，ファイル記述子(ソケットに割り当てられた整数値)を返す
    // - AF_INETはIPv4
    // - SOCK_STREAMは信頼性の高い全2重バイトストリームの指定
    // - IPPROTO_TCPはTCPの指定
    int server_fd;
    if ((server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        // socket()はエラーのとき負の値を返す
        perror("socket");
        exit(1);
    }

    // プログラム再実行時に，サーバのポートをすぐに再利用できるようにする
    int yes = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    // bind()で自ノード側のアドレスを設定
    struct sockaddr_in server_addr; // sockaddr_inはIPアドレスとポート番号を保持する構造体
    memset(&server_addr, 0, sizeof(server_addr)); // 構造体をいったんゼロクリア
    server_addr.sin_family = AF_INET; // IPv4アドレス
    // サーバ側のIPアドレスはANY(指定しない)．htonlは4バイトをネットワークバイトオーダに変換
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(8000); // サーバ側のポート番号の指定．htonsは2バイトをネットワークバイトオーダに変換
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    // ソケットをサーバ用に設定．5はコネクション要求を貯めておくキューの長さ
    // - コネクション要求はaccept()で受け取る必要があるが，それをどのくらい溜めておけるか
    if (listen(server_fd, 5) < 0) {
        perror("listen");
        exit(1);
    }

    printf("open http://localhost:8000/ with your browser!\n");

    // クライアントからのコネクションを処理するためのループ
    while (1) {
        // accept()はクライアントとのコネクションが確立するのを待ってブロックする（待つ）．
        // 確立すると当該コネクションで通信するためのファイル記述子を返す．
        // - client_addrはクライアントのIPアドレスとポート番号を受け取るための構造体
        int client_fd;
        struct sockaddr_in client_addr;
        socklen_t addr_length = sizeof client_addr;
        if ((client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_length)) < 0) {
            perror("accept");
            exit(1);
        }
        handle_client(client_fd, &client_addr); // クライアントとのやりとりを行う
    }
}

/*
 * クライアントとHTTP/1.0で通信する
 */
void handle_client(int client_fd, struct sockaddr_in *client_addr)
{
    // inet_ntop()でクライアントのIPアドレスを "1.2.3.4" 形式に変換
    // - IPアドレスはsockaddr_in構造体のsin_addrメンバに入っている
    char ipstr[128];
    if (inet_ntop(AF_INET, &client_addr->sin_addr, ipstr, sizeof ipstr) == NULL) {
        perror("inet_ntop");
        return;
    }
    printf("Connection from %s has been established!\n", ipstr);

    // ファイル記述子を直接扱うとバイト列で読み書きすることしかできない
    // 行単位で読み書きするために，fdopen()を使ってfgetsやfprintfを使えるようにする
    //   "r+" は読み込みも書き込みもできるようにする指定
    FILE *fp = fdopen(client_fd, "r+");
    if (fp == NULL) {
        perror("fdopen");
        return;
    }

    // headersはクライアントが送信するHTTPヘッダを覚えておくための配列．固定長なのはわかりやすさ優先
    //   そもそも1行目しか使ってないので不要...
    const int MAX_HEADER_LINES = 256;
    char *headers[MAX_HEADER_LINES];
    int header_index = 0;
    // HTTPヘッダを読むための繰り返し
    while (1) {
        if (header_index >= MAX_HEADER_LINES) {
            puts("too many HTTP headers!");
            goto bailout;
        }
        // 1行読み込み
        char line[1024];
        if (fgets(line, sizeof line, fp) == NULL) {
            puts("connection closed!");
            goto bailout;
        }
        // fgetsは改行を削除しないので，行中の最初の\rか\nを\0に書き換えて切り詰める
        char *eol = strpbrk(line, "\r\n");
        if (eol) {
            *eol = '\0';
        }
        printf("Received: %s\n", line);
        // headersに代入．strdup()は文字列をmallocした領域にコピーする関数
        headers[header_index++] = strdup(line);

        // 空行判定
        if (strcmp(line, "") == 0) {
            break;
        }
    }

    // 最初の行は "GET / HTTP/1.0" のような形式になっている．これを取り出す．
    char *req_line = headers[0];
    char *req[] = { NULL, NULL, NULL }; // method, path, versionの3つの要素を入れる配列
    // strtok()で空白で分割する
    for (int i = 0; i < 3; i++) {
        char *word = strtok(req_line, " "); // " " で区切られた最初のトークンを返す
        req_line = NULL; // strtok()の2回目以降の呼び出しでは第1引数をNULLにする約束
        req[i] = word;
        if (word == NULL) {
            break;
        }
    }
    // 念のため，3つめの要素があるかを確認
    if (req[2] == NULL) {
        printf("wrong format: %s\n", headers[0]);
        goto bailout;
    }
    const char *method = req[0];
    const char *path = req[1];
    const char *http_version = req[2];
    printf("method=%s, path=%s, http_version=%s\n", method, path, http_version);
    // 要求を処理
    if (strcmp(method, "GET") == 0) {
        handle_get(fp, path);
    } else {
        // GET要求以外の場合，クライアントに 501 Not Implemented エラーを返す
        printf("unsupported method: %s\n", method);
        fprintf(fp, "HTTP/1.0 501 Not Implemented\r\n");
    }
    bailout:
    fclose(fp); // fcloseは内部でcloseを呼ぶので，クライアントとのコネクションが切断される
    for (int i = 0; i < header_index; i++) {
        free(headers[i]); // strdup()でmallocした領域を開放
    }
}

/*
 * GET要求を処理する
 */
void handle_get(FILE *fp, const char *path)
{
    if (strcmp(path, "/") == 0) {    // http://localhost:8000/ へのアクセスの場合
        // C言語では連続した文字列は自動的に連結される
        fprintf(fp,
                "HTTP/1.0 200 OK\r\n"         // ステータス行
                "Content-Type: text/html\r\n" // 本体がHTMLであることを伝える
                "\r\n"                        // ヘッダの終了を表す空行
                "<!DOCTYPE html>\r\n"         // 以下はHTML (HTML5)
                "<html>\r\n"
                "<head><title>Sample</title></head>\r\n"
                "<body>This server is implemented in C!</body>\r\n"
                "</html>\r\n"
        );
    } else {    // 他のパス (http://localhost:8000/abc.html) などへのアクセスの場合，404エラー
        fprintf(fp,
                "HTTP/1.0 404 Not Found\r\n"  // 404はNot Foundを表す
                "Content-Type: text/html\r\n"
                "\r\n"
                "<!DOCTYPE html>\r\n"
                "<html>\r\n"
                "<head><title>404 Not Found</title></head>\r\n"
                "<body>%s is not found</body>\r\n"
                "</html>\r\n",
                path
        );
    }
    fflush(fp); // 溜まっているバッファをフラッシュ(送信)
}
