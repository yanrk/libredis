/********************************************************
 * Description : redis db class
 * Author      : baoc, yanrk
 * Email       : yanrkchina@163.com
 * Version     : 3.0
 * History     :
 * Copyright(C): 2023
 ********************************************************/

#ifdef _MSC_VER
    #include <winsock2.h>
#else
    #include <sys/time.h>
#endif // _MSC_VER
#include <cstring>
#include <cstdio>
#include <string>
#include <list>
#include <vector>
#include <sstream>
#include "hiredis.h"
#include "hircluster.h"
#include "libredis.h"

#if 0 // defined(DEBUG) || defined(_DEBUG)
    #define RUN_LOG_ERR(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
    #define RUN_LOG_TRK(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
    #define RUN_LOG_DBG(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#else
    #define RUN_LOG_ERR(fmt, ...)
    #define RUN_LOG_TRK(fmt, ...)
    #define RUN_LOG_DBG(fmt, ...)
#endif // defined(DEBUG) || defined(_DEBUG)

template <typename T>
bool string_to_type(const std::string & str, T & val)
{
    std::istringstream iss(str);
    iss.setf(std::ios::boolalpha);
    iss >> val;
    return (!iss.fail());
}

template <typename T>
bool string_to_type(const std::string & str, T * val);

template <typename T>
bool type_to_string(T val, std::string & str)
{
    std::ostringstream oss;
    oss.setf(std::ios::fixed, std::ios::floatfield);
    oss.setf(std::ios::boolalpha);
    oss << val;
    str = oss.str();
    return (true);
}

template <typename T>
bool type_to_string(T * val, std::string & str);

template <>
bool type_to_string(const char * val, std::string & str)
{
    if (nullptr == val)
    {
        return (false);
    }
    else
    {
        str = val;
        return (true);
    }
}

template <typename T>
bool string_to_type(const std::list<std::string> & str_list, std::list<T> & val_list)
{
    bool ret = true;
    for (std::list<std::string>::const_iterator iter = str_list.begin(); str_list.end() != iter; ++iter)
    {
        T val;
        if (string_to_type(*iter, val))
        {
            val_list.push_back(val);
        }
        else
        {
            ret = false;
        }
    }
    return (ret);
}

template <typename T>
bool type_to_string(const std::list<T> & val_list, std::list<std::string> & str_list)
{
    bool ret = true;
    for (typename std::list<T>::const_iterator iter = val_list.begin(); val_list.end() != iter; ++iter)
    {
        std::string str;
        if (type_to_string(*iter, str))
        {
            str_list.push_back(str);
        }
        else
        {
            ret = false;
        }
    }
    return (ret);
}

class RedisDBImpl
{
public:
    RedisDBImpl();
    ~RedisDBImpl();

public:
    bool open(const std::string & address, const std::string & username, const std::string & password, const std::string & table, uint32_t timeout);
    void close();
    bool destroy();

public:
    bool set(const std::string & key, const std::string & value);
    bool get(const std::string & key, std::string & value);
    template <typename T> bool set(const std::string & key, T value);
    template <typename T> bool get(const std::string & key, T & value);

public:
    bool find(const std::string & key);
    bool find(const std::string & pattern, std::list<std::string> & keys);

public:
    bool erase(const std::string & key);
    bool erase(const std::list<std::string> & keys);

public:
    bool persist(const std::string & key);
    bool persist(const std::list<std::string> & keys);

public:
    bool expire(const std::string & key, const std::string & seconds);
    bool expire(const std::list<std::string> & keys, const std::string & seconds);

public:
    bool push_back(const std::string & queue, const std::string & value);
    bool pop_front(const std::string & queue, std::string & value);
    template <typename T> bool push_back(const std::string & queue, T value);
    template <typename T> bool pop_front(const std::string & queue, T & value);

public:
    bool clear(const std::string & queue);

private:
    bool login();
    void logoff();
    bool authenticate();
    bool select_table();

private:
    bool execute_command(const std::list<std::string> & args, int return_type, void * result);

private:
    bool                            m_running;
    std::string                     m_redis_address;
    std::string                     m_redis_username;
    std::string                     m_redis_password;
    std::string                     m_redis_table;
    struct timeval                  m_redis_timeout;
    redisContext                  * m_redis_context;
    redisClusterContext           * m_redis_cluster_context;
};

template <typename T>
bool RedisDBImpl::set(const std::string & key, T value)
{
    std::string str_value;
    return (type_to_string(value, str_value) && set(key, str_value));
}

template <typename T>
bool RedisDBImpl::get(const std::string & key, T & value)
{
    std::string str_value;
    return (get(key, str_value) && string_to_type(str_value, value));
}

template <typename T>
bool RedisDBImpl::push_back(const std::string & queue, T value)
{
    std::string str_value;
    return (type_to_string(value, str_value) && push_back(queue, str_value));
}

template <typename T>
bool RedisDBImpl::pop_front(const std::string & queue, T & value)
{
    std::string str_value;
    return (pop_front(queue, str_value) && string_to_type(str_value, value));
}

static int strcmp_ignore_case(const char * str1, const char * str2)
{
#ifdef _MSC_VER
    return (stricmp(str1, str2));
#else
    return (strcasecmp(str1, str2));
#endif // _MSC_VER
}

RedisDBImpl::RedisDBImpl()
    : m_running(false)
    , m_redis_address("127.0.0.1:6379")
    , m_redis_username()
    , m_redis_password()
    , m_redis_table("0")
    , m_redis_timeout()
    , m_redis_context(nullptr)
    , m_redis_cluster_context(nullptr)
{
    m_redis_timeout.tv_sec = 5;
    m_redis_timeout.tv_usec = 0;
}

RedisDBImpl::~RedisDBImpl()
{
    close();
}

bool RedisDBImpl::open(const std::string & address, const std::string & username, const std::string & password, const std::string & table, uint32_t timeout)
{
    close();

    RUN_LOG_DBG("redis db init begin");

    do
    {
        m_running = true;

        m_redis_address = address;
        m_redis_username = username;
        m_redis_password = password;
        m_redis_table = table;
        m_redis_timeout.tv_sec = timeout / 1000;
        m_redis_timeout.tv_usec = timeout % 1000 * 1000;

        if (!login())
        {
            RUN_LOG_ERR("redis db init failure while login to redis server");
            break;
        }

        RUN_LOG_DBG("redis db init success");

        return (true);
    } while (false);

    RUN_LOG_ERR("redis db init failure");

    close();

    return (false);
}

void RedisDBImpl::close()
{
    if (m_running)
    {
        m_running = false;

        RUN_LOG_DBG("redis db exit begin");

        logoff();

        RUN_LOG_DBG("redis db exit end");
    }
}

bool RedisDBImpl::login()
{
    if (nullptr != m_redis_context || nullptr != m_redis_cluster_context)
    {
        return (true);
    }

    if (std::string::npos == m_redis_address.find(','))
    {
        std::string redis_host;
        uint16_t redis_port = 6379;
        std::string::size_type pos = m_redis_address.find(':');
        if (std::string::npos == pos)
        {
            redis_host = m_redis_address;
        }
        else
        {
            redis_host = m_redis_address.substr(0, pos);
            string_to_type(m_redis_address.substr(pos + 1), redis_port);
        }

        m_redis_context = redisConnectWithTimeout(redis_host.c_str(), redis_port, m_redis_timeout);
        if (nullptr != m_redis_context && 0 == m_redis_context->err)
        {
            RUN_LOG_DBG("connect redis server [%s] success", m_redis_address.c_str());
            if (authenticate() && select_table())
            {
                return (true);
            }
        }
        else
        {
            RUN_LOG_ERR("connect redis server [%s] failure (%s)", m_redis_address.c_str(), (nullptr != m_redis_context ? m_redis_context->errstr : "unknown"));
        }
    }
    else
    {
        do
        {
            m_redis_cluster_context = redisClusterContextInit();
            if (nullptr == m_redis_cluster_context)
            {
                RUN_LOG_ERR("init redis cluster failure");
                break;
            }

            int result = redisClusterSetOptionAddNodes(m_redis_cluster_context, m_redis_address.c_str());
            if (REDIS_OK != result)
            {
                RUN_LOG_ERR("set redis cluster nodes [%s] failure (%s)", m_redis_address.c_str(), m_redis_cluster_context->errstr);
                break;
            }

            if (!m_redis_username.empty())
            {
                result = redisClusterSetOptionUsername(m_redis_cluster_context, m_redis_username.c_str());
                if (REDIS_OK != result)
                {
                    RUN_LOG_ERR("set redis cluster username [%s] failure (%s)", m_redis_username.c_str(), m_redis_cluster_context->errstr);
                    break;
                }
            }

            if (!m_redis_password.empty())
            {
                result = redisClusterSetOptionPassword(m_redis_cluster_context, m_redis_password.c_str());
                if (REDIS_OK != result)
                {
                    RUN_LOG_ERR("set redis cluster password [%s] failure (%s)", m_redis_password.c_str(), m_redis_cluster_context->errstr);
                    break;
                }
            }

            result = redisClusterSetOptionConnectTimeout(m_redis_cluster_context, m_redis_timeout);
            if (REDIS_OK != result)
            {
                RUN_LOG_ERR("set redis cluster timeout failure (%s)", m_redis_cluster_context->errstr);
                break;
            }

            result = redisClusterSetOptionRouteUseSlots(m_redis_cluster_context);
            if (REDIS_OK != result)
            {
                RUN_LOG_ERR("set redis cluster slots failure (%s)", m_redis_cluster_context->errstr);
                break;
            }

            result = redisClusterConnect2(m_redis_cluster_context);
            if (REDIS_OK != result)
            {
                RUN_LOG_ERR("connect redis cluster [%s] failure (%s)", m_redis_address.c_str(), m_redis_cluster_context->errstr);
                break;
            }

            RUN_LOG_DBG("connect redis cluster [%s] success", m_redis_address.c_str());

            return (true);
        } while (false);
    }

    logoff();

    return (false);
}

void RedisDBImpl::logoff()
{
    if (nullptr != m_redis_context)
    {
        redisFree(m_redis_context);
        m_redis_context = nullptr;
    }

    if (nullptr != m_redis_cluster_context)
    {
        redisClusterFree(m_redis_cluster_context);
        m_redis_cluster_context = nullptr;
    }
}

bool RedisDBImpl::execute_command(const std::list<std::string> & args, int return_type, void * result)
{
    if (!m_running || args.empty() || !login())
    {
        return (false);
    }

    std::string command;
    std::vector<const char *> arg_ptr;
    std::vector<size_t> arg_len;
    for (std::list<std::string>::const_iterator iter = args.begin(); args.end() != iter; ++iter)
    {
        const std::string & arg = *iter;
        if (command.empty())
        {
            command = arg;
        }
        else
        {
            command += " \"" + arg + "\"";
        }
        arg_ptr.push_back(arg.c_str());
        arg_len.push_back(arg.size());
    }
    const char * redis_name = nullptr;
    redisReply * redis_reply = nullptr;
    if (nullptr != m_redis_context)
    {
        redis_name = "server";
        redis_reply = reinterpret_cast<redisReply *>(redisCommandArgv(m_redis_context, static_cast<int>(args.size()), &arg_ptr[0], &arg_len[0]));
    }
    else
    {
        redis_name = "cluster";
        redis_reply = reinterpret_cast<redisReply *>(redisClusterCommandArgv(m_redis_cluster_context, static_cast<int>(args.size()), &arg_ptr[0], &arg_len[0]));
    }
    if (nullptr == redis_reply)
    {
        RUN_LOG_ERR("redis %s execute command [%s] failure", redis_name, command.c_str());
        logoff();
        return (false);
    }

    bool ret = false;
    bool good = true;

    if (return_type == redis_reply->type)
    {
        switch (return_type)
        {
            case REDIS_REPLY_STRING:
            {
                std::string & value = *reinterpret_cast<std::string *>(result);
                value = redis_reply->str;
                ret = true;
                break;
            }
            case REDIS_REPLY_ARRAY:
            {
                std::list<std::string> & values = *reinterpret_cast<std::list<std::string> *>(result);
                for (size_t index = 0; index < redis_reply->elements; ++index)
                {
                    values.push_back(redis_reply->element[index]->str);
                }
                ret = true;
                break;
            }
            case REDIS_REPLY_INTEGER:
            {
                ret = (redis_reply->integer > 0);
                break;
            }
            case REDIS_REPLY_NIL:
            {
                break;
            }
            case REDIS_REPLY_STATUS:
            {
                ret = (0 == strcmp_ignore_case(redis_reply->str, "ok"));
                break;
            }
            case REDIS_REPLY_ERROR:
            {
                good = false;
                break;
            }
            default:
            {
                good = false;
                break;
            }
        }

        if (ret)
        {
            RUN_LOG_DBG("redis execute command [%s] success", command.c_str());
        }
        else if (good)
        {
            RUN_LOG_TRK("redis execute command [%s] failure (%s)", command.c_str(), (REDIS_REPLY_ERROR == redis_reply->type ? redis_reply->str : "unknown"));
        }
        else
        {
            RUN_LOG_ERR("redis execute command [%s] exception (%s)", command.c_str(), (REDIS_REPLY_ERROR == redis_reply->type ? redis_reply->str : "unknown"));
        }
    }

    freeReplyObject(redis_reply);

    if (!good)
    {
        RUN_LOG_DBG("disconnect to redis %s", redis_name);
        logoff();
    }

    return (ret);
}

bool RedisDBImpl::authenticate()
{
    if (m_redis_password.empty())
    {
        return (true);
    }
    std::list<std::string> args;
    args.push_back("auth");
    args.push_back(m_redis_password);
    return (execute_command(args, REDIS_REPLY_STATUS, nullptr));
}

bool RedisDBImpl::select_table()
{
    std::list<std::string> args;
    args.push_back("select");
    args.push_back(m_redis_table);
    return (execute_command(args, REDIS_REPLY_STATUS, nullptr));
}

bool RedisDBImpl::destroy()
{
    std::list<std::string> args;
    args.push_back("flushdb");
    return (execute_command(args, REDIS_REPLY_STATUS, nullptr));
}

bool RedisDBImpl::set(const std::string & key, const std::string & value)
{
    std::list<std::string> args;
    args.push_back("set");
    args.push_back(key);
    args.push_back(value);
    return (execute_command(args, REDIS_REPLY_STATUS, nullptr));
}

bool RedisDBImpl::get(const std::string & key, std::string & value)
{
    std::list<std::string> args;
    args.push_back("get");
    args.push_back(key);
    return (execute_command(args, REDIS_REPLY_STRING, &value));
}

bool RedisDBImpl::find(const std::string & key)
{
    std::list<std::string> args;
    args.push_back("exists");
    args.push_back(key);
    return (execute_command(args, REDIS_REPLY_INTEGER, nullptr));
}

bool RedisDBImpl::find(const std::string & pattern, std::list<std::string> & keys)
{
    if (nullptr != m_redis_cluster_context)
    {
        RUN_LOG_ERR("redis cluster not support find pattern");
        return (false);
    }
    std::list<std::string> args;
    args.push_back("keys");
    args.push_back(pattern);
    return (execute_command(args, REDIS_REPLY_ARRAY, &keys));
}

bool RedisDBImpl::erase(const std::string & key)
{
    std::list<std::string> args;
    args.push_back("del");
    args.push_back(key);
    return (execute_command(args, REDIS_REPLY_INTEGER, nullptr));
}

bool RedisDBImpl::erase(const std::list<std::string> & keys)
{
    bool ret = true;
    for (std::list<std::string>::const_iterator iter = keys.begin(); keys.end() != iter; ++iter)
    {
        if (!erase(*iter))
        {
            ret = false;
        }
    }
    return (ret);
}

bool RedisDBImpl::persist(const std::string & key)
{
    std::list<std::string> args;
    args.push_back("persist");
    args.push_back(key);
    return (execute_command(args, REDIS_REPLY_INTEGER, nullptr));
}

bool RedisDBImpl::persist(const std::list<std::string> & keys)
{
    bool ret = true;
    for (std::list<std::string>::const_iterator iter = keys.begin(); keys.end() != iter; ++iter)
    {
        if (!persist(*iter))
        {
            ret = false;
        }
    }
    return (ret);
}

bool RedisDBImpl::expire(const std::string & key, const std::string & seconds)
{
    std::list<std::string> args;
    args.push_back("expire");
    args.push_back(key);
    args.push_back(seconds);
    return (execute_command(args, REDIS_REPLY_INTEGER, nullptr));
}

bool RedisDBImpl::expire(const std::list<std::string> & keys, const std::string & seconds)
{
    bool ret = true;
    for (std::list<std::string>::const_iterator iter = keys.begin(); keys.end() != iter; ++iter)
    {
        if (!expire(*iter, seconds))
        {
            ret = false;
        }
    }
    return (ret);
}

bool RedisDBImpl::push_back(const std::string & queue, const std::string & value)
{
    std::list<std::string> args;
    args.push_back("rpush");
    args.push_back(queue);
    args.push_back(value);
    return (execute_command(args, REDIS_REPLY_INTEGER, nullptr));
}

bool RedisDBImpl::pop_front(const std::string & queue, std::string & value)
{
    std::list<std::string> args;
    args.push_back("lpop");
    args.push_back(queue);
    return (execute_command(args, REDIS_REPLY_STRING, &value));
}

bool RedisDBImpl::clear(const std::string & queue)
{
    return (erase(queue));
}

RedisDB::RedisDB() : m_redis_db_impl(nullptr)
{

}

RedisDB::~RedisDB()
{
    close();
}

bool RedisDB::open(const std::string & address, const std::string & username, const std::string & password, uint16_t table, uint32_t timeout)
{
    close();

    std::string str_table("0");
    type_to_string(table, str_table);

    m_redis_db_impl = new RedisDBImpl;
    if (nullptr != m_redis_db_impl && m_redis_db_impl->open(address, username, password, str_table, timeout))
    {
        return (true);
    }

    close();

    return (false);
}

void RedisDB::close()
{
    if (nullptr != m_redis_db_impl)
    {
        m_redis_db_impl->close();
        delete m_redis_db_impl;
        m_redis_db_impl = nullptr;
    }
}

bool RedisDB::destroy()
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->destroy());
}

bool RedisDB::find(const std::string & key)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->find(key));
}

bool RedisDB::find(const std::string & pattern, std::list<std::string> & keys)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->find(pattern, keys));
}

bool RedisDB::erase(const std::string & key)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->erase(key));
}

bool RedisDB::erase(const std::list<std::string> & keys)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->erase(keys));
}

bool RedisDB::persist(const std::string & key)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->persist(key));
}

bool RedisDB::persist(const std::list<std::string> & keys)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->persist(keys));
}

bool RedisDB::expire(const std::string & key, int64_t seconds)
{
    std::string str_seconds;
    return (nullptr != m_redis_db_impl && type_to_string(seconds, str_seconds) && m_redis_db_impl->expire(key, str_seconds));
}

bool RedisDB::expire(const std::list<std::string> & keys, int64_t seconds)
{
    std::string str_seconds;
    return (nullptr != m_redis_db_impl && type_to_string(seconds, str_seconds) && m_redis_db_impl->expire(keys, str_seconds));
}

bool RedisDB::set(const std::string & key, const char * value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->set(key, value));
}

bool RedisDB::set(const std::string & key, const std::string & value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->set(key, value));
}

bool RedisDB::set(const std::string & key, bool value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->set(key, value));
}

bool RedisDB::set(const std::string & key, int8_t value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->set(key, value));
}

bool RedisDB::set(const std::string & key, uint8_t value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->set(key, value));
}

bool RedisDB::set(const std::string & key, int16_t value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->set(key, value));
}

bool RedisDB::set(const std::string & key, uint16_t value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->set(key, value));
}

bool RedisDB::set(const std::string & key, int32_t value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->set(key, value));
}

bool RedisDB::set(const std::string & key, uint32_t value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->set(key, value));
}

bool RedisDB::set(const std::string & key, int64_t value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->set(key, value));
}

bool RedisDB::set(const std::string & key, uint64_t value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->set(key, value));
}

bool RedisDB::set(const std::string & key, float value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->set(key, value));
}

bool RedisDB::set(const std::string & key, double value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->set(key, value));
}

bool RedisDB::get(const std::string & key, std::string & value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->get(key, value));
}

bool RedisDB::get(const std::string & key, bool & value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->get(key, value));
}

bool RedisDB::get(const std::string & key, int8_t & value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->get(key, value));
}

bool RedisDB::get(const std::string & key, uint8_t & value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->get(key, value));
}

bool RedisDB::get(const std::string & key, int16_t & value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->get(key, value));
}

bool RedisDB::get(const std::string & key, uint16_t & value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->get(key, value));
}

bool RedisDB::get(const std::string & key, int32_t & value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->get(key, value));
}

bool RedisDB::get(const std::string & key, uint32_t & value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->get(key, value));
}

bool RedisDB::get(const std::string & key, int64_t & value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->get(key, value));
}

bool RedisDB::get(const std::string & key, uint64_t & value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->get(key, value));
}

bool RedisDB::get(const std::string & key, float & value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->get(key, value));
}

bool RedisDB::get(const std::string & key, double & value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->get(key, value));
}

bool RedisDB::clear(const std::string & queue)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->clear(queue));
}

bool RedisDB::push_back(const std::string & queue, const char * value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->push_back(queue, value));
}

bool RedisDB::push_back(const std::string & queue, const std::string & value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->push_back(queue, value));
}

bool RedisDB::push_back(const std::string & queue, bool value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->push_back(queue, value));
}

bool RedisDB::push_back(const std::string & queue, int8_t value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->push_back(queue, value));
}

bool RedisDB::push_back(const std::string & queue, uint8_t value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->push_back(queue, value));
}

bool RedisDB::push_back(const std::string & queue, int16_t value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->push_back(queue, value));
}

bool RedisDB::push_back(const std::string & queue, uint16_t value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->push_back(queue, value));
}

bool RedisDB::push_back(const std::string & queue, int32_t value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->push_back(queue, value));
}

bool RedisDB::push_back(const std::string & queue, uint32_t value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->push_back(queue, value));
}

bool RedisDB::push_back(const std::string & queue, int64_t value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->push_back(queue, value));
}

bool RedisDB::push_back(const std::string & queue, uint64_t value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->push_back(queue, value));
}

bool RedisDB::push_back(const std::string & queue, float value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->push_back(queue, value));
}

bool RedisDB::push_back(const std::string & queue, double value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->push_back(queue, value));
}

bool RedisDB::pop_front(const std::string & queue, std::string & value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->pop_front(queue, value));
}

bool RedisDB::pop_front(const std::string & queue, bool & value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->pop_front(queue, value));
}

bool RedisDB::pop_front(const std::string & queue, int8_t & value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->pop_front(queue, value));
}

bool RedisDB::pop_front(const std::string & queue, uint8_t & value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->pop_front(queue, value));
}

bool RedisDB::pop_front(const std::string & queue, int16_t & value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->pop_front(queue, value));
}

bool RedisDB::pop_front(const std::string & queue, uint16_t & value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->pop_front(queue, value));
}

bool RedisDB::pop_front(const std::string & queue, int32_t & value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->pop_front(queue, value));
}

bool RedisDB::pop_front(const std::string & queue, uint32_t & value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->pop_front(queue, value));
}

bool RedisDB::pop_front(const std::string & queue, int64_t & value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->pop_front(queue, value));
}

bool RedisDB::pop_front(const std::string & queue, uint64_t & value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->pop_front(queue, value));
}

bool RedisDB::pop_front(const std::string & queue, float & value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->pop_front(queue, value));
}

bool RedisDB::pop_front(const std::string & queue, double & value)
{
    return (nullptr != m_redis_db_impl && m_redis_db_impl->pop_front(queue, value));
}
