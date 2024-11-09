#include <iostream>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>
#include <dpp/dpp.h>
#include <CURL/curl/curl.h>
using namespace std;
using namespace dpp;
// Webhook URL'si
const std::string leadboardurl = "-";
const std::string dosyayukleyici = "-";
const string token = "-";
const snowflake logchannelid = 1304542748975829065;
const uint64_t CHANNEL_ID = 123456789012345678;
dpp::cluster bot(token, dpp::i_default_intents | dpp::i_message_content);
std::string indirmeurl;

struct Response {
    std::string data;
};
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((Response*)userp)->data.append((char*)contents, size * nmemb);
    return size * nmemb;
}
std::string url_bul(const std::string& response_data) {
    size_t start = response_data.find("https://");
    if (start == std::string::npos) return "";
    size_t end = response_data.find("\"", start);
    return response_data.substr(start, end - start);
}
void dosya_gonder(const std::string& webhook_url, const std::string& dosya_yolu) {
    // Dosyan�n mevcut ve bo� olup olmad���n� kontrol et
    std::ifstream dosya(dosya_yolu, std::ios::binary | std::ios::ate);
    if (!dosya.is_open() || dosya.tellg() == 0) {
        std::cerr << "Dosya mevcut de�il veya boyutu 0 bayt: " << dosya_yolu << std::endl;
        return;
    }
    dosya.close();

    CURL* curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        curl_mime* form = curl_mime_init(curl);
        curl_mimepart* field = curl_mime_addpart(form);

        // Webhook ayarlar� ve dosya g�nderimi
        curl_easy_setopt(curl, CURLOPT_URL, webhook_url.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        // Form ayarlar�
        curl_mime_name(field, "file");
        curl_mime_filedata(field, dosya_yolu.c_str());  // Dosyay� yolla

        curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

        // Yan�t� i�lemek i�in callback ayarlar�
        Response response;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        // Dosya g�nderme i�lemi
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "Dosya g�nderme hatas�: " << curl_easy_strerror(res) << std::endl;
        }
        else {
            std::cout << "Dosya ba�ar�yla g�nderildi!" << std::endl;

            // Dosya URL'sini bulma
            indirmeurl = url_bul(response.data);
            if (!indirmeurl.empty()) {
                std::cout << "Bulunan URL: " << indirmeurl << std::endl;
            }
            else {
                std::cout << "URL bulunamad�." << std::endl;
            }
        }

        curl_mime_free(form);
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
}
std::string discordWebhookEdit(const std::string& webhookUrl, const std::string& messageId, const std::string& content, bool tts) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        std::string editUrl = webhookUrl + "/messages/" + messageId;
        std::string boundary = "---------------------------boundary";
        std::string postData = "--" + boundary + "\r\n";
        postData += "Content-Disposition: form-data; name=\"content\"\r\n\r\n";
        postData += content + "\r\n";
        postData += "--" + boundary + "\r\n";
        postData += "Content-Disposition: form-data; name=\"tts\"\r\n\r\n";
        postData += (tts ? "true\r\n" : "false\r\n");
        postData += "--" + boundary + "--\r\n";

        curl_easy_setopt(curl, CURLOPT_URL, editUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, (struct curl_slist*)curl_slist_append(NULL, ("Content-Type: multipart/form-data; boundary=" + boundary).c_str()));
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "cURL error: " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

    return readBuffer;
}

void save_user_data(const std::string& name, const std::string& puan) {
    // JSON dosyas�n� a��yoruz
    std::ifstream input_file("users.json");
    nlohmann::json json_data;

    // E�er dosya bo�sa veya yoksa, users array'ini olu�turuyoruz
    if (input_file.good() && input_file.peek() != std::ifstream::traits_type::eof()) {
        input_file >> json_data; // JSON verisini okuyoruz
    }
    else {
        json_data["users"] = nlohmann::json::array(); // users array'ini olu�turuyoruz
    }

    bool user_found = false;

    // Tablodaki her kullan�c�y� kontrol ediyoruz
    for (auto& user : json_data["users"]) {
        if (user["name"] == name) {
            // Kullan�c� bulundu, puanlar� kar��la�t�r�yoruz
            try {
                // string'i integer'a �evirmeyi deneyelim
                int existing_puan = std::stoi(user["puan"].get<std::string>());
                int new_puan = std::stoi(puan);

                if (existing_puan < new_puan) {
                    // Yeni puan daha b�y�kse, puan� g�ncelliyoruz
                    user["puan"] = puan;
                    std::cout << "Puan g�ncellendi: " << name << " - " << puan << std::endl;
                }
            }
            catch (const std::exception& e) {
                std::cout << "Puan d�n���m� hatas�: " << e.what() << std::endl;
            }

            user_found = true;
            break;
        }
    }

    // E�er kullan�c� bulunmad�ysa, yeni kullan�c�y� ekliyoruz
    if (!user_found) {
        nlohmann::json new_user = {
            {"name", name},
            {"puan", puan}
        };
        json_data["users"].push_back(new_user);  // Yeni kullan�c�y� users array'ine ekliyoruz
        std::cout << "Yeni kullan�c� eklendi: " << name << " - " << puan << std::endl;
    }

    // JSON verisini dosyaya yaz�yoruz
    std::ofstream output_file("users.json");
    output_file << json_data.dump(4);
    output_file.close();  // Dosyay� tamamen kapat�yoruz

    // Dosya sisteminin tamamlanmas� i�in k�sa bir bekleme s�resi
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Dosyay� g�nderiyoruz
    dosya_gonder(dosyayukleyici, "users.json");
}

void on_message_create(const dpp::message_create_t& event) {
    // Mesaj, log kanal�ndan geliyorsa
    if (event.msg.channel_id == logchannelid) {
        // Mesaj i�eri�ini al�yoruz
        std::string message_content = event.msg.content;

        // Ba��ndan 7, sonundan ise 3 karakteri siliyoruz
        if (message_content.length() > 7) {
            message_content.erase(0, 7);  // Ba��ndan 7 karakteri sil
        }
        if (message_content.length() > 3) {
            message_content.erase(message_content.length() - 3, 3);  // Sonundan 3 karakteri sil
        }

        // D�zenlenmi� mesaj� ekrana yazd�r�yoruz
        //std::cout << "D�zenlenmi� Mesaj: " << message_content << std::endl;

        // JSON verisini parse ediyoruz
        try {
            nlohmann::json json_data = nlohmann::json::parse(message_content);
            std::string name = json_data["name"];
            std::string toplampuan = json_data["toplampuan"];

            // Verileri ekrana yazd�r�yoruz
            std::cout << "Name: " << name << std::endl;
            std::cout << "Toplampuan: " << toplampuan << std::endl;
            save_user_data(name, toplampuan);
            std::string response = discordWebhookEdit(leadboardurl, "1304934491709771890", indirmeurl, true);
            std::cout << "Response: " << response << std::endl;
        }
        catch (const nlohmann::json::parse_error& e) {
            std::cerr << "JSON parse hatas�: " << e.what() << std::endl;
        }
    }
}


int main() {
    setlocale(LC_ALL, "Turkish");

    // DPP bot objesini ba�lat�yoruz
    bot.on_ready([](const dpp::ready_t& event) {
        bot.set_presence(dpp::presence(dpp::ps_online, dpp::at_game, "GeoGame Backend"));
        });
    // Mesaj geldi�inde tetiklenecek fonksiyonu ayarl�yoruz
    bot.on_message_create(on_message_create);

    // Botu �al��t�r
    std::thread bot_thread([]() {
        bot.start(dpp::st_wait);
        });
    bot_thread.join();
    return 0;
}
