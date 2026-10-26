// ============================================================================
// 🛡️ AUTO-GENERATED C++ AUTH & ONLINE LIB LOADER FOR 8BALL MODE (8BALLMOD)
// ============================================================================
// SERVER URL : https://unfair.mgcheats.store/connect
// GAME CODE  : 8BALLMOD
//
// 📖 IMPLEMENTATION GUIDE (Kahan Kaise Lagaayein):
//
// 1. Dependencies:
//    - Requires libcurl (-lcurl) and dlfcn (-ldl for dlopen/dlsym on Android NDK).
//
// 2. Usage Example in your NDK / Main CPP file:
//
//    #include "8BALLMOD_login.h"
//
//    void PerformLogin() {
//        std::string hwid = "DEVICE_HWID_HERE"; // Get Android Android_ID or Build Serial
//        std::string userKey = "USER_ENTERED_KEY";
//
//        GameAuth::Auth auth(hwid, "8BALLMOD");
//        GameAuth::AuthResponse res = auth.Login(userKey);
//
//        if (res.success) {
//            // Check App Update
//            if (res.app_version != "1.0.0" && !res.update_url.empty()) {
//                // Show update dialog to user using res.update_url & res.update_msg
//            }
//
//            // 🚀 Online LIB Download & Execution (Anti-Crack Protection)
//            if (res.lib_status == "active" && !res.lib_url.empty()) {
//                bool loaded = auth.DownloadAndLoadLib(res.lib_url, "/data/data/com.game.packagename/files/libmod.so");
//                if (!loaded) {
//                    // Lib load failed or server killed the Lib
//                    return;
//                }
//            }
//        } else {
//            // Show error: res.message
//        }
//    }
// ============================================================================

#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <curl/curl.h>
#include <dlfcn.h>

namespace GameAuth {

    struct AuthResponse {
        bool success = false;
        std::string message = "";
        
        // Mod Features
        std::string token = "";
        std::string modname = "";
        std::string esp_status = "";
        std::string aim_status = "";
        std::string silentaim_status = "";
        std::string bullettrack_status = "";
        std::string memory_status = "";
        std::string floating_status = "";

        // Online LIB & App Update Info
        std::string lib_status = "active";
        std::string lib_url = "";
        std::string app_version = "1.0.0";
        std::string update_url = "";
        std::string update_msg = "";
    };

    class Auth {
    private:
        std::string m_BaseUrl = "https://unfair.mgcheats.store/connect"; 
        std::string m_Game = "8BALLMOD";
        std::string m_LicenseKey;
        std::string m_Hwid;
        void* m_LibHandle = nullptr;

        static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
            ((std::string*)userp)->append((char*)contents, size * nmemb);
            return size * nmemb;
        }

        static size_t FileWriteCallback(void* ptr, size_t size, size_t nmemb, FILE* stream) {
            return fwrite(ptr, size, nmemb, stream);
        }

        std::string PostURL(std::string post_data) {
            CURL* curl = curl_easy_init();
            std::string response_string;
            if (curl) {
                curl_easy_setopt(curl, CURLOPT_URL, m_BaseUrl.c_str());
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

                CURLcode res = curl_easy_perform(curl);
                if (res != CURLE_OK) {
                    response_string = "";
                }
                curl_easy_cleanup(curl);
            }
            return response_string;
        }

        std::string get_json_val(std::string json, std::string key) {
            size_t pos = json.find("\"" + key + "\"");
            if (pos == std::string::npos) return "";
            pos = json.find(":", pos + key.length() + 2);
            if (pos == std::string::npos) return "";
            size_t start = json.find_first_not_of(" \t\n\r", pos + 1);
            if (start == std::string::npos) return "";
            if (json[start] == '\"') {
                start++;
                size_t end = json.find('\"', start);
                if (end == std::string::npos) return "";
                return json.substr(start, end - start);
            } else {
                size_t end = json.find_first_of(", \t\n\r}", start);
                if (end == std::string::npos) return json.substr(start);
                return json.substr(start, end - start);
            }
        }

    public:
        Auth(std::string hwid, std::string game = "8BALLMOD") 
            : m_Hwid(hwid), m_Game(game) 
        {
            curl_global_init(CURL_GLOBAL_ALL);
        }

        ~Auth() {
            if (m_LibHandle) {
                dlclose(m_LibHandle);
            }
            curl_global_cleanup();
        }

        AuthResponse Login(std::string license) {
            m_LicenseKey = license;
            std::string payload = "game=" + m_Game + "&user_key=" + license + "&serial=" + m_Hwid;
            std::string response = PostURL(payload);
            
            AuthResponse res;
            if (response.empty()) {
                res.success = false;
                res.message = "Failed to connect to server.";
                return res;
            }

            std::string status_val = get_json_val(response, "status");
            if (status_val == "true") {
                res.success = true;
                res.message = "Login Successful!";
                res.token = get_json_val(response, "token");
                res.modname = get_json_val(response, "modname");
                res.esp_status = get_json_val(response, "ESP");
                res.aim_status = get_json_val(response, "AIM");
                res.silentaim_status = get_json_val(response, "SilentAim");
                res.bullettrack_status = get_json_val(response, "BulletTrack");
                res.memory_status = get_json_val(response, "Memory");
                res.floating_status = get_json_val(response, "Floating");
                
                res.lib_status = get_json_val(response, "lib_status");
                res.lib_url = get_json_val(response, "lib_url");
                res.app_version = get_json_val(response, "app_version");
                res.update_url = get_json_val(response, "update_url");
                res.update_msg = get_json_val(response, "update_msg");
            } else {
                res.success = false;
                res.message = get_json_val(response, "reason");
                if (res.message.empty()) {
                    res.message = "Invalid key, expired, or device mismatch.";
                }
            }
            return res;
        }

        // 🚀 Online LIB Download & Load Function
        bool DownloadAndLoadLib(std::string url, std::string savePath) {
            if (url.empty()) return false;

            FILE* fp = fopen(savePath.c_str(), "wb");
            if (!fp) return false;

            CURL* curl = curl_easy_init();
            if (curl) {
                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, FileWriteCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
                
                CURLcode res = curl_easy_perform(curl);
                curl_easy_cleanup(curl);
                fclose(fp);

                if (res == CURLE_OK) {
                    m_LibHandle = dlopen(savePath.c_str(), RTLD_NOW);
                    return (m_LibHandle != nullptr);
                }
            } else {
                fclose(fp);
            }
            return false;
        }
    };
}
