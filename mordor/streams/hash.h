#ifndef __MORDOR_HASH_STREAM_H__
#define __MORDOR_HASH_STREAM_H__
// Copyright (c) 2009 - Mozy, Inc.

#include <vector>

#include <openssl/sha.h>
#include <openssl/md5.h>

#include "assert.h"
#include "filter.h"

namespace Mordor {

class HashStream : public FilterStream
{
public:
    typedef boost::shared_ptr<HashStream> ptr;

public:
    HashStream(Stream::ptr parent, bool own = true)
        : FilterStream(parent, own)
    {}

    bool supportsSeek() { return false; }
    bool supportsTruncate() { return false; }
    bool supportsUnread() { return false; }

    size_t read(Buffer &buffer, size_t length);
    size_t read(void *buffer, size_t length);
    size_t write(const Buffer &buffer, size_t length);
    size_t write(const void *buffer, size_t length);
    long long seek(long long offset, Anchor anchor = BEGIN);

    // Returns the hash in *binary*
    virtual std::string hash() const;
    virtual size_t hashSize() const = 0;
    virtual void hash(void *result, size_t length) const = 0;

    virtual void reset() = 0;

protected:
    virtual void updateHash(const void *buffer, size_t length) = 0;
};

class SHA0Stream : public HashStream
{
public:
    SHA0Stream(Stream::ptr parent, bool own = true);

    size_t hashSize() const;
    void hash(void *result, size_t length) const;
    void reset();

protected:
    void updateHash(const void *buffer, size_t length);

private:
    SHA_CTX m_ctx;
};

class SHA1Stream : public HashStream
{
public:
    SHA1Stream(Stream::ptr parent, bool own = true);

    size_t hashSize() const;
    void hash(void *result, size_t length) const;
    void reset();

protected:
    void updateHash(const void *buffer, size_t length);

private:
    SHA_CTX m_ctx;
};

class MD5Stream : public HashStream
{
public:
    MD5Stream(Stream::ptr parent, bool own = true);

    size_t hashSize() const;
    void hash(void *result, size_t length) const;
    void reset();

protected:
    void updateHash(const void *buffer, size_t length);

private:
    MD5_CTX m_ctx;
};

class CRC32Stream : public HashStream
{
public:
    /// Using a well-known polynomial will automatically select a precomputed table
    enum WellknownPolynomial
    {
        /// CRC-32-IEEE, used in V.42, Ethernet, MPEG-2, PNG, POSIX cksum, etc.
        IEEE = 0x04C11DB7,
        /// CRC-32C used in iSCSI and SCTP
        CASTAGNOLI = 0x1EDC6F41,
        /// CRC-32K
        KOOPMAN = 0x741B8CD7
    };
public:
    CRC32Stream(Stream::ptr parent, unsigned int polynomial = IEEE,
        bool own = true);
    CRC32Stream(Stream::ptr parent, const unsigned int *precomputedTable,
        bool own = true);

    static std::vector<unsigned int> precomputeTable(unsigned int polynomial);

    size_t hashSize() const;
    void hash(void *result, size_t length) const;
    void reset();

protected:
    void updateHash(const void *buffer, size_t length);

private:
    unsigned int m_crc;
    const std::vector<unsigned int> m_tableStorage;
    const unsigned int *m_table;
};

}

#endif
