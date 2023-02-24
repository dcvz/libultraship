#include "JsonConfig.h"
#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>
#include <Utils/StringHelper.h>

namespace fs = std::filesystem;

JsonConfig::JsonConfig(std::string path) : mPath(path) {
    // check if path exists and that it's a file
    if (!fs::exists(path) || !fs::is_regular_file(path)) {
        mIsNewConfig = true;
        mJson = json::object();
        return;
    }

    try {
        std::ifstream ifs(path);
        mJson = json::parse(ifs);
    } catch (json::parse_error& e) {
        SPDLOG_ERROR("Failed to parse JSON file: {}", e.what());

        // If failure to parse, we will create a new file
        mIsNewConfig = true;
        mJson = json::object();
    }
}

bool IsKeyArrayIndex(std::string key) {
    return std::all_of(key.cbegin(), key.cend(), ::isdigit);
}

int32_t ConvertArrayIndexToInt(std::string key) {
    try {
        int32_t num = stoi(key, nullptr, 10);
        return num >= 0 ? num : -1;
    } catch (std::invalid_argument) {
        return -1;
    } catch (std::out_of_range) {
        return -1;
    }
}

json* AccessNestedValue(std::string key, json* currentJson) {
    if (IsKeyArrayIndex(key)) {
        return &(*currentJson)[ConvertArrayIndexToInt(key)];
    } else {
        return &(*currentJson)[key];
    }
}

bool ArrayOrObjectContainsKey(std::string key, json* currentJson) {
    if (currentJson->is_array() && IsKeyArrayIndex(key)) {
        auto idx = ConvertArrayIndexToInt(key);
        return currentJson->size() >= idx - 1 && !currentJson->at(idx).is_null();
    } else {
        return currentJson->contains(key);
    }
}

void JsonConfig::SetArbitraryType(std::string key, json::value_type value) {
    auto keyParts = StringHelper::Split(key, ".");

    if (keyParts.size() > 1) {
        // nested key
        json* currentJson = &mJson;
        for (size_t i = 0; i < keyParts.size() - 1; i++) {
            if (!ArrayOrObjectContainsKey(keyParts[i], currentJson)) {
                if (IsKeyArrayIndex(keyParts[i + 1])) {
                    // create new array
                    *AccessNestedValue(keyParts[i], currentJson) = json::array();
                } else {
                    // create new object
                    *AccessNestedValue(keyParts[i], currentJson) = json::object();
                }
            }
            currentJson = AccessNestedValue(keyParts[i], currentJson);
        }
        *AccessNestedValue(keyParts[keyParts.size() - 1], currentJson) = value;
    } else {
        // not nested
        mJson[key] = value;
    }
}

json::value_type JsonConfig::GetArbitraryType(std::string key) {
    auto keyParts = StringHelper::Split(key, ".");

    // find the deepest nested key if it exists
    json* currentJson = &mJson;
    for (size_t i = 0; i < keyParts.size() - 1; i++) {
        if (!ArrayOrObjectContainsKey(keyParts[i], currentJson)) {
            return nullptr;
        }
        currentJson = AccessNestedValue(keyParts[i], currentJson);
    }

    // extract the value if possible
    if (ArrayOrObjectContainsKey(keyParts[keyParts.size() - 1], currentJson)) {
        return *AccessNestedValue(keyParts[keyParts.size() - 1], currentJson);
    } else {
        return nullptr;
    }
}

// MARK: - Public API

bool JsonConfig::IsNewConfig() {
    return mIsNewConfig;
}

void EraseKeyInArrayOrObject(std::string key, json* currentJson) {
    if (currentJson->is_array()) {
        (*currentJson).erase(ConvertArrayIndexToInt(key));
    } else {
        (*currentJson).erase(key);
    }
}

void JsonConfig::DeleteEntry(std::string key) {
    auto keyParts = StringHelper::Split(key, ".");

    // clear nested key, if after deletion parent is empty, delete it too
    if (keyParts.size() > 1) {
        json* currentJson = &mJson;
        for (size_t i = 0; i < keyParts.size() - 1; i++) {
            if (!ArrayOrObjectContainsKey(keyParts[i], currentJson)) {
                return;
            }
            currentJson = AccessNestedValue(keyParts[i], currentJson);
        }
        EraseKeyInArrayOrObject(keyParts[keyParts.size() - 1], currentJson);

        // delete empty parents
        for (size_t i = keyParts.size() - 1; i > 0; i--) {
            if ((*currentJson).empty()) {
                currentJson = &mJson;
                for (size_t j = 0; j < i - 1; j++) {
                    currentJson = AccessNestedValue(keyParts[j], currentJson);
                }
                EraseKeyInArrayOrObject(keyParts[i - 1], currentJson);
            }
        }
    } else {
        // not nested
        mJson.erase(key);
    }
}

json::value_type JsonConfig::GetRawEntry(std::string key) {
    return GetArbitraryType(key);
}

void JsonConfig::SetInteger(std::string key, int32_t value) {
    return SetArbitraryType(key, value);
}

int32_t JsonConfig::GetInteger(std::string key, int32_t defaultValue) {
    json::value_type value = GetArbitraryType(key);
    if (value.is_number_integer()) {
        return value;
    } else {
        return defaultValue;
    }
}

void JsonConfig::SetUInteger(std::string key, uint32_t value) {
    return SetArbitraryType(key, value);
}

uint32_t JsonConfig::GetUInteger(std::string key, uint32_t defaultValue) {
    json::value_type value = GetArbitraryType(key);
    if (value.is_number_unsigned()) {
        return value;
    } else {
        return defaultValue;
    }
}

void JsonConfig::SetFloat(std::string key, float value) {
    return SetArbitraryType(key, value);
}

float JsonConfig::GetFloat(std::string key, float defaultValue) {
    json::value_type value = GetArbitraryType(key);
    if (value.is_number_float()) {
        return value;
    } else {
        return defaultValue;
    }
}

void JsonConfig::SetString(std::string key, std::string value) {
    return SetArbitraryType(key, value);
}

std::string JsonConfig::GetString(std::string key, std::string defaultValue) {
    json::value_type value = GetArbitraryType(key);
    if (value.is_string() && !value.get<std::string>().empty()) {
        return value;
    } else {
        return defaultValue;
    }
}

void JsonConfig::SetBoolean(std::string key, bool value) {
    return SetArbitraryType(key, value);
}

bool JsonConfig::GetBoolean(std::string key, bool defaultValue) {
    json::value_type value = GetArbitraryType(key);
    if (value.is_boolean()) {
        return value;
    } else {
        return defaultValue;
    }
}

void JsonConfig::PersistToDisk() {
#if defined(__SWITCH__) || defined(__WIIU__)
    FILE* w fopen(mPath.c_str(), "w");
    std::string jsonStr = mJson.dump(4);
    fwrite(jsonStr.c_str(), sizeof(char), jsonStr.length(), w);
    fclose(w);
#else
    std::ofstream file(mPath);
    file << mJson.dump(4);
    file.close();
#endif

    mIsNewConfig = false;
}
