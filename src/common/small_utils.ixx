module;

#include <functional>
#include <memory>

#include <boost/type_index.hpp>
#include <iomanip>
#include <random>
#include <regex>
#include <sstream>

export module small_utils;

export namespace fast::util
{
// 禁止拷贝基类
class NonCopyable
{
  public:
    NonCopyable() = default;

    NonCopyable(const NonCopyable &) = delete;

    NonCopyable &operator=(const NonCopyable &) = delete;

    NonCopyable &operator=(NonCopyable &&that) = delete;
};

/**
 *
 *     std::shared_ptr<void> auto_free(nullptr, [&i](void *)
 *                                   {
 *                                       LOG_INFO << i;
 *                                   }
 *   );
 *    可用共享指针替代
 */

class OnceToken : public NonCopyable
{
  public:
    using task = std::function<void(void)>;

    template <typename FUNC> explicit OnceToken(const FUNC &onConstructed, task onDestructed = nullptr)
    {
        onConstructed();
        _onDestructed = std::move(onDestructed);
    }

    explicit OnceToken(std::nullptr_t, task onDestructed = nullptr)
    {
        _onDestructed = std::move(onDestructed);
    }

    ~OnceToken()
    {
        if (_onDestructed)
        {
            _onDestructed();
        }
    }

  public:
    OnceToken() = delete;

    OnceToken(const OnceToken &) = delete;

    OnceToken(OnceToken &&) = delete;

    OnceToken &operator=(const OnceToken &) = delete;

    OnceToken &operator=(OnceToken &&) = delete;

  private:
    task _onDestructed;
};

// 对象安全的构建和析构
// 构建后执行onCreate函数
// 析构前执行onDestory函数
// 在函数onCreate和onDestory中可以执行构造或析构中不能调用的方法，比如说shared_from_this或者虚函数
class Creator
{
  public:
    template <typename C, typename... ArgsType> static std::shared_ptr<C> create(ArgsType &&...args)
    {
        std::shared_ptr<C> ret(new C(std::forward<ArgsType>(args)...), [](C *ptr) {
            ptr->onDestory();
            delete ptr;
        });
        ret->onCreate();
        return ret;
    }

  private:
    Creator() = default;

    ~Creator() = default;
};

/**
 * @brief 单例模式封装类
 * @details T 类型
 */
template <class T> class Singleton
{
  public:
    /**
     * @brief 返回单例裸指针
     */
    static T *GetInstance()
    {
        static T v;
        return &v;
    }
    // 删除拷贝构造和赋值操作
    Singleton(const Singleton &) = delete;
    Singleton &operator=(const Singleton &) = delete;

  protected:
    Singleton() = default;
    virtual ~Singleton() = default;
};

/**
 * @brief 单例模式智能指针封装类
 * @details T 类型
 */
template <class T> class SingletonPtr
{
  public:
    /**
     * @brief 返回单例智能指针
     */
    static std::shared_ptr<T> GetInstance()
    {
        static std::shared_ptr<T> v(new T);
        return v;
    }
    // 删除拷贝构造和赋值操作
    SingletonPtr(const SingletonPtr &) = delete;
    SingletonPtr &operator=(const SingletonPtr &) = delete;

  protected:
    SingletonPtr() = default;
    virtual ~SingletonPtr() = default;
};

// 工厂模板类
template <typename product_base, typename... products> class factory final
{
  public:
    using string = std::string_view;

    template <typename T> struct clz_name_t
    {
        string id = boost::typeindex::type_id_with_cvr<T>().pretty_name();
        T data;
    };

    using named_products = std::tuple<clz_name_t<products>...>;

    template <typename... Args> static auto create(const string &id, Args &&...args)
    {
        std::unique_ptr<product_base> result{};

        std::apply([](auto &&...it) { ((static_check<decltype(it.data)>()), ...); }, named_products{});

        std::apply(
            [&](auto &&...it) {
                ((it.id == id ? result = std::make_unique<decltype(it.data)>(std::forward<Args>(args)...) : result),
                 ...);
            },
            named_products{});
        return result;
    }

    template <typename... Args> static std::shared_ptr<product_base> make_shared(const string &id, Args &&...args)
    {
        return std::shared_ptr<product_base>(create(id, std::forward<Args>(args)...));
    }

    template <typename... Args> static std::unique_ptr<product_base> make_unique(const string &id, Args &&...args)
    {
        return create(id, std::forward<Args>(args)...);
    }

    template <typename... Args> static product_base *create_nacked_ptr(const string &id, Args &&...args)
    {
        return create(id, std::forward<Args>(args)...).release();
    }

  private:
    template <typename product> static void static_check()
    {
        static_assert(std::is_base_of<product_base, product>::value, "All products must inherit from product_base");
    }
};

template <class T> inline bool compare_vector_values(std::vector<T> const &v1, std::vector<T> const &v2)
{
    bool not_ok = false;
    if (v1.size() == v2.size())
    {
        for (std::size_t i = 0; i < v1.size(); i++)
        {
            if (std::strcmp(v1[i], v2[i]) != 0)
            {
                not_ok = true;
                break;
            }
        }
    }
    else
    {
        not_ok = true;
    }
    return (not_ok == false);
}

class UUID
{
  public:
    static std::string generate()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);
        static std::uniform_int_distribution<> dis2(8, 11);

        std::stringstream ss;
        ss << std::hex;
        for (int i = 0; i < 8; i++)
        {
            ss << dis(gen);
        }
        ss << "-";
        for (int i = 0; i < 4; i++)
        {
            ss << dis(gen);
        }
        ss << "-4";
        for (int i = 0; i < 3; i++)
        {
            ss << dis(gen);
        }
        ss << "-";
        ss << dis2(gen);
        for (int i = 0; i < 3; i++)
        {
            ss << dis(gen);
        }
        ss << "-";
        for (int i = 0; i < 12; i++)
        {
            ss << dis(gen);
        }
        return ss.str();
    }
};

std::string remove_html_tags(const std::string &html)
{
    std::regex re("<[^>]*>");
    return std::regex_replace(html, re, "");
}

std::string as_string(std::string_view view)
{
    return std::string(view);
}

bool is_utf8_starter(char c)
{
    return (c & 0x80) != 0 && (c & 0x40) != 0;
}

std::string split_first_n_unicode_chars(const std::string &utf8_str, size_t n)
{
    std::string result;
    size_t unicode_chars_count = 0;
    size_t i = 0;

    while (i < utf8_str.size() && unicode_chars_count < n)
    {
        if (is_utf8_starter(utf8_str[i]))
        {
            size_t bytes_needed = 1;
            if ((utf8_str[i] & 0xE0) == 0xC0)
                bytes_needed = 2;
            else if ((utf8_str[i] & 0xF0) == 0xE0)
                bytes_needed = 3;
            else if ((utf8_str[i] & 0xF8) == 0xF0)
                bytes_needed = 4;

            result.append(utf8_str.begin() + i, utf8_str.begin() + i + bytes_needed);
            unicode_chars_count++;
            i += bytes_needed - 1;
        }
        i++;
    }

    return result;
}

} // namespace fast::util
