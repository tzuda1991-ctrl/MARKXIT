#include <json/json.hpp>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <random>

#include "include/obfuscate.h"

bool bValid = false;

std::string xor_encrypt(const std::string& data, const std::string& key) {
    std::string result;
    result.reserve(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        result += data[i] ^ key[i % key.length()];
    }
    return result;
}

std::string xor_decrypt(const std::string& data, const std::string& key) {
    return xor_encrypt(data, key);
}

std::string base64_encode(const std::string& data) {
    static const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    size_t i = 0;
    unsigned char a3[3];
    unsigned char a4[4];

    while (data.length() - i >= 3) {
        a3[0] = static_cast<unsigned char>(data[i]);
        a3[1] = static_cast<unsigned char>(data[i + 1]);
        a3[2] = static_cast<unsigned char>(data[i + 2]);

        a4[0] = (a3[0] & 0xfc) >> 2;
        a4[1] = ((a3[0] & 0x03) << 4) + ((a3[1] & 0xf0) >> 4);
        a4[2] = ((a3[1] & 0x0f) << 2) + ((a3[2] & 0xc0) >> 6);
        a4[3] = a3[2] & 0x3f;

        for (int j = 0; j < 4; j++)
            result += chars[a4[j]];

        i += 3;
    }

    if (data.length() - i > 0) {
        int remaining = data.length() - i;
        for (int j = 0; j < 3; j++)
            a3[j] = (j < remaining) ? static_cast<unsigned char>(data[i + j]) : 0;

        a4[0] = (a3[0] & 0xfc) >> 2;
        a4[1] = ((a3[0] & 0x03) << 4) + ((a3[1] & 0xf0) >> 4);
        a4[2] = ((a3[1] & 0x0f) << 2) + ((a3[2] & 0xc0) >> 6);
        a4[3] = a3[2] & 0x3f;

        for (int j = 0; j < remaining + 1; j++)
            result += chars[a4[j]];
        while (result.length() % 4)
            result += '=';
    }

    return result;
}

std::string base64_decode(const std::string& input) {
    static const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++)
        T[static_cast<unsigned char>(chars[i])] = i;

    int val = 0, valb = -8;
    for (unsigned char c : input) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            result.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }

    return result;
}

INLINE std::string getDt(int offsetSeconds = 0) {
    using namespace std::chrono;
    system_clock::time_point now = system_clock::now();
    system_clock::time_point ist = now + hours(3) + seconds(offsetSeconds);
    std::time_t t = system_clock::to_time_t(ist);

    std::tm tm{};
    gmtime_r(&t, &tm);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

INLINE std::string gToken(const std::string& data, const std::string& key) {
    std::string encrypted = xor_encrypt(data, key);
    std::string encoded = base64_encode(encrypted);
    return encoded;
}

INLINE std::string decryptData(const std::string& encryptedData, const std::string& key) {
    try {
        auto jsonObj = nlohmann::json::parse(encryptedData);

        if (!jsonObj.contains("data") || !jsonObj["data"].is_string())
            return "";

        std::string encoded = jsonObj["data"].get<std::string>();
        std::string decoded = base64_decode(encoded);
        return xor_decrypt(decoded, key);
    } catch (...) {
        return "";
    }
}

#include "8BALLMOD_login.h"

std::string ERROR_MESSAGE = "";

static bool logged_in = false;
static bool is_logging_in = false;
std::string g_Token, g_Auth;
std::string g_ExpTime = "N/A";

// Global auth instance pointer to keep it alive for dlclose in destructor, if needed.
GameAuth::Auth* g_GameAuth = nullptr;

INLINE bool Login(std::string androidID, std::string key) {
    if (androidID.empty()) {
        ERROR_MESSAGE = O("Could not get Android ID");
        return false;
    }
    
    if (key.empty()) {
        ERROR_MESSAGE = O("Key Is Empty or Failed to get Key");
        return false;
    }
    
    is_logging_in = true;
    ERROR_MESSAGE = "";

    try {
        if (g_GameAuth) {
            delete g_GameAuth;
            g_GameAuth = nullptr;
        }

        g_GameAuth = new GameAuth::Auth(androidID, OO("8BALLMOD").str());
        GameAuth::AuthResponse res = g_GameAuth->Login(key);

        if (res.success) {
            // Check App Update
            if (res.app_version != OO("1.0").str() && !res.update_url.empty()) {
                // Here we could set an update flag or show a message.
                // For now, if version mismatch, we reject login or show a warning.
                // To force update:
                // ERROR_MESSAGE = O("Your version is old. Please update.");
                // is_logging_in = false;
                // return false;
            }

            // Online LIB Download & Execution
            if (res.lib_status == OO("active").str() && !res.lib_url.empty()) {
                // For Android NDK, /data/data/ + package_name + /files is the standard readable/writable private path.
                // You will need to make sure this path matches your game's package name exactly.
                std::string libPath = OO("/data/data/com.miniclip.eightballpool/files/libmod.so").str();
                
                bool loaded = g_GameAuth->DownloadAndLoadLib(res.lib_url, libPath);
                if (!loaded) {
                    ERROR_MESSAGE = O("Failed to download or load the security module.");
                    is_logging_in = false;
                    return false;
                }
            }

            logged_in = true;
            is_logging_in = false;
            
            // Bypass logic mapping
            g_Token = OO("0wQRlDkgoQlf").str();
            g_Auth  = OO("0wQRlDkgoQlf").str();
            g_ExpTime = "N/A"; // Or parse from message if provided
            bValid = g_Token == g_Auth;
            
            // Map remote statuses to our persistent variables if they exist
            if (res.esp_status == "true") persistent_bool[O("bESP")] = true;
            if (res.aim_status == "true") persistent_bool[O("bAim")] = true;

            persistent_string["key"] = key;
            save_persistence();

            return true;
        } else {
            ERROR_MESSAGE = res.message;
            is_logging_in = false;
            return false;
        }

    } catch (const std::exception& e) {
        ERROR_MESSAGE = OO("Error: ").str() + std::string(e.what());
        is_logging_in = false;
        return false;
    }
}
