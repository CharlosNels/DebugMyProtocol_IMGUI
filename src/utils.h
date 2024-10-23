#ifndef UTILS_H
#define UTILS_H

#include <spdlog/spdlog.h>
#include <stdint.h>
#include <string.h>
#include <unordered_map>
#include <vector>

#define SPDLOG_LOGGER_CALL_(level, ...)                                                                                \
    spdlog::log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, level, __VA_ARGS__)
#define LogTrace(...) SPDLOG_LOGGER_CALL_(spdlog::level::trace, __VA_ARGS__)
#define LogDebug(...) SPDLOG_LOGGER_CALL_(spdlog::level::debug, __VA_ARGS__)
#define LogInfo(...) SPDLOG_LOGGER_CALL_(spdlog::level::info, __VA_ARGS__)
#define LogWarn(...) SPDLOG_LOGGER_CALL_(spdlog::level::warn, __VA_ARGS__)
#define LogError(...) SPDLOG_LOGGER_CALL_(spdlog::level::err, __VA_ARGS__)
#define LogCritical(...) SPDLOG_LOGGER_CALL_(spdlog::level::critical, __VA_ARGS__)

struct ComboBoxData {
    const char *text;
    int index;
    bool item_selected;
    ComboBoxData() : text(nullptr), index(0), item_selected(false) {}
};

void render_combo_box(ComboBoxData &combo_box_data, const char *combo_box_label, const char **items, int start,
                      int end);

uint16_t getBit(uint8_t data, int bit_index);

void setBit(uint8_t &data, int bit_index, uint16_t value);

void setModbusPacketTransID(char *buffer, uint16_t trans_id);

uint16_t CRC_16(const char *data, int len);

uint8_t LRC(const char *data, int len);

int pageConvert(int num, int page);

size_t toHexString(const uint8_t *data, int len, char *buffer, char sep = ' ');

size_t fromHexString(const char *buffer, int len, uint8_t *data);

void getTimeStampString(char *buffer, size_t buffer_size);

template <class T> T myFromLittleEndianByteSwap(const void *src) {
    const size_t size = sizeof(T);
    char const *src_ = (const char *)src;
    T ret{};
    char *dest = (char *)&ret;
    for (int i = 0; i < size; i += 2) {
        dest[i] = src_[i + 1];
        dest[i + 1] = src_[i];
    }
    return ret;
}

template <class T> void myToLittleEndianByteSwap(T val, void *dest) {
    const size_t size = sizeof(T);
    char *src = (char *)&val;
    char *dest_ = (char *)dest;
    for (int i = 0; i < size; i += 2) {
        dest_[i] = src[i + 1];
        dest_[i + 1] = src[i];
    }
}

template <class T> T myFromBigEndianByteSwap(const void *src) {
    T ret{};
    char *dest = (char *)&ret;
    char const *src_ = (const char *)src;
    for (int i = 0; i < sizeof(T); ++i) {
        dest[i] = src_[sizeof(T) - i - 1];
    }
    return ret;
}

template <class T> void myToBigEndianByteSwap(T val, void *dest) {
    char *src = (char *)&val;
    char *dest_ = (char *)dest;
    for (int i = 0; i < sizeof(T); ++i) {
        dest_[i] = src[sizeof(T) - i - 1];
    }
}

template <class T> T myFromLittleEndian(const void *src) {
    T ret{};
    char *dest = (char *)&ret;
    memcpy(dest, src, sizeof(T));
    return ret;
}

template <class T> void myToLittleEndian(T val, void *dest) { memcpy(dest, &val, sizeof(T)); }

template <class T> T myFromBigEndian(const void *src) {
    char const *src_ = (const char *)src;
    const size_t size = sizeof(T);
    T ret{};
    char *dest = (char *)&ret;
    for (int i = 0; i < size; i += 2) {
        dest[i] = src_[size - i - 2];
        dest[i + 1] = src_[size - i - 1];
    }
    return ret;
}

template <class T> void myToBigEndian(T val, void *dest) {
    const size_t size = sizeof(T);
    char *src = (char *)&val;
    char *dest_ = (char *)dest;
    for (int i = 0; i < size; i += 2) {
        dest_[i] = src[size - i - 2];
        dest_[i + 1] = src[size - i - 1];
    }
}

template <class K, class V> std::vector<K> getKeysOfMap(const std::unordered_map<K, V> &map) {
    std::vector<K> keys;
    keys.reserve(map.size());
    for (auto &it : map) {
        keys.push_back(it.first);
    }
    return keys;
}

template <class K, class V> K getKeyOfValueInMap(const std::unordered_map<K, V> &map, const V &value) {
    for (auto &it : map) {
        if (it.second == value) {
            return it.first;
        }
    }
    return K{};
}

#endif
