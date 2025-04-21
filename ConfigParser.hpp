#pragma once

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

template<typename T, typename = void>
class Option; // フォワード宣言

// =======================
// OptionBase (型消去用)
// =======================
class OptionBase {
public:
    virtual void setValue(const std::string& str) = 0;
    virtual void applyDefault() = 0;
    virtual ~OptionBase() = default;
};

template<typename T>
class Option<
  T,
  typename std::enable_if<std::is_same<
    decltype(std::declval<std::istream&>() >> std::declval<T&>()),
    std::istream&>::value>::type> : public OptionBase 
{
public:
    Option(const std::string& name, T& ref): name_(name), ref_(ref)
    {}

    template<typename U>
    Option<T>* default_val(const U& val)
    {
        std::ostringstream oss;
        oss << val;
        defaultVal_ = oss.str();
        return this;
    }

    Option<T>* expected(size_t n)
    {
        expectedCount_ = n;
        return this;
    }

    void applyDefault() override
    {
        if (!defaultVal_.empty())
            setValue(defaultVal_);
    }

    void setValue(const std::string& str) override
    {
        std::istringstream iss(str);
        T val;
        iss >> val;
        if (!iss.fail()) {
            ref_ = val;
        } else {
            throw std::runtime_error("Failed to parse value: " + str);
        }
    }

private:
    T& ref_;
    std::string name_;
    std::string defaultVal_;
    size_t expectedCount_ = 0;
};

template<typename T>
class Option<std::vector<T>, void> : public OptionBase 
{
public:
    Option(const std::string& name, std::vector<T>& ref): name_(name), ref_(ref)
    {}

    template<typename U>
    Option<std::vector<T>>* default_val(const U& val)
    {
        std::ostringstream oss;
        oss << val;
        defaultVal_ = oss.str();
        return this;
    }

    Option<std::vector<T>>* expected(size_t n)
    {
        expectedCount_ = n;
        return this;
    }

    void applyDefault() override
    {
        if (!defaultVal_.empty())
            setValue(defaultVal_);
    }

    void setValue(const std::string& str) override
    {
        ref_.clear();
        std::istringstream iss(str);
        std::string token;

        while (std::getline(iss, token, ',')) {
            std::istringstream elem_iss(token);
            T val;
            elem_iss >> val;
            if (elem_iss) {
            ref_.push_back(val);
            } else {
            throw std::runtime_error("Parse error in vector element: " + token);
            }
        }

        if (expectedCount_ > 0 && ref_.size() != expectedCount_) {
            throw std::runtime_error(
            "Expected " + std::to_string(expectedCount_) + " elements, got "
            + std::to_string(ref_.size()));
        }
    }

private:
    std::vector<T>& ref_;
    std::string name_;
    std::string defaultVal_;
    size_t expectedCount_ = 0;
};

template<typename T>
class Option<T, typename std::enable_if<std::is_enum<T>::value>::type>
  : public OptionBase 
{
public:
    Option(const std::string& name, T& ref): name_(name), ref_(ref)
    {}

    template<typename U>
    Option<T>* default_val(const U& val)
    {
        std::ostringstream oss;
        oss << static_cast<typename std::underlying_type<T>::type>(val);
        defaultVal_ = oss.str();
        return this;
    }

    Option<T>* expected(size_t n)
    {
        expectedCount_ = n;
        return this;
    }

    void applyDefault() override
    {
        if (!defaultVal_.empty())
            setValue(defaultVal_);
    }

    void setValue(const std::string& str) override
    {
        // 数値からenum変換（文字列からも拡張可能）
        std::istringstream iss(str);
        typename std::underlying_type<T>::type val;
        iss >> val;
        if (!iss.fail()) {
            ref_ = static_cast<T>(val);
        } else {
            throw std::runtime_error("Failed to parse enum value: " + str);
        }
    }

private:
    T& ref_;
    std::string name_;
    std::string defaultVal_;
    size_t expectedCount_ = 0;
};

// =======================
// Parser 本体
// =======================
class ConfigParser 
{
public:
    /// <summary>
    /// Constructor
    /// </summary>
    /// <param name="delimiter">YAMLのデリミター':'以外も可能</param>
    ConfigParser(std::string delimiter = ":"): delimiter_(delimiter)
    {}

    /// <summary>
    /// オプション設定
    /// </summary>
    /// <typeparam name="T"></typeparam>
    /// <param name="name"></param>
    /// <param name="variable">ベクトルの初期値を与えたい場合は文字列で列挙</param>
    /// <param name="description"></param>
    /// <returns></returns>
    template<typename T>
    Option<T>* add_option(const std::string& name, T& variable, std::string description = "")
    {
        auto opt = std::make_unique<Option<T>>(name, variable);
        Option<T>* rawPtr = opt.get(); // チェーン用
        options_[name] = std::move(opt);
        return rawPtr;
    }

    /// <summary>
    /// YAML形式の設定ファイルをパースする
    /// </summary>
    /// <param name="configFile"></param>
    /// <returns></returns>
    int parse(const char* configFile)
    {
        std::vector<std::string> lines;

        std::ifstream file(configFile);
        std::string line;

        while (std::getline(file, line)) {
            // 空行やコメント行をスキップ
            if (line.empty() || line[0] == '#') {
            continue;
            } else if (line.size() == 1 && line[0] == ' ') {
            // 1文字だけの行は無視
            //continue;
            }

            std::string arg_name, value;

            size_t pos = line.find(delimiter_);
            if (pos != std::string::npos) {
            arg_name = line.substr(0, pos);

            std::cout << arg_name << std::endl;

            value = line.substr(pos + delimiter_.length());
            if (options_.count(arg_name)) {
                options_[arg_name]->setValue(value);
            }
            }
        }
        return 0;
    }

protected:
    void set(const std::string& name, const std::string& value)
    {
        if (options_.count(name)) {
            options_[name]->setValue(value);
        }
    }

private:
    std::string delimiter_;
    std::unordered_map<std::string, std::unique_ptr<OptionBase>> options_;
};
