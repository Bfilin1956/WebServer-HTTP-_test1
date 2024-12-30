#include <boost/asio.hpp>
#include <filesystem>
#include <thread>
#include <iostream>
#include <sstream>
#include <vector>
#include <fstream>
#include <ctime>
#include <unordered_map>

using namespace boost::asio;
using namespace boost::asio::ip;
namespace fs = std::filesystem;

struct FileInfo {
    std::string name;
    std::string path;
};

std::vector<FileInfo> filesList;

void scanDirectory(const std::string& rootPath) {
    if (!fs::exists(rootPath) || !fs::is_directory(rootPath)) {
        std::cerr << "Error: Directory " << rootPath << " does not exist or is not a directory." << std::endl;
        return;
    }

    for (const auto& entry : fs::recursive_directory_iterator(rootPath)) {
        if (entry.is_regular_file()) {
            filesList.push_back({ entry.path().filename().string(), entry.path().string() });
        }
    }
}

std::string findFileInList(const std::string& requestedPath) {
    for (const auto& file : filesList) {
        if (requestedPath == "/" + file.name) {
            return file.path;
        }
    }
    return "";
}

void log_request(const std::string& client_ip, const std::string& request) {
    std::istringstream request_stream(request);
    std::string method, path, protocol;
    request_stream >> method >> path >> protocol;

    std::time_t now = std::time(nullptr);
    struct tm time_info;
    char time_buffer[100];

    if (localtime_s(&time_info, &now) == 0) {
        std::strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", &time_info);

        std::ofstream log("server.log", std::ios::app);
        if (log.is_open()) {
            log << "[" << time_buffer << "] " << client_ip << " " << path << " " << method << "\n";
        }
    }
}

std::string get_content_type(const std::string& path) {
    static const std::unordered_map<std::string, std::string> mime_types = {
        {".html", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".json", "application/json"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".png", "image/png"},
        {".gif", "image/gif"},
        {".txt", "text/plain"},
        {".pdf", "application/pdf"},
        {".xml", "application/xml"},
    };

    for (const auto& [ext, mime] : mime_types) {
        if (path.size() >= ext.size() && path.substr(path.size() - ext.size()) == ext) {
            return mime;
        }
    }

    return "application/octet-stream";
}

std::string create_response(const std::string& request) {
    std::istringstream request_stream(request);
    std::string method, path, protocol;

    request_stream >> method >> path >> protocol;

    if (path.find("/WWWROOT") == 0) {
        path = path.substr(8);
    }

    if (method == "GET") {
        std::string filePath = findFileInList(path);
        if (!filePath.empty()) {
            std::ifstream file(filePath, std::ios::binary);
            if (file.is_open()) {
                std::ostringstream content;
                char buffer[8192];
                while (file.read(buffer, sizeof(buffer))) {
                    content.write(buffer, file.gcount());
                }
                content.write(buffer, file.gcount());

                return "HTTP/1.1 200 OK\r\n"
                    "Content-Type: " + get_content_type(path) + "\r\n"
                    "Content-Length: " + std::to_string(content.str().size()) + "\r\n\r\n" +
                    content.str();
            }
        }
        return "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nFile not found.";
    }
    else if (method == "POST" && path == "/api/echo") {
        std::string body;
        getline(request_stream, body, '\0');
        return "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n" + body;
    }

    return "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nRoute not found.";
}

void handle_client(tcp::socket socket) {
    auto read_buffer = std::make_shared<std::array<char, 1024>>();
    auto self = std::make_shared<tcp::socket>(std::move(socket));
    std::string client_ip = self->remote_endpoint().address().to_string();

    self->async_read_some(boost::asio::buffer(*read_buffer),
        [read_buffer, self, client_ip](const boost::system::error_code& ec, std::size_t bytes_transferred) {
            if (!ec) {
                std::string request(read_buffer->data(), bytes_transferred);

                log_request(client_ip, request);

                try {
                    std::string response = create_response(request);
                    boost::asio::async_write(*self, boost::asio::buffer(response),
                        [self](const boost::system::error_code& ec, std::size_t) {
                            if (ec) {
                                std::cerr << "Write error: " << ec.message() << std::endl;
                            }
                        });
                }
                catch (const std::exception& ex) {
                    std::cerr << "Exception: " << ex.what() << std::endl;
                }
            }
        });
}

void start_accepting(tcp::acceptor& acceptor) {
    acceptor.async_accept([&acceptor](const boost::system::error_code& ec, tcp::socket socket) {
        if (!ec) {
            handle_client(std::move(socket));
        }
        start_accepting(acceptor);
        });
}

int main() {
    setlocale(LC_ALL, "ru");
    scanDirectory("WWWROOT");

    try {
        io_context io;
        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 8080));

        start_accepting(acceptor);

        const int num_threads = std::thread::hardware_concurrency();
        std::vector<std::thread> threads;
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&io]() { io.run(); });
        }

        for (auto& t : threads) {
            t.join();
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }

    return 0;
}
