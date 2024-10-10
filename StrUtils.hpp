#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

namespace xs {
inline std::string& Trim(std::string& _str, std::string strTrim, bool _left = true, bool _right = true) {
    if (_right)
        _str.erase(_str.find_last_not_of(strTrim) + 1);
    if (_left)
        _str.erase(0, _str.find_first_not_of(strTrim));
    return _str;
}

template <typename T>
inline std::string ToString(T p) {
    std::ostringstream stream;
    stream << p;
    return stream.str();
}

template <>
inline std::string ToString(std::string p) {
    return p;
}

inline const std::string& ToString(const std::string& _value) {
    return _value;
}

template <typename T>
inline T Abs(const T& iValue) {
    //return iValue > 0 ? iValue : -iValue;
    return std::abs(iValue);
}

template <typename T1, typename T2>
inline std::string ToString(T1 p1, T2 p2) {
    std::ostringstream stream;
    stream << p1 << p2;
    return stream.str();
}

template <typename T1, typename T2, typename T3>
inline std::string ToString(T1 p1, T2 p2, T3 p3) {
    std::ostringstream stream;
    stream << p1 << p2 << p3;
    return stream.str();
}

template <typename T1, typename T2, typename T3, typename T4>
inline std::string ToString(T1 p1, T2 p2, T3 p3, T4 p4) {
    std::ostringstream stream;
    stream << p1 << p2 << p3 << p4;
    return stream.str();
}

template <>
inline std::string ToString<bool>(bool _value) {
    return _value ? "true" : "false";
}

template <typename T>
inline T ParseValue(const std::string& _value) {
    std::istringstream stream(_value);
    T result;
    stream >> result;
    if (stream.fail()) {
        return T();
    } else {
        int item = stream.get();
        while (item != -1) {
            if (item != ' ' && item != '\t')
                return T();
            item = stream.get();
        }
    }
    return result;
}

template <>
inline bool ParseValue(const std::string& _value) {
    if (_value == "True" || _value == "true" || _value == "1")
        return true;
    return false;
}

template <>
inline char ParseValue(const std::string& _value) {
    return (char)ParseValue<short>(_value);
}

template <>
inline unsigned char ParseValue(const std::string& _value) {
    return (unsigned char)ParseValue<unsigned short>(_value);
}

template <>
inline std::string ParseValue(const std::string& _value) {
    return _value;
}

inline long long ParseLLong(const std::string& _value) {
    return ParseValue<long long>(_value);
}

inline short ParseShort(const std::string& _value) {
    return ParseValue<short>(_value);
}

inline unsigned short ParseUShort(const std::string& _value) {
    return ParseValue<unsigned short>(_value);
}

inline int ParseInt(const std::string& _value) {
    return ParseValue<int>(_value);
}

inline unsigned int ParseUInt(const std::string& _value) {
    return ParseValue<unsigned int>(_value);
}

inline size_t ParseSizeT(const std::string& _value) {
    return ParseValue<size_t>(_value);
}

inline float ParseFloat(const std::string& _value) {
    return ParseValue<float>(_value);
}

inline double ParseDouble(const std::string& _value) {
    return ParseValue<double>(_value);
}

inline bool ParseBool(const std::string& _value) {
    return ParseValue<bool>(_value);
}

inline char ParseChar(const std::string& _value) {
    return ParseValue<char>(_value);
}

inline unsigned char ParseUChar(const std::string& _value) {
    return ParseValue<unsigned char>(_value);
}

namespace templates {
template <typename T>
inline void SplitImp(std::vector<std::string>& _ret, const std::string& _source, const std::string& _delims) {
    size_t start = 0;
    size_t end = _source.find(_delims);
    while (start != _source.npos) {
        if (end != _source.npos)
            _ret.push_back(_source.substr(start, end - start));
        else {
            _ret.push_back(_source.substr(start));
            break;
        }
        start = end;
        if (start != _source.npos) {
            start += _delims.size();
        }
        end = _source.find(_delims, start);
    }
}
} //namespace templates

inline std::vector<std::string> Split(const std::string& _source, const std::string& _delims = "\t\n ") {
    std::vector<std::string> result;
    templates::SplitImp<void>(result, _source, _delims);
    return result;
}

inline std::string& ReplaceAll(std::string& str, const std::string& tar, const std::string& sou) {
    if (str.empty() || str.size() < tar.size()) {
        return str;
    }
    auto pos = str.find(tar, 0);
    while (pos != std::string::npos) {
        str.replace(pos, tar.size(), sou);
        pos = str.find(tar, pos);
    }
    return str;
}

inline std::string& ToUpper(std::string& str) {
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

inline std::string& ToLower(std::string& str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}
} // namespace strutils
