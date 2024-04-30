#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>

namespace xs {

#ifndef _HIREDIS_LOG
#define _HIREDIS_LOG(_MSG_)
#endif

class HiRedisHelper {
  public:
    typedef std::string TKey;
    typedef std::string TField;
    typedef std::string TValue;
    typedef std::vector<TField> TFields;
    typedef std::vector<TKey> TKeys;
    typedef std::shared_ptr<TValue> TValuePtr;
    typedef std::vector<TValuePtr> TValues;
    typedef std::set<TValuePtr> TSet;

    struct TSSetValue {
        TKey key;
        int64_t score = 0;
    };

    typedef std::shared_ptr<TSSetValue> TSSetValuePtr;
    typedef std::vector<TSSetValuePtr> TSSet;
    typedef std::map<TKey, TValuePtr> THash;

    using RedisReplyPtr = std::shared_ptr<redisReply>;

    bool Connect(const std::string& strAddress, const std::string& strPass = "") {
        auto nFind = strAddress.find(":");
        if (nFind == std::string::npos) {
            LOG_E("straddr is not find port");
            return false;
        }
        auto strIP = strAddress.substr(0, nFind);
        auto nPort = atoi(strAddress.c_str() + nFind + 1);
        return Connect(strIP, nPort, 10, strPass);
    }
    bool Connect(const std::string& strHost, uint32_t nPort, uint32_t nOutTime = 10, const std::string& strPass = "") {
        if (NULL != _pCtx) {
            redisFree(_pCtx);
            _pCtx = NULL;
        }
        _strHost = strHost;
        _nPort = nPort;
        _nTimeout = nOutTime;
        _strPass = strPass;

        bool bRet = false;
        timeval _timeoutVal{_nTimeout, 0};

        _pCtx = redisConnectWithTimeout(_strHost.c_str(), _nPort, _timeoutVal);
        if (NULL == _pCtx || _pCtx->err) {
            if (NULL != _pCtx) {
                SetErrInfo(_pCtx->errstr);
                redisFree(_pCtx);
                _pCtx = NULL;
            } else {
                SetErrInfo("pctx is create emtpy");
            }
        } else {
            if (_strPass.empty()) {
                bRet = true;
            } else {
                bRet = Auth(strPass);
                if (!bRet) {
                    SetErrInfo("login pass word error");
                }
            }
        }
        return bRet;
    }

  public:
    //connection
    // AUTH
    bool Auth(const TKey& pass) {
        auto pReply = CommandWrap("AUTH %s", _strPass.c_str());
        return CheckReply(pReply);
    }

    // PING
    bool Ping() {
        auto pReply = CommandWrap("PING");
        return CheckReply(pReply);
    }

    // ECHO
    // QUIT
    // SELECT

    // Commands operating on string values
    // APPEND        bool append( const string& key, const string& value);
    // BITCOUNT      bool bitcount( const string& key, int& count, const int start = 0, const int end = 0);
    // BITOP         bool bitop( const BITOP operation, const string& destkey, const KEYS& keys, int& lenght);
    // BITPOS        bool bitpos( const string& key, const int bit, int64_t& pos, const int start = 0, const int end = 0);
    bool Decr(const TKey& key, int64_t& result) {
        return CommandInteger(result, "DECR %s", key.c_str());
    }
    // DECRBY        bool decrby( const string& key, const int by, int64_t& result);
    // GET
    bool Get(const TKey& key, TValue& value) {
        return CommandString(value, "GET %s", key.c_str());
    }
    // GETBIT        bool getbit( const string& key, const int& offset, int& bit);
    // GETRANGE      bool getrange( const string& key, const int start, const int end, string& out);
    // GETSET        bool getset( const string& key, const string& newValue, string& oldValue);
    bool Incr(const TKey& key, int64_t& result) {
        return CommandInteger(result, "INCR %s", key.c_str());
    }
    // INCRBY        bool incrby( const string& key, const int by, int64_t& result);
    // INCRBYFLOAT
    // MGET          bool mget(const DBIArray& dbi, const KEYS &  keys, ReplyData& vDdata);
    // MSET          bool mset(const DBIArray& dbi, const VDATA& data);
    // MSETNX
    // PSETEX        bool psetex( const string& key, const int milliseconds, const string& value);
    // SET

    bool Set(const TKey& key, const TValue& value, const int second = 0) {
        return Set(key, value.c_str(), value.length(), second);
    }

    bool Set(const TKey& key, const char* value, int len, const int second = 0) {
        if (0 == second) {
            return CommandBool("SET %s %b", key.c_str(), value, len);
        } else {
            return CommandBool("SET %s %b EX %d", key.c_str(), value, len, second);
        }
    }

    // SETBIT        bool setbit( const string& key, const int offset, const int64_t newbitValue, int64_t oldbitValue);
    // SETEX         bool setex( const string& key, const int seconds, const string& value);
    // SETNX         bool setnx( const string& key, const string& value);
    // SETRANGE      bool setrange( const string& key, const int offset, const string& value, int& length);
    // STRLEN        bool strlen( const string& key, int& length);

    // DEL
    bool Del(const TKey& key) {
        int64_t nDel = 0;
        return CommandInteger(nDel, "DEL %s", key.c_str());
    }
    //bool del(const DBIArray& dbi, const KEYS &  vkey, int64_t& count);
    // DUMP
    // EXISTS         bool exists( const string& key);
    // EXPIRE         bool expire( const string& key, const unsigned int second);
    // EXPIREAT       bool expireat( const string& key, const unsigned int timestamp);
    // KEYS
    // MIGRATE
    // MOVE
    // OBJECT
    // PERSIST        bool persist( const string& key);
    // PEXPIRE        bool pexpire( const string& key, const unsigned int milliseconds);
    // PEXPIREAT      bool pexpireat( const string& key, const unsigned int millisecondstimestamp);
    // PTTL           bool pttl( const string& key, int64_t &milliseconds);
    // RANDOMKEY      bool randomkey( KEY& key);
    // RENAME
    // RENAMENX
    // RESTORE
    // SCAN

    // SORT           bool sort( ArrayReply& array, const string& key, const char* by = NULL,
    //							  LIMIT *limit = NULL, bool alpha = false, const FILEDS* get = NULL,
    //							  const SORTODER order = ASC, const char* destination = NULL);

    // TTL            bool ttl( const string& key, int64_t& seconds);
    // TYPE

    // HDEL
    bool HDel(const TKey& key, const TField& filed, int64_t* num) {
        return CommandInteger(*num, "HDEL %s %s", key.c_str(), filed.c_str());
    }
    //bool hdel( const string& key, const KEYS& vfiled, int64_t& num);
    // HEXISTS        bool hexist( const string& key, const string& filed);
    // HGET

    bool HGet(const TKey& key, const TField& filed, TValue* value) {
        return CommandString(*value, "HGET %s %s", key.c_str(), filed.c_str());
    }

    // HGETALL
    bool HGetAll(const TKey& key, THash ret) {
        bool bOK = CommandHash(ret, "HGETALL %s", key.c_str());
        return bOK;
    }
    // HINCRBY
    bool HIncrby(const TKey& key, const TField& field, int32_t increment, int64_t& value) {
        return CommandInteger(value, "HINCRBY %s %s %d", key.c_str(), field.c_str(), increment);
    }
    // HINCRBYFLOAT   bool hincrbyfloat( const string& key, const string& filed, const float increment, float& value);
    bool HKeys(const TKey& key, TValues* values) {
        return CommandArray(*values, "HKEYS %s", key.c_str());
    }
    // HLEN
    bool HLen(const TKey& key, int64_t* count) {
        return CommandInteger(*count, "HLEN %s", key.c_str());
    }
    // HMGET          bool hmget( const string& key, const KEYS& filed, ArrayReply& array);
    // HMSET          bool hmset( const string& key, const VDATA& vData);
    // HSCAN

    bool HSet(const TKey& key, const TField& filed, const TValue& value) {
        int64_t retval = 0;
        bool bOK = CommandInteger(retval, "HSET %s %s %b", key.c_str(), filed.c_str(), value.c_str(), value.length());
        return bOK && retval == 1;
    }

    bool HSetnx(const TKey& key, const TField& filed, const TValue& value) {
        int64_t retval = 0;
        bool bOK = CommandInteger(retval, "HSETNX %s %s %b", key.c_str(), filed.c_str(), value.c_str(), value.length());
        return bOK && retval == 1;
    }

    // HVALS          bool hvals( const string& key, VALUES& values);

    // BLPOP
    // BRPOP
    // BRPOPLPUSH
    // LINDEX         bool lindex( const string& key, const int64_t index, VALUE& value);
    // LINSERT        bool linsert( const string& key, const LMODEL mod, const string& pivot, const string& value, int64_t& retval);
    // LLEN           bool llen( const string& key, int64_t& len);
    // LPOP           bool lpop( const string& key, string& value);
    // LPUSH          bool lpush( const string& key, const VALUES& vValue, int64_t& length);
    // LPUSHX         bool lpushx( const string& key, const string& value, int64_t& length);
    // LRANGE         bool lrange( const string& key, const int64_t start, const int64_t end, ArrayReply& array);
    // LREM           bool lrem( const string& key, const int count, const string& value, int64_t num);
    // LSET           bool lset( const string& key, const int index, const string& value);
    // LTRIM          bool ltrim( const string& key, const int start, const int end);
    // RPOP           bool rpop( const string& key, string& value);
    // RPOPLPUSH      bool rpoplpush( const string& key_src, const string& key_dest, string& value);
    // RPUSH

    bool RPush(const TKey& key, const TValue& value, int64_t& length) {
        return CommandInteger(length, "RPUSH %s %s", key.c_str(), value.c_str());
    }

    // RPUSHX         bool rpushx( const string& key, const string& value, int64_t& length);

    void AddParam(TValues& vDes, const TValues& vSrc) {
        for (TValues::const_iterator iter = vSrc.begin(); iter != vSrc.end(); ++iter) {
            vDes.push_back(*iter);
        }
    }

    //Set(集合)
    // 	bool SAdd(const TKey& key, const TValues& vValue, int64_t& count)
    // 	{
    // 		TValues vCmdData;
    // 		vCmdData.push_back("SADD");
    // 		vCmdData.push_back(key);
    // 		AddParam(vCmdData, vValue);
    // 		return CommandArgv(vCmdData, count);
    // 	}

    bool SAdd(const TKey& key, const TValue& value) {
        int64_t retval;
        bool bOK = CommandInteger(retval, "SADD %s %b", key.c_str(), value.c_str(), value.length());
        return bOK && retval == 1;
    }

    bool SCard(const TKey& key, int64_t& nCount) {
        return CommandInteger(nCount, "SCARD %s", key.c_str());
    }
    // SDIFF          bool sdiff(const DBIArray& dbi, const KEYS& vKkey, VALUES& vValue);
    // SDIFFSTORE     bool sdiffstore( const KEY& destinationkey, const DBIArray& vdbi, const KEYS& vkey, int64_t& count);
    // SINTER         bool sinter(const DBIArray& dbi, const KEYS& vkey, VALUES& vValue);
    // SINTERSTORE    bool sinterstore( const KEY& destinationkey, const DBIArray& vdbi, const KEYS& vkey, int64_t& count);
    // SISMEMBER      bool sismember( const KEY& key, const VALUE& member);
    // SMEMBERS       bool smembers( const KEY& key, VALUES& vValue);
    bool SMembers(const TKey& key, TValues& values) {
        return CommandArray(values, "SMEMBERS %s", key.c_str());
    }
    // SMOVE          bool smove( const KEY& srckey, const KEY& deskey, const VALUE& member);
    bool SPop(const TKey& key, TValue& value) {
        return CommandString(value, "SPOP %s", key.c_str());
    }
    // SRANDMEMBER    bool srandmember( const KEY& key, VALUES& vmember, int num = 0);
    // SREM           bool srem( const KEY& key, const VALUES& vmembers, int64_t& count);
    // SSCAN
    // SUNION         bool sunion(const DBIArray& dbi, const KEYS& vkey, VALUES& vValue);
    // SUNIONSTORE    bool sunionstore( const KEY& deskey, const DBIArray& vdbi, const KEYS& vkey, int64_t& count);

    // ZADD
    bool ZAdd(const TKey& key, const TField& value, const int32_t& score) {
        int64_t retval = 0;
        bool bOK = CommandInteger(retval, "ZADD %s %d %s", key.c_str(), score, value.c_str());
        return bOK;
    }

    // ZCARD
    bool ZCard(const TKey& key, int64_t* num) {
        return CommandInteger(*num, "ZCARD %s", key.c_str());
    }

    bool ZCount(const TKey& key, int32_t begin, int32_t end, int64_t* count) {
        int64_t retval = 0;
        bool bOK = CommandInteger(retval, "ZCOUNT %s %d %d", key.c_str(), begin, end);
        *count = retval;
        return bOK;
    }
    // ZINCRBY            bool zincrby( const string& key, const double &increment, const string& member, string& value);
    // ZINTERSTORE
    // ZRANGE
    bool ZRangeWithScore(const TKey& key, int32_t start, int32_t end, TSSet* vValues) {
        bool bOK = CommandSSet(*vValues, "ZRANGE %s %d %d WITHSCORES", key.c_str(), start, end);
        return bOK;
    }
    // ZRANGEBYSCORE
    // ZRANK             bool ZRank( const KEY& key, const FILED& member, int64_t &rank);
    // ZREM

    bool ZRem(const TKey& key, const TField& field) {
        int64_t nCount = 0;
        bool bOK = CommandInteger(nCount, "ZREM %s %s", key.c_str(), field.c_str());
        return bOK;
    }
    // ZREMRANGEBYRANK
    bool ZRemRangeByRank(const TKey& key, int32_t nBegin, int32_t nEnd, int64_t* num) {
        bool bOK = CommandInteger(*num, "ZREMRANGEBYRANK %s %d %d", key.c_str(), nBegin, nEnd);
        return bOK;
    }
    // ZREMRANGEBYSCORE
    bool ZRemRangeByScore(const TKey& key, int32_t nBegin, int32_t nEnd, int64_t* num) {
        bool bOK = CommandInteger(*num, "ZREMRANGEBYSCORE %s %d %d", key.c_str(), nBegin, nEnd);
        return bOK;
    }
    // ZREVRANGE
    bool ZRevrangeWithScore(const TKey& key, int start, int end, TSSet* vValues) {
        auto bOK = CommandSSet(*vValues, "ZREVRANGE %s %d %d WITHSCORES", key.c_str(), start, end);
        return bOK;
    }

    bool ZRevrange(const TKey& key, int start, int end, TValues* vValues) {
        auto bOK = CommandArray(*vValues, "ZREVRANGE %s %d %d", key.c_str(), start, end);
        return bOK;
    }
    // ZREVRANGEBYSCORE
    // ZREVRANK           bool zrevrank( const string& key, const string &member, int64_t& rank);
    // ZSCAN
    // ZSCORE
    bool Zscore(const TKey& key, const TField& member, TValue& score) {
        auto bOK = CommandString(score, "ZSCORE %s %s", key.c_str(), member.c_str());
        return bOK;
    }
    // ZUNIONSTORE

    // PSUBSCRIBE
    // PUBLISH
    // PUBSUB
    // PUNSUBSCRIBE
    // SUBSCRIBE
    // UNSUBSCRIBE

    // DISCARD
    // EXEC
    // MULTI
    // UNWATCH
    // WATCH
    template <typename... Args>
    bool CommandBool(const char* szFmt, Args... args) {
        bool bRet = false;
        auto pReply = CommandWrap(szFmt, std::forward<Args>(args)...);
        if (CheckReply(pReply)) {
            if (REDIS_REPLY_STATUS == pReply->type) {
                bRet = true;
            } else {
                bRet = (pReply->integer == 1) ? true : false;
            }
        } else {
            SetErrInfo(pReply);
        }
        return bRet;
    }

    template <typename... Args>
    bool CommandInteger(int64_t& nIntval, const char* szFmt, Args... args) {
        bool bRet = false;
        auto pReply = CommandWrap(szFmt, args...);
        if (CheckReply(pReply)) {
            nIntval = pReply->integer;
            bRet = true;
        } else {
            SetErrInfo(pReply);
        }
        return bRet;
    }

    template <typename... Args>
    bool CommandString(TValue& strData, const char* szFmt, Args... args) {
        bool bRet = false;
        auto pReply = CommandWrap(szFmt, args...);
        if (CheckReply(pReply)) {
            strData.assign(pReply->str, pReply->len);
            bRet = true;
        } else {
            SetErrInfo(pReply);
        }
        return bRet;
    }

    template <typename... Args>
    RedisReplyPtr CommandWrap(const char* szFmt, Args... args) {
        if (!_pCtx) {
            return nullptr;
        }
        void* pCommand = redisCommand(_pCtx, szFmt, args...);
        if (!pCommand) {
            SetErrInfo(std::string(_pCtx->errstr));
            return nullptr;
        }
        redisReply* reply = static_cast<redisReply*>(pCommand);
        RedisReplyPtr pRet(reply, FreeReply);
        return pRet;
    }

    // 	bool CommandArgv(const TValues& vDataIn, int64_t &nRetval)
    // 	{
    // 		bool bRet = false;
    // 		std::vector<const char*> argv(vDataIn.size());
    // 		std::vector<size_t> argvlen(vDataIn.size());
    // 		unsigned int j = 0;
    //
    // 		for(TValues::const_iterator iter = vDataIn.begin(); iter != vDataIn.end(); ++iter, ++j) {
    // 			argv[j] = iter->c_str(), argvlen[j] = iter->size();
    // 		}
    //
    // 		redisReply *reply = static_cast<redisReply *>(redisCommandArgv(_pCtx, vDataIn.size(), &(argv[0]), &(argvlen[0])));
    //
    // 		if(CheckReply(reply)) {
    // 			nRetval = reply->integer;
    // 			bRet = true;
    // 		}
    // 		else {
    // 			SetErrInfo(reply);
    // 		}
    // 		FreeReply(reply);
    // 		return bRet;
    // 	}

    template <typename... Args>
    bool CommandHash(THash& ret, const char* szFmt, Args... args) {
        bool bRet = false;
        auto pReply = CommandWrap(szFmt, args...);
        if (CheckReply(pReply) && (pReply->type == REDIS_REPLY_ARRAY)) {
            bRet = true;
            for (size_t i = 0; i + 1 < pReply->elements; i += 2) {
                redisReply* pKey = pReply->element[i];
                redisReply* pValue = pReply->element[i + 1];
                TKey strKey(pKey->str, pKey->len);
                auto strValue = std::make_shared<TValue>(pValue->str, pValue->len);
                ret.insert(std::make_pair(strKey, strValue));
            }
        } else {
            SetErrInfo(pReply);
        }
        return bRet;
    }

    template <typename... Args>
    bool CommandArray(TValues& ret, const char* szFmt, Args... args) {
        bool bRet = false;
        auto pReply = CommandWrap(szFmt, args...);
        if (CheckReply(pReply) && (pReply->type == REDIS_REPLY_ARRAY)) {
            bRet = true;
            for (size_t i = 0; i < pReply->elements; i++) {
                redisReply* pValue = pReply->element[i];
                auto strValue = std::make_shared<TValue>(pValue->str, pValue->len);
                ret.push_back(strValue);
            }
        } else {
            SetErrInfo(pReply);
        }
        return bRet;
    }

#ifndef WIN32
#define _atoi64(val) strtoll(val, NULL, 10)
#endif

    template <typename... Args>
    bool CommandSSet(TSSet& ret, const char* szFmt, Args... args) {
        bool bRet = false;
        auto pReply = CommandWrap(szFmt, args...);
        if (CheckReply(pReply) && (pReply->type == REDIS_REPLY_ARRAY)) {
            bRet = true;
            for (size_t i = 0; i + 1 < pReply->elements; i += 2) {
                auto pValue = pReply->element[i];
                auto pScore = pReply->element[i + 1];
                auto pItem = std::make_shared<TSSetValue>();
                pItem->key.assign(pValue->str, pValue->len);
#ifdef WIN32
                pItem->score = ::_atoi64(pScore->str);
#else
                pItem->score = ::strtoll(pScore->str, NULL, 10);
#endif
                ret.push_back(pItem);
            }
        } else {
            SetErrInfo(pReply);
        }
        return bRet;
    }

    static void FreeReply(const redisReply* reply) {
        if (NULL != reply) {
            freeReplyObject((void*)reply);
        }
    }

#define CONNECT_CLOSED_ERROR "redis connection be closed"

    void SetErrInfo(RedisReplyPtr pReply) {
        if (!pReply) {
            SetErrInfo(CONNECT_CLOSED_ERROR);
        } else {
            SetErrInfo(std::string(pReply->str, pReply->len));
        }
    }

    void SetErrInfo(redisReply* p) {
        if (NULL == p) {
            SetErrInfo(CONNECT_CLOSED_ERROR);
        } else {
            redisReply* reply = (redisReply*)p;
            SetErrInfo(std::string(reply->str, reply->len));
        }
    }

    void SetErrInfo(const std::string& strerr) {
        if (!strerr.empty()) {
            _HIREDIS_LOG(strerr);
        }
    }

    // 	void SetErrMessage(const char* fmt, ...)
    // 	{
    // 		char szBuf[128] = {0};
    // 		va_list va;
    // 		va_start(va, fmt);
    // 		vsnprintf(szBuf, sizeof(szBuf), fmt, va);
    // 		va_end(va);
    // 		SetErrString(szBuf, ::strlen(szBuf));
    // 	}

    bool CheckReply(RedisReplyPtr pReply) {
        if (pReply) {
            return CheckReply(pReply.get());
        }
        return false;
    }

    /* TODO 如何封装命令
	redisCommand函数返回redisReply，我们需要通过判断它的type字段
	来知道返回了具体什么样的内容：
	REDIS_REPLY_STATUS      表示状态，内容通过str字段查看，字符串长度是len字段
	REDIS_REPLY_ERROR       表示出错，查看出错信息，如上的str,len字段
	REDIS_REPLY_INTEGER    返回整数，从integer字段获取值
	REDIS_REPLY_NIL            没有数据返回
	REDIS_REPLY_STRING      返回字符串，查看str,len字段
	REDIS_REPLY_ARRAY       返回一个数组，查看elements的值（数组个数），通过
	element[index]的方式访问数组元素，每个数组元素是一个redisReply对象的指针
	*/
    bool CheckReply(const redisReply* reply) {
        if (NULL == reply) {
            return false;
        }
        switch (reply->type) {
        case REDIS_REPLY_STRING: {
            return true;
        }
        case REDIS_REPLY_ARRAY: {
            return true;
        }
        case REDIS_REPLY_INTEGER: {
            return true;
        }
        case REDIS_REPLY_NIL: {
            return false;
        }
        case REDIS_REPLY_STATUS: {
#ifdef WIN32
            return (_stricmp(reply->str, "OK") == 0) ? true : false;
#else
            return (strcasecmp(reply->str, "OK") == 0) ? true : false;
#endif // WIN32
        }
        case REDIS_REPLY_ERROR: {
            return false;
        }
        default: {
            return false;
        }
        }
        return false;
    }

    std::string _strPass;
    unsigned int _nTimeout = 0; // connect timeout second
    redisContext* _pCtx = NULL;
    std::string _strHost;
    unsigned int _nPort = 0;
    //std::function<void(int, std::string)> _funcErrCall = nullptr;
};

}