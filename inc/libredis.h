/********************************************************
 * Description : redis db class
 * Author      : baoc, yanrk
 * Email       : yanrkchina@163.com
 * Version     : 3.0
 * History     :
 * Copyright(C): 2023
 ********************************************************/

#ifndef REDIS_DB_H
#define REDIS_DB_H


#ifdef _MSC_VER
    #define LIBREDIS_CDECL           __cdecl
    #define LIBREDIS_STDCALL         __stdcall
    #ifdef EXPORT_LIBREDIS_DLL
        #define LIBREDIS_API         __declspec(dllexport)
    #else
        #ifdef USE_LIBREDIS_DLL
            #define LIBREDIS_API     __declspec(dllimport)
        #else
            #define LIBREDIS_API
        #endif // USE_LIBREDIS_DLL
    #endif // EXPORT_LIBREDIS_DLL
#else
    #define LIBREDIS_CDECL
    #define LIBREDIS_STDCALL
    #define LIBREDIS_API
#endif // _MSC_VER

#include <cstdint>
#include <string>
#include <list>

class RedisDBImpl;

class LIBREDIS_API RedisDB
{
public:
    RedisDB();
    RedisDB(const RedisDB &);
    RedisDB(RedisDB &&);
    RedisDB & operator = (const RedisDB &);
    RedisDB & operator = (RedisDB &&);
    ~RedisDB();

public:
    bool open(const std::string & address, const std::string & username, const std::string & password, uint16_t table = 0, uint32_t timeout = 5000);
    void close();
    bool destroy();

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
    bool expire(const std::string & key, int64_t seconds);
    bool expire(const std::list<std::string> & keys, int64_t seconds);

public:
    bool set(const std::string & key, const char * value);
    bool set(const std::string & key, const std::string & value);
    bool set(const std::string & key, bool value);
    bool set(const std::string & key, int8_t value);
    bool set(const std::string & key, uint8_t value);
    bool set(const std::string & key, int16_t value);
    bool set(const std::string & key, uint16_t value);
    bool set(const std::string & key, int32_t value);
    bool set(const std::string & key, uint32_t value);
    bool set(const std::string & key, int64_t value);
    bool set(const std::string & key, uint64_t value);
    bool set(const std::string & key, float value);
    bool set(const std::string & key, double value);

public:
    bool get(const std::string & key, std::string & value);
    bool get(const std::string & key, bool & value);
    bool get(const std::string & key, int8_t & value);
    bool get(const std::string & key, uint8_t & value);
    bool get(const std::string & key, int16_t & value);
    bool get(const std::string & key, uint16_t & value);
    bool get(const std::string & key, int32_t & value);
    bool get(const std::string & key, uint32_t & value);
    bool get(const std::string & key, int64_t & value);
    bool get(const std::string & key, uint64_t & value);
    bool get(const std::string & key, float & value);
    bool get(const std::string & key, double & value);

public:
    bool clear(const std::string & queue);

public:
    bool push_back(const std::string & queue, const char * value);
    bool push_back(const std::string & queue, const std::string & value);
    bool push_back(const std::string & queue, bool value);
    bool push_back(const std::string & queue, int8_t value);
    bool push_back(const std::string & queue, uint8_t value);
    bool push_back(const std::string & queue, int16_t value);
    bool push_back(const std::string & queue, uint16_t value);
    bool push_back(const std::string & queue, int32_t value);
    bool push_back(const std::string & queue, uint32_t value);
    bool push_back(const std::string & queue, int64_t value);
    bool push_back(const std::string & queue, uint64_t value);
    bool push_back(const std::string & queue, float value);
    bool push_back(const std::string & queue, double value);

public:
    bool pop_front(const std::string & queue, std::string & value);
    bool pop_front(const std::string & queue, bool & value);
    bool pop_front(const std::string & queue, int8_t & value);
    bool pop_front(const std::string & queue, uint8_t & value);
    bool pop_front(const std::string & queue, int16_t & value);
    bool pop_front(const std::string & queue, uint16_t & value);
    bool pop_front(const std::string & queue, int32_t & value);
    bool pop_front(const std::string & queue, uint32_t & value);
    bool pop_front(const std::string & queue, int64_t & value);
    bool pop_front(const std::string & queue, uint64_t & value);
    bool pop_front(const std::string & queue, float & value);
    bool pop_front(const std::string & queue, double & value);

private:
    RedisDBImpl                       * m_redis_db_impl;
};


#endif // REDIS_DB_H
