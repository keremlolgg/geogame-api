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
    // Dosyanýn mevcut ve boþ olup olmadýðýný kontrol et
    std::ifstream dosya(dosya_yolu, std::ios::binary | std::ios::ate);
    if (!dosya.is_open() || dosya.tellg() == 0) {
        std::cerr << "Dosya mevcut deðil veya boyutu 0 bayt: " << dosya_yolu << std::endl;
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

        // Webhook ayarlarý ve dosya gönderimi
        curl_easy_setopt(curl, CURLOPT_URL, webhook_url.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        // Form ayarlarý
        curl_mime_name(field, "file");
        curl_mime_filedata(field, dosya_yolu.c_str());  // Dosyayý yolla

        curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

        // Yanýtý iþlemek için callback ayarlarý
        Response response;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        // Dosya gönderme iþlemi
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "Dosya gönderme hatasý: " << curl_easy_strerror(res) << std::endl;
        }
        else {
            std::cout << "Dosya baþarýyla gönderildi!" << std::endl;

            // Dosya URL'sini bulma
            indirmeurl = url_bul(response.data);
            if (!indirmeurl.empty()) {
                std::cout << "Bulunan URL: " << indirmeurl << std::endl;
            }
            else {
                std::cout << "URL bulunamadý." << std::endl;
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
    // JSON dosyasýný açýyoruz
    std::ifstream input_file("users.json");
    nlohmann::json json_data;

    // Eðer dosya boþsa veya yoksa, users array'ini oluþturuyoruz
    if (input_file.good() && input_file.peek() != std::ifstream::traits_type::eof()) {
        input_file >> json_data; // JSON verisini okuyoruz
    }
    else {
        json_data["users"] = nlohmann::json::array(); // users array'ini oluþturuyoruz
    }

    bool user_found = false;

    // Tablodaki her kullanýcýyý kontrol ediyoruz
    for (auto& user : json_data["users"]) {
        if (user["name"] == name) {
            // Kullanýcý bulundu, puanlarý karþýlaþtýrýyoruz
            try {
                // string'i integer'a çevirmeyi deneyelim
                int existing_puan = std::stoi(user["puan"].get<std::string>());
                int new_puan = std::stoi(puan);

                if (existing_puan < new_puan) {
                    // Yeni puan daha büyükse, puaný güncelliyoruz
                    user["puan"] = puan;
                    std::cout << "Puan güncellendi: " << name << " - " << puan << std::endl;
                }
            }
            catch (const std::exception& e) {
                std::cout << "Puan dönüþümü hatasý: " << e.what() << std::endl;
            }

            user_found = true;
            break;
        }
    }

    // Eðer kullanýcý bulunmadýysa, yeni kullanýcýyý ekliyoruz
    if (!user_found) {
        nlohmann::json new_user = {
            {"name", name},
            {"puan", puan}
        };
        json_data["users"].push_back(new_user);  // Yeni kullanýcýyý users array'ine ekliyoruz
        std::cout << "Yeni kullanýcý eklendi: " << name << " - " << puan << std::endl;
    }

    // JSON verisini dosyaya yazýyoruz
    std::ofstream output_file("users.json");
    output_file << json_data.dump(4);
    output_file.close();  // Dosyayý tamamen kapatýyoruz

    // Dosya sisteminin tamamlanmasý için kýsa bir bekleme süresi
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Dosyayý gönderiyoruz
    dosya_gonder(dosyayukleyici, "users.json");
}

void on_message_create(const dpp::message_create_t& event) {
    // Mesaj, log kanalýndan geliyorsa
    if (event.msg.channel_id == logchannelid) {
        // Mesaj içeriðini alýyoruz
        std::string message_content = event.msg.content;

        // Baþýndan 7, sonundan ise 3 karakteri siliyoruz
        if (message_content.length() > 7) {
            message_content.erase(0, 7);  // Baþýndan 7 karakteri sil
        }
        if (message_content.length() > 3) {
            message_content.erase(message_content.length() - 3, 3);  // Sonundan 3 karakteri sil
        }

        // Düzenlenmiþ mesajý ekrana yazdýrýyoruz
        //std::cout << "Düzenlenmiþ Mesaj: " << message_content << std::endl;

        // JSON verisini parse ediyoruz
        try {
            nlohmann::json json_data = nlohmann::json::parse(message_content);
            std::string name = json_data["name"];
            std::string toplampuan = json_data["toplampuan"];

            // Verileri ekrana yazdýrýyoruz
            std::cout << "Name: " << name << std::endl;
            std::cout << "Toplampuan: " << toplampuan << std::endl;
            save_user_data(name, toplampuan);
            std::string response = discordWebhookEdit(leadboardurl, "1304934491709771890", indirmeurl, true);
            std::cout << "Response: " << response << std::endl;
        }
        catch (const nlohmann::json::parse_error& e) {
            std::cerr << "JSON parse hatasý: " << e.what() << std::endl;
        }
    }
}


int main() {
    setlocale(LC_ALL, "Turkish");

    // DPP bot objesini baþlatýyoruz
    bot.on_ready([](const dpp::ready_t& event) {
        bot.set_presence(dpp::presence(dpp::ps_online, dpp::at_game, "GeoGame Backend"));
        });
    // Mesaj geldiðinde tetiklenecek fonksiyonu ayarlýyoruz
    bot.on_message_create(on_message_create);

    // Botu çalýþtýr
    std::thread bot_thread([]() {
        bot.start(dpp::st_wait);
        });
    bot_thread.join();
    return 0;
}
