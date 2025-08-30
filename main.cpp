#include "crow_all.h"
#include <atomic>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <mutex>

struct Event {
    std::string name;
    std::string action;
    int value;
    std::string timestamp;
};

class AtomicCounterServer {
private:
    std::atomic<int> counter;
    std::vector<Event> recentEvents;
    std::mutex eventsMutex;

    std::string getCurrentTimestamp() {
        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    void addEvent(const std::string& name, const std::string& action, int value) {
        std::lock_guard<std::mutex> lock(eventsMutex);
        Event event{name, action, value, getCurrentTimestamp()};
        recentEvents.insert(recentEvents.begin(), event);

        if (recentEvents.size() > 5) {
            recentEvents.pop_back();
        }
    }

    std::string urlEncode(const std::string& value) {
        std::ostringstream escaped;
        escaped.fill('0');
        escaped << std::hex;

        for (char c : value) {
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                escaped << c;
            } else {
                escaped << '%' << std::setw(2) << int((unsigned char)c);
            }
        }

        return escaped.str();
    }

    std::string urlDecode(const std::string& value) {
        std::string result;
        for (size_t i = 0; i < value.size(); ++i) {
            if (value[i] == '%' && i + 2 < value.size()) {
                int hexValue;
                std::istringstream hexStream(value.substr(i + 1, 2));
                if (hexStream >> std::hex >> hexValue) {
                    result += static_cast<char>(hexValue);
                    i += 2;
                } else {
                    result += value[i];
                }
            } else if (value[i] == '+') {
                result += ' ';
            } else {
                result += value[i];
            }
        }
        return result;
    }

    std::string generateCSS() {
        return R"(
            <style>
                * {
                    margin: 0;
                    padding: 0;
                    box-sizing: border-box;
                }

                body {
                    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
                    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
                    min-height: 100vh;
                    padding: 20px;
                    display: flex;
                    flex-direction: column;
                    align-items: center;
                }

                .container {
                    background: white;
                    border-radius: 20px;
                    padding: 30px;
                    box-shadow: 0 20px 40px rgba(0,0,0,0.1);
                    width: 100%;
                    max-width: 400px;
                    text-align: center;
                }

                .counter {
                    font-size: 80px;
                    font-weight: bold;
                    color: #333;
                    margin: 20px 0;
                    text-shadow: 2px 2px 4px rgba(0,0,0,0.1);
                }

                .button {
                    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
                    color: white;
                    border: none;
                    padding: 20px 40px;
                    font-size: 24px;
                    font-weight: bold;
                    border-radius: 50px;
                    cursor: pointer;
                    margin: 20px 0;
                    width: 100%;
                    box-shadow: 0 10px 20px rgba(0,0,0,0.2);
                    transition: transform 0.2s, box-shadow 0.2s;
                }

                .button:hover {
                    transform: translateY(-2px);
                    box-shadow: 0 15px 30px rgba(0,0,0,0.3);
                }

                .button:active {
                    transform: translateY(0);
                }

                .form-group {
                    margin: 15px 0;
                    width: 100%;
                }

                input[type="text"], select {
                    width: 100%;
                    padding: 15px;
                    font-size: 18px;
                    border: 2px solid #ddd;
                    border-radius: 10px;
                    margin: 5px 0;
                }

                input[type="submit"] {
                    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
                    color: white;
                    border: none;
                    padding: 15px;
                    font-size: 18px;
                    font-weight: bold;
                    border-radius: 10px;
                    cursor: pointer;
                    width: 100%;
                    margin-top: 10px;
                }

                .events-table {
                    width: 100%;
                    border-collapse: collapse;
                    margin-top: 20px;
                    font-size: 14px;
                }

                .events-table th, .events-table td {
                    padding: 10px;
                    text-align: left;
                    border-bottom: 1px solid #eee;
                }

                .events-table th {
                    background-color: #f8f9fa;
                    font-weight: bold;
                    color: #666;
                }

                .events-table tr:hover {
                    background-color: #f5f5f5;
                }

                h1 {
                    color: #333;
                    margin-bottom: 20px;
                    font-size: 28px;
                }

                .setup-form {
                    display: flex;
                    flex-direction: column;
                    align-items: center;
                    width: 100%;
                }

                .action-form {
                    margin: 10px 0;
                }
            </style>
        )";
    }

public:
    AtomicCounterServer() : counter(0) {}

    std::string handleGet(const crow::request& req) {
        std::string name, team;

        // Parse cookies
        auto cookie_header = req.get_header_value("Cookie");
        size_t pos = 0;
        while (pos < cookie_header.size()) {
            size_t eq_pos = cookie_header.find('=', pos);
            if (eq_pos == std::string::npos) break;

            size_t semi_pos = cookie_header.find(';', eq_pos);
            if (semi_pos == std::string::npos) semi_pos = cookie_header.size();

            std::string key = cookie_header.substr(pos, eq_pos - pos);
            std::string value = cookie_header.substr(eq_pos + 1, semi_pos - eq_pos - 1);

            // Trim whitespace
            key.erase(0, key.find_first_not_of(" "));
            key.erase(key.find_last_not_of(" ") + 1);

            if (key == "name") name = urlDecode(value);
            else if (key == "team") team = value;

            pos = semi_pos + 1;
        }

        std::stringstream html;
        html << "<!DOCTYPE html><html lang='ru'><head>"
             << "<meta charset='UTF-8'>"
             << "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";

        // Добавляем автообновление только для страницы со счетчиком
        if (!name.empty() && !team.empty()) {
            html << "<meta http-equiv='refresh' content='2'>";
        }

        html << "<title>🫖 Счетчик</title>"
             << generateCSS()
             << "</head><body>"
             << "<div class='container'>";

        if (name.empty() || team.empty()) {
            // Show setup form
            html << "<h1>Добро пожаловать!</h1>"
                 << "<form class='setup-form' method='POST'>"
                 << "<div class='form-group'>"
                 << "<input type='text' name='name' placeholder='Ваше имя' required>"
                 << "</div>"
                 << "<div class='form-group'>"
                 << "<select name='team' required>"
                 << "<option value=''>Выберите команду</option>"
                 << "<option value='plus'>➕ Плюс</option>"
                 << "<option value='minus'>➖ Минус</option>"
                 << "</select>"
                 << "</div>"
                 << "<input type='submit' value='Начать'>"
                 << "</form>";
        } else {
            // Show counter interface
            html << "<h1>Счетчик: " << name << "</h1>"
                 << "<div class='counter'>" << counter.load() << "</div>";

            // Форма для действия через POST
            html << "<form class='action-form' method='POST'>"
                 << "<input type='hidden' name='perform_action' value='true'>";

            if (team == "plus") {
                html << "<button type='submit' class='button'>➕ Увеличить</button>";
            } else {
                html << "<button type='submit' class='button'>➖ Уменьшить</button>";
            }

            html << "</form>";

            // Show recent events
            html << "<h2>Последние события</h2>"
                 << "<table class='events-table'>"
                 << "<tr><th>Имя</th><th>Действие</th><th>Значение</th></tr>"; // <th>Время</th> removed

            std::lock_guard<std::mutex> lock(eventsMutex);
            for (const auto& event : recentEvents) {
                html << "<tr>"
                     << "<td>" << event.name << "</td>"
                     << "<td>" << event.action << "</td>"
                     << "<td>" << event.value << "</td>"
                     // << "<td>" << event.timestamp << "</td>"
                     << "</tr>";
            }

            html << "</table>";
        }

        html << "</div></body></html>";
        return html.str();
    }

    crow::response handlePost(const crow::request& req) {
        std::string body = req.body;
        std::string name, team;
        bool performAction = false;

        // Parse form data
        size_t name_pos = body.find("name=");
        size_t team_pos = body.find("team=");
        size_t action_pos = body.find("perform_action=");

        if (name_pos != std::string::npos) {
            size_t name_end = body.find('&', name_pos);
            if (name_end == std::string::npos) name_end = body.size();
            name = body.substr(name_pos + 5, name_end - name_pos - 5);
            name = urlDecode(name);
        }

        if (team_pos != std::string::npos) {
            size_t team_end = body.find('&', team_pos);
            if (team_end == std::string::npos) team_end = body.size();
            team = body.substr(team_pos + 5, team_end - team_pos - 5);
        }

        if (action_pos != std::string::npos) {
            performAction = true;
        }

        crow::response response;

        if (performAction) {
            // Действие от пользователя - получаем куки и выполняем действие
            auto cookie_header = req.get_header_value("Cookie");
            std::string cookie_name, cookie_team;

            size_t pos = 0;
            while (pos < cookie_header.size()) {
                size_t eq_pos = cookie_header.find('=', pos);
                if (eq_pos == std::string::npos) break;

                size_t semi_pos = cookie_header.find(';', eq_pos);
                if (semi_pos == std::string::npos) semi_pos = cookie_header.size();

                std::string key = cookie_header.substr(pos, eq_pos - pos);
                std::string value = cookie_header.substr(eq_pos + 1, semi_pos - eq_pos - 1);

                key.erase(0, key.find_first_not_of(" "));
                key.erase(key.find_last_not_of(" ") + 1);

                if (key == "name") cookie_name = urlDecode(value);
                else if (key == "team") cookie_team = value;

                pos = semi_pos + 1;
            }

            if (!cookie_name.empty() && !cookie_team.empty()) {
                int new_value;
                if (cookie_team == "plus") {
                    new_value = ++counter;
                    addEvent(cookie_name, "➕", new_value);
                } else if (cookie_team == "minus") {
                    new_value = --counter;
                    addEvent(cookie_name, "➖", new_value);
                }
            }

            response.code = 302;
            response.set_header("Location", "/");

        } else if (!name.empty() && !team.empty()) {
            // Установка имени и команды
            std::string encodedName = urlEncode(name);

            response.code = 302;
            response.set_header("Location", "/");
            response.add_header("Set-Cookie", "name=" + encodedName + "; Path=/; Max-Age=3600");
            response.add_header("Set-Cookie", "team=" + team + "; Path=/; Max-Age=3600");

        } else {
            response.code = 400;
            response.body = "Invalid form data";
        }

        return response;
    }
};

int main() {
    crow::SimpleApp app;
    AtomicCounterServer server;

    CROW_ROUTE(app, "/")
        .methods("GET"_method)
        ([&server](const crow::request& req) {
            return server.handleGet(req);
        });

    CROW_ROUTE(app, "/")
        .methods("POST"_method)
        ([&server](const crow::request& req) {
            return server.handlePost(req);
        });

    std::cout << "Server running on :8080" << std::endl;
    app.port(8080).multithreaded().run();

    return 0;
}
