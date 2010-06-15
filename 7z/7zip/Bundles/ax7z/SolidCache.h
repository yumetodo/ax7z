// SolidCache.h

#pragma once

#ifndef SOLIDCACHE_H
#define SOLIDCACHE_H

#define NOMINMAX
#include <windows.h>
#undef NOMINMAX
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <ctime>
#include <cassert>

#define __STDC_CONSTANT_MACROS
#include <boost/cstdint.hpp>
#define UINT64_S "I64"

#include <boost/function_types/result_type.hpp>
#include <boost/function_types/parameter_types.hpp>
#include <boost/function_types/function_arity.hpp>
#include <boost/mpl/at.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/call_traits.hpp>

#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing.hpp>

#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>

#ifdef NDEBUG
#define OutputDebugPrintf (void)
#else
static void OutputDebugPrintf(char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	char buf[2048];
	std::vsprintf(buf, format, ap);
	OutputDebugString(buf);
	va_end(ap);
}
#endif

struct sqlite3;

class SolidFileCacheDisk;

class SolidCacheDisk
{
// TODO: Bad design
	friend struct SolidCacheDiskCallback;
private:
	int m_nMaxDisk;
	int m_nPurgeDisk;
	std::string m_sCacheFolder;
	sqlite3* m_db;

#define BOOST_PP_ITERATION_PARAMS_1 (4, (0, 4, "Lock.h", 1))
#include BOOST_PP_ITERATE()

// private:
	void InitDB_();
	void CheckDB_();
	bool ExistsArchive_(const char* archive) const;
	void AddArchive_(const char* archive);
	unsigned int GetArchiveIdx_(const char* archive) const;
	bool ExistsEntry_(unsigned int aidx, unsigned int idx) const;
	void AppendEntry_(unsigned int aidx, unsigned int idx, const void* data, unsigned int size);
	void AddEntry_(unsigned int aidx, unsigned int idx, const void *data, unsigned int size);
	void PurgeUnreferenced_();
	void PurgeUnreferencedOther_(unsigned int aidx);
	void PurgeUnreferencedWithAIdx_(unsigned int aidx);
	void PurgeUnmarkedAll_();
	void PurgeUnmarkedOther_(unsigned int aidx);
	boost::uint64_t GetSize_() const;
	void ReduceSizeWithAIdx_(unsigned int aidx, boost::uint64_t size);
	void ReduceSize_(boost::uint64_t size, unsigned int exclude_aidx);
	bool IsProcessing_(const char* archive, unsigned int index) const;
// public:
	bool IsCached_(const char* archive, unsigned int index) const;
	bool Exists_(const char* archive, unsigned int index) const;
	void Append_(const char* archive, unsigned int index, const void* data, unsigned int size);
	void Cached_(const char* archive, unsigned int index);
	void CachedVector_(const char* archive, std::vector<unsigned int>& vIndex);
	void PurgeUnmarked_(const char *archive);
	void GetContent_(const char* archive, unsigned int index, void* dest, unsigned int size) const;
	void OutputContent_(const char* archive, unsigned int index, unsigned int size, FILE* fp) const;
	void AccessArchive_(const char* archive);
	void Clear_();

//
	std::string GetFileName(__int64 id) const;
public:
	SolidCacheDisk():m_nMaxDisk(-1),m_nPurgeDisk(10),m_sCacheFolder(""),m_db(0) {}
	~SolidCacheDisk();
	bool IsCached(const char* archive, unsigned int index) const
	{
		return CallWithSharedLock(&SolidCacheDisk::IsCached_, archive, index);
	}
	bool Exists(const char* archive, unsigned int index) const
	{
		return CallWithSharedLock(&SolidCacheDisk::Exists_, archive, index);
	}
	void Append(const char* archive, unsigned int index, const void* data, unsigned int size)
	{
		return CallWithLockGuard(&SolidCacheDisk::Append_, archive, index, data, size);
	}
	void Cached(const char* archive, unsigned int index)
	{
		return CallWithLockGuard(&SolidCacheDisk::Cached_, archive, index);
	}
	void CachedVector(const char* archive, std::vector<unsigned int>& vIndex)
	{
		return CallWithLockGuard(&SolidCacheDisk::CachedVector_, archive, vIndex);
	}
	void PurgeUnmarked(const char *archive)
	{
		return CallWithLockGuard(&SolidCacheDisk::PurgeUnmarked_, archive);
	}
	void GetContent(const char* archive, unsigned int index, void* dest, unsigned int size) const
	{
		return CallWithSharedLock(&SolidCacheDisk::GetContent_, archive, index, dest, size);
	}
	void OutputContent(const char* archive, unsigned int index, unsigned int size, FILE* fp) const
	{
		return CallWithSharedLock(&SolidCacheDisk::OutputContent_, archive, index, size, fp);
	}
	void AccessArchive(const char* archive)
	{
		return CallWithLockGuard(&SolidCacheDisk::AccessArchive_, archive);
	}
	void Clear()
	{
		return CallWithLockGuard(&SolidCacheDisk::Clear_);
	}

	static SolidCacheDisk& GetInstance();
	static SolidFileCacheDisk GetFileCache(const std::string& filename);

	int GetMaxDisk() const { return m_nMaxDisk; }
	boost::uint64_t GetMaxDiskInBytes() const { return m_nMaxDisk * UINT64_C(1024)*UINT64_C(1024); }
	int SetMaxDisk(int nNew)
	{
		int nOld = m_nMaxDisk;
		m_nMaxDisk = nNew;
		return nOld;
	}
	int GetPurgeDisk() const { return m_nPurgeDisk; }
	boost::uint64_t GetPurgeDiskInBytes() const { return m_nPurgeDisk * UINT64_C(1024)*UINT64_C(1024); }
	int SetPurgeDisk(int nNew)
	{
		int nOld = m_nPurgeDisk;
		m_nPurgeDisk = nNew;
		return nOld;
	}
	const std::string& GetCacheFolder() const { return m_sCacheFolder; }
	std::string SetCacheFolder(std::string sNew);
};

class SolidFileCacheDisk
{
	friend class SolidCacheDisk;
private:
	SolidCacheDisk &m_scd;
	std::string m_sArchive;
	UINT32 m_nMaxNum;
	SolidFileCacheDisk(SolidCacheDisk &scd, const std::string &sArchive)
	    : m_scd(scd), m_sArchive(sArchive)
	{
	}
public:
	bool IsCached(unsigned int index) const
	{
		return m_scd.IsCached(m_sArchive.c_str(), index);
	}
	bool Exists(unsigned int index) const
	{
		return m_scd.Exists(m_sArchive.c_str(), index);
	}
	void Append(unsigned int index, const void* data, unsigned int size)
	{
		m_scd.Append(m_sArchive.c_str(), index, data, size);
	}
	void Cached(unsigned int index)
	{
		m_scd.Cached(m_sArchive.c_str(), index);
	}
	void CachedVector(std::vector<unsigned int>& vIndex)
	{
		m_scd.CachedVector(m_sArchive.c_str(), vIndex);
	}
	void PurgeUnmarked()
	{
		m_scd.PurgeUnmarked(m_sArchive.c_str());
	}
	void GetContent(unsigned int index, void* dest, unsigned int size) const
	{
		m_scd.AccessArchive(m_sArchive.c_str());
		m_scd.GetContent(m_sArchive.c_str(), index, dest, size);
	}
	void OutputContent(unsigned int index, unsigned int size, FILE* fp) const
	{
		m_scd.AccessArchive(m_sArchive.c_str());
		m_scd.OutputContent(m_sArchive.c_str(), index, size, fp);
	}
};

class SolidFileCacheMemory
{
	friend class SolidCacheMemory;

#define BOOST_PP_ITERATION_PARAMS_1 (4, (0, 3, "Lock.h", 2))
#include BOOST_PP_ITERATE()

// private:
	void AccessArchive_() const;
// public:
	boost::uint64_t ReduceSize_(boost::uint64_t uiSize, void(*fCallback)(void*, const std::string&, unsigned int, void*, unsigned int, bool), void* pArg);
	void Append_(unsigned int index, const void* data, unsigned int size)
	{
		m_cache[index].vBuffer.insert(m_cache[index].vBuffer.end(), 
			static_cast<const unsigned char*>(data), 
			static_cast<const unsigned char*>(data)+size);
		OutputDebugPrintf("SolidCacheMemory::Append: %d %p %d %d, %02X %02X %02X %02X", index, data, size, m_cache[index].vBuffer.size(), m_cache[index].vBuffer[0], m_cache[index].vBuffer[1], m_cache[index].vBuffer[2], m_cache[index].vBuffer[3]);
	}
	void Cached_(unsigned int index)
	{
		m_cache[index].fCached = true;
//		OutputDebugPrintf("SolidCache::Cached: %d", index);
	}
	void CachedVector_(std::vector<unsigned int>& vIndex)
	{
		std::vector<unsigned int>::iterator it = vIndex.begin(), itEnd = vIndex.end();
		for(; it != itEnd; ++it) {
			if(m_cache.count(*it)) {
				OutputDebugPrintf("SolidCacheMemory::CachedVector %d", *it);
				m_cache[*it].fCached = true;
			}
		}
	}
	void Remove_(unsigned int index)
	{
		m_cache.erase(index);
	}
	void PurgeUnmarked_()
	{
		FileCache::iterator it = m_cache.begin(), itEnd = m_cache.end();
		while(it != itEnd) {
			if(!it->second.fCached) {
				it = m_cache.erase(it);
			} else 
				++it;
		}
	}
	unsigned int GetCount_() const
	{
		return m_cache.size();
	}
	unsigned int GetCurSize_(unsigned int index) const
	{
		return m_cache[index].vBuffer.size();
	}
	const void* GetContent_(unsigned int index) const
	{
		return &m_cache[index].vBuffer[0];
	}
	void GetContent_(unsigned int index, void* dest, unsigned int size) const
	{
		OutputDebugPrintf("SolidCacheMemory::GetContent: %d %p %d %d %02X %02X %02X %02X", index, dest, size, m_cache[index].vBuffer.size(), m_cache[index].vBuffer[0], m_cache[index].vBuffer[1], m_cache[index].vBuffer[2], m_cache[index].vBuffer[3]);
		// TODO: error check
		CopyMemory(dest, &m_cache[index].vBuffer[0], std::min(size, m_cache[index].vBuffer.size()));
		AccessArchive_();
	}
	void OutputContent_(unsigned int index, unsigned int size, FILE* fp) const
	{
		OutputDebugPrintf("SolidCacheMemory::OutputContent: %d %p %d %d %02X %02X %02X %02X", index, fp, size, m_cache[index].vBuffer.size(), m_cache[index].vBuffer[0], m_cache[index].vBuffer[1], m_cache[index].vBuffer[2], m_cache[index].vBuffer[3]);
		fwrite(&m_cache[index].vBuffer[0], std::min(size, m_cache[index].vBuffer.size()), 1, fp);
		AccessArchive_();
	}

private:
	struct Entry
	{
		bool fCached;
		std::vector<unsigned char> vBuffer;
		Entry():fCached(false) {}
	};
	typedef std::map<unsigned int, Entry> FileCache;
	SolidFileCacheMemory(FileCache& cache, std::time_t &atime) : m_cache(cache), m_atime(atime)
	{
	}
	void AccessArchive() const
	{
		return CallWithLockGuard(&SolidFileCacheMemory::AccessArchive_);
	}
	FileCache& m_cache;
	mutable std::time_t& m_atime;
public:
	bool IsCached_(unsigned int index) const
	{
		return m_cache.count(index) > 0 && m_cache[index].fCached;
	}
	bool Exists_(unsigned int index) const
	{
		return m_cache.count(index) > 0;
	}
	void Append(unsigned int index, const void* data, unsigned int size)
	{
		return CallWithLockGuard(&SolidFileCacheMemory::Append_, index, data, size);
	}
	void Cached(unsigned int index)
	{
		return CallWithLockGuard(&SolidFileCacheMemory::Cached_, index);
	}
	void CachedVector(std::vector<unsigned int>& vIndex)
	{
		return CallWithLockGuard(&SolidFileCacheMemory::CachedVector_, vIndex);
	}
	void Remove(unsigned int index)
	{
		return CallWithLockGuard(&SolidFileCacheMemory::Remove_, index);
	}
	void PurgeUnmarked()
	{
		return CallWithLockGuard(&SolidFileCacheMemory::PurgeUnmarked_);
	}
//	unsigned int ReduceSize(unsigned int uiSize, void(*fCallback)(void*, const std::string&, unsigned int, void*, unsigned int, bool), void* pArg);
	unsigned int GetCount() const
	{
		return CallWithSharedLock(&SolidFileCacheMemory::GetCount_);
	}
	unsigned int GetCurSize(unsigned int index) const
	{
		return CallWithSharedLock(&SolidFileCacheMemory::GetCurSize_, index);
	}
	const void* GetContent(unsigned int index) const
	{
		return CallWithSharedLock(static_cast<const void* (SolidFileCacheMemory::*)(unsigned int) const>(&SolidFileCacheMemory::GetContent_), index);
	}
	void GetContent(unsigned int index, void* dest, unsigned int size) const
	{
		return CallWithSharedLock(static_cast<void (SolidFileCacheMemory::*)(unsigned int, void*, unsigned int) const>(&SolidFileCacheMemory::GetContent_), index, dest, size);
	}
	void OutputContent(unsigned int index, unsigned int size, FILE* fp) const
	{
		return CallWithSharedLock(&SolidFileCacheMemory::OutputContent_, index, size, fp);
	}
};

class SolidCacheMemory
{
private:
	typedef std::map<std::string, SolidFileCacheMemory::FileCache> Table;
	typedef std::map<std::string, std::time_t> ArchiveTable;
	Table m_mTable;
	ArchiveTable m_mAccess;
	int m_nMaxMemory;
	int m_nPurgeMemory;

	std::string MakeKey(const std::string &sArchive) const;
	std::string RestoreKey(const std::string &sArchive) const
	{
		return sArchive.substr(0, sArchive.find_last_of(':', sArchive.find_last_of(':') - 1));
	}

#define BOOST_PP_ITERATION_PARAMS_1 (4, (0, 4, "Lock.h", 3))
#include BOOST_PP_ITERATE()

// private:
	void AccessArchive_(const char* archive);
	std::string GetLRUArchive_(const std::string& sExcludeArchive) const;

// public:
	boost::uint64_t GetSize_() const;
	boost::uint64_t ReduceSize_(boost::uint64_t uiSize, void(*)(void*, const std::string&, unsigned int, void*, unsigned int, bool), void* pArg, const std::string &sExcludeArchive);
	void Clear_()
	{
		m_mTable.clear();
		m_mAccess.clear();
	}
	void PurgeUnreferenced_()
	{
		Table::iterator it = m_mTable.begin(), itEnd = m_mTable.end();
		while(it != itEnd) {
			if(it->second.size() == 0) {
				it = m_mTable.erase(it);
			} else ++it;
		}
	}
	bool Exists_(const std::string &sArchive) const
	{
		return m_mTable.count(sArchive) > 0;
	}
	bool Peek_(bool (SolidFileCacheMemory::*method)(unsigned int) const, const std::string& sArchive, unsigned int index) const
	{
		Table::const_iterator it = m_mTable.find(sArchive);
		ArchiveTable::const_iterator it2 = m_mAccess.find(sArchive);
		if(it != m_mTable.end()) {
			return (SolidFileCacheMemory(const_cast<Table::mapped_type&>(it->second), const_cast<ArchiveTable::mapped_type&>(it2->second)).*method)(index);
		}
		return false;
	}
	SolidFileCacheMemory GetFileCache_(const std::string& sArchive)
	{
		return SolidFileCacheMemory(m_mTable[sArchive], m_mAccess[sArchive]);
	}
	SolidFileCacheMemory GetFileCache_(const std::string& sArchive) const
	{
		Table::const_iterator it = m_mTable.find(sArchive);
		ArchiveTable::const_iterator it2 = m_mAccess.find(sArchive);
		if(it == m_mTable.end() || it2 == m_mAccess.end()) throw std::domain_error("SolidCacheMemory::GetFileCache() const: not initialized");
		return SolidFileCacheMemory(const_cast<Table::mapped_type&>(it->second), const_cast<ArchiveTable::mapped_type&>(it2->second));
	}

//
//	void AccessArchive(const char* archive);
//	std::string GetLRUArchive(const std::string& sExcludeArchive) const;

public:
	SolidCacheMemory():m_nMaxMemory(-1),m_nPurgeMemory(10) {}
	SolidFileCacheMemory GetFileCache(const std::string& sArchive)
	{
		return SolidFileCacheMemory(m_mTable[MakeKey(sArchive)], m_mAccess[MakeKey(sArchive)]);
	}
	SolidFileCacheMemory GetFileCache(const std::string& sArchive) const
	{
		Table::const_iterator it = m_mTable.find(MakeKey(sArchive));
		ArchiveTable::const_iterator it2 = m_mAccess.find(MakeKey(sArchive));
		if(it == m_mTable.end() || it2 == m_mAccess.end()) throw std::domain_error("SolidCacheMemory::GetFileCache() const: not initialized");
		return SolidFileCacheMemory(const_cast<Table::mapped_type&>(it->second), const_cast<ArchiveTable::mapped_type&>(it2->second));
	}
	bool Peek(bool (SolidFileCacheMemory::*method)(unsigned int) const, const std::string& sArchive, unsigned int index) const
	{
		return CallWithSharedLock(&SolidCacheMemory::Peek_, method, MakeKey(sArchive), index);
	}
	void PurgeUnreferenced()
	{
		return CallWithLockGuard(&SolidCacheMemory::PurgeUnreferenced_);
	}
	bool Exists(const std::string &sArchive) const
	{
		return CallWithSharedLock(&SolidCacheMemory::Exists_, MakeKey(sArchive));
	}
	boost::uint64_t GetSize() const
	{
		return CallWithSharedLock(&SolidCacheMemory::GetSize_);
	}
	boost::uint64_t ReduceSize(boost::uint64_t uiSize, void(*f)(void*, const std::string&, unsigned int, void*, unsigned int, bool), void* pArg, const std::string& sExcludeArchive)
	{
		return CallWithLockGuard(&SolidCacheMemory::ReduceSize_, uiSize, f, pArg, MakeKey(sExcludeArchive));
	}
	void Clear()
	{
		return CallWithLockGuard(&SolidCacheMemory::Clear_);
	}

	int GetMaxMemory() const { return m_nMaxMemory; }
	boost::uint64_t GetMaxMemoryInBytes() const { return m_nMaxMemory * UINT64_C(1024)*UINT64_C(1024); }
	int SetMaxMemory(int nNew)
	{
		int nOld = m_nMaxMemory;
		m_nMaxMemory = nNew;
		return nOld;
	}
	int GetPurgeMemory() const { return m_nPurgeMemory; }
	boost::uint64_t GetPurgeMemoryInBytes() const { return m_nPurgeMemory * UINT64_C(1024)*UINT64_C(1024); }
	int SetPurgeMemory(int nNew)
	{
		int nOld = m_nPurgeMemory;
		m_nPurgeMemory = nNew;
		return nOld;
	}
};

class SolidFileCache;

class SolidCache
{
private:
	int m_nMaxLookAhead;
	SolidCacheDisk m_scd;
	SolidCacheMemory m_scm;
	boost::shared_mutex m_sm;
	SolidCache() : m_nMaxLookAhead(-1) {}
public:
	static SolidCache& GetInstance();
	static SolidFileCache GetFileCache(const std::string& filename);
	typedef boost::shared_mutex Mutex;
	static Mutex& GetMutex(void)
	{
		return GetInstance().m_sm;
	}

	bool IsCached(const std::string& sArchive, unsigned int index) const
	{
		return m_scm.Peek(SolidFileCacheMemory::IsCached_, sArchive, index) || m_scd.IsCached(sArchive.c_str(), index);
	}
	void Append(const std::string& sArchvie, unsigned int index, const void* data, unsigned int size);
	void Cached(const std::string& sArchive, unsigned int index)
	{
		OutputDebugPrintf("SolidCache::Cached %s %d", sArchive.c_str(), index);
		if(m_scm.Peek(SolidFileCacheMemory::Exists_, sArchive, index)) {
			OutputDebugPrintf("SolidCache::Cached:memory %s %d", sArchive.c_str(), index);
			m_scm.GetFileCache(sArchive).Cached(index);
		} else if(m_scd.Exists(sArchive.c_str(), index)) {
			OutputDebugPrintf("SolidCache::Cached:disk %s %d", sArchive.c_str(), index);
			m_scd.Cached(sArchive.c_str(), index);
		} else {
			assert("SolidCache::Cached: Not reached");
		}
	}
	void CachedVector(const std::string& sArchive, std::vector<unsigned int>& vIndex)
	{
		if(m_scm.Exists(sArchive)) {
			m_scm.GetFileCache(sArchive).CachedVector(vIndex);
		}
		m_scd.CachedVector(sArchive.c_str(), vIndex);
	}
	void PurgeUnmarked(const std::string& sArchive)
	{
		m_scm.GetFileCache(sArchive).PurgeUnmarked();
		m_scm.PurgeUnreferenced();
		m_scd.PurgeUnmarked(sArchive.c_str());
	}
	void GetContent(const std::string& sArchive, unsigned int index, void* dest, unsigned int size) const
	{
		OutputDebugPrintf("SolidCache::GetContent %s %d %d bytes", sArchive.c_str(), index, size);
		if(m_scm.Peek(SolidFileCacheMemory::IsCached_, sArchive, index)) {
			m_scm.GetFileCache(sArchive).GetContent(index, dest, size);
		} else if(m_scd.IsCached(sArchive.c_str(), index)) {
			m_scd.GetContent(sArchive.c_str(), index, dest, size);
		} else {
			assert("SolidCache::GetContent: Not reached");
		}
	}
	void OutputContent(const std::string& sArchive, unsigned int index, unsigned int size, FILE* fp) const
	{
		OutputDebugPrintf("SolidCache::OutputContent %s %d %d bytes", sArchive.c_str(), index, size);
		if(m_scm.Peek(SolidFileCacheMemory::IsCached_, sArchive, index)) {
			m_scm.GetFileCache(sArchive).OutputContent(index, size, fp);
		} else if(m_scd.IsCached(sArchive.c_str(), index)) {
			m_scd.OutputContent(sArchive.c_str(), index, size, fp);
		} else {
			assert("SolidCache::OutputContent: Not reached");
		}
	}

	void Clear()
	{
		m_scm.Clear();
		m_scd.Clear();
	}

	int GetMaxLookAhead() const { return m_nMaxLookAhead; }
	int SetMaxLookAhead(int nNew)
	{
		int nOld = m_nMaxLookAhead;
		m_nMaxLookAhead = nNew;
		return nOld;
	}
	int GetMaxMemory() const { return m_scm.GetMaxMemory(); }
	boost::uint64_t GetMaxMemoryInBytes() const { return m_scm.GetMaxMemoryInBytes(); }
	int SetMaxMemory(int nNew) { return m_scm.SetMaxMemory(nNew); }
	int GetPurgeMemory() const { return m_scm.GetPurgeMemory(); }
	boost::uint64_t GetPurgeMemoryInBytes() const { return m_scm.GetPurgeMemoryInBytes(); }
	int SetPurgeMemory(int nNew) { return m_scm.SetPurgeMemory(nNew); }
	int GetMaxDisk() const { return m_scd.GetMaxDisk(); }
	boost::uint64_t GetMaxDiskInBytes() const { return m_scd.GetMaxDiskInBytes(); }
	int SetMaxDisk(int nNew) { return m_scd.SetMaxDisk(nNew); }
	int GetPurgeDisk() const { return m_scd.GetPurgeDisk(); }
	boost::uint64_t GetPurgeDiskInBytes() const { return m_scd.GetPurgeDiskInBytes(); }
	int SetPurgeDisk(int nNew) { return m_scd.SetPurgeDisk(nNew); }
	const std::string& GetCacheFolder() const { return m_scd.GetCacheFolder(); }
	std::string SetCacheFolder(std::string sNew) { return m_scd.SetCacheFolder(sNew); }
};

class SolidFileCache
{
	friend class SolidCache;
private:
	SolidCache &m_sc;
	std::string m_sArchive;
	UINT32 m_nMaxNum;
	SolidFileCache(SolidCache &sc, const std::string &sArchive) : m_sc(sc), m_sArchive(sArchive) {}
public:
	bool IsCached(unsigned int index) const { return m_sc.IsCached(m_sArchive, index); }
	void Append(unsigned int index, const void* data, unsigned int size)
	{
		m_sc.Append(m_sArchive, index, data, size);
	}
	void Cached(unsigned int index) { m_sc.Cached(m_sArchive, index); }
	void CachedVector(std::vector<unsigned int>& vIndex) { m_sc.CachedVector(m_sArchive, vIndex); }
	void PurgeUnmarked() { m_sc.PurgeUnmarked(m_sArchive); }
	void GetContent(unsigned int index, void* dest, unsigned int size) const
	{
		m_sc.GetContent(m_sArchive, index, dest, size);
	}
	void OutputContent(unsigned int index, unsigned int size, FILE* fp) const
	{
		m_sc.OutputContent(m_sArchive, index, size, fp);
	}

	void GetExtractVector(std::vector<UINT32> &v, UINT32 index, UINT32 num, UINT32 max_num)
	{
// Need to co-operate with Append() / Cached()
//		UINT32 start = m_cache.size() ? std::min((--m_cache.end())->first, index) : 0;
//		UINT32 end = std::min(std::max(start + num, index), max_num - 1);
		UINT32 start = 0;
		m_nMaxNum = max_num;
		UINT32 end = m_nMaxNum - 1;
		v.resize(end - start + 1);
		for(UINT32 i = start; i <= end; ++i) v[i - start] = i;
	}
	int GetProgress(UINT32 index)
	{
		return index;
	}
	int GetProgressDenom(UINT32 index)
	{
		return m_nMaxNum;
	}
};

#endif