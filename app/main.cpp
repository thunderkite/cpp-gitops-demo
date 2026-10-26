#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>

#ifndef APP_VERSION
#define APP_VERSION "dev"
#endif

static std::string env_or(const char* name, const char* fallback) {
    const char* v = std::getenv(name);
    return v ? v : fallback;
}

int main() {
    std::signal(SIGPIPE, SIG_IGN);

    const int port = std::stoi(env_or("PORT", "8080"));
    const std::string greeting = env_or("GREETING", "Hello!");

    char host[256] = {};
    gethostname(host, sizeof host - 1);

    int srv = socket(AF_INET, SOCK_STREAM, 0);
    int yes = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(srv, reinterpret_cast<sockaddr*>(&addr), sizeof addr) < 0 ||
        listen(srv, 64) < 0) {
        perror("bind/listen");
        return 1;
    }

    std::cout << "cpp-demo " << APP_VERSION << " listening on :" << port << std::endl;

    for (;;) {
        int c = accept(srv, nullptr, nullptr);
        if (c < 0) continue;

        timeval tv{2, 0};
        setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);

        char buf[2048];
        ssize_t n = read(c, buf, sizeof buf - 1);
        if (n < 0) n = 0;
        std::string req(buf, static_cast<size_t>(n));

        std::string body;
        if (req.rfind("GET /healthz", 0) == 0) {
            body = "ok\n";
        } else {
            body = "Hello from C++ deployed by Argo CD\n"
                   "version : " APP_VERSION "\n"
                   "pod     : " + std::string(host) + "\n"
                   "greeting: " + greeting + "\n";
        }

        std::string resp =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain; charset=utf-8\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "Connection: close\r\n\r\n" + body;

        write(c, resp.data(), resp.size());
        close(c);
    }
}