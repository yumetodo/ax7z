#include <assert.h>
#include "SolidCache.h"

SolidCache& SolidCache::GetInstance()
{
	static SolidCache sc;
	return sc;
}

SolidFileCache SolidCache::GetFileCache(const std::string& filename)
{
	return SolidFileCache(GetInstance(), filename);
}

// TODO: probably, this operation itself should be moved to SolidCacheDisk
struct SolidCacheDiskCallback
{
	static void ReduceSizeCallback(void *pArg, const std::string& sArchive, unsigned int index, void* data, unsigned int size, bool flag)
	{
		SolidCacheDisk *pscd = static_cast<SolidCacheDisk*>(pArg);
		assert(!pscd->Exists_(sArchive.c_str(), index));
		pscd->Append_(sArchive.c_str(), index, data, size);
		if(flag) pscd->Cached_(sArchive.c_str(), index);
	}
};

void SolidCache::Append(const std::string& sArchive, unsigned int index, const void* data, unsigned int size)
{
	OutputDebugPrintf("SolidCache::Append %s %d %d bytes", sArchive.c_str(), index, size);
	if(m_scd.Exists(sArchive.c_str(), index) || GetMaxMemory() <= 0) {
		m_scd.Append(sArchive.c_str(), index, data, size);
		OutputDebugPrintf("SolidCache::Append:disk %s %d %d bytes", sArchive.c_str(), index, size);
	} else {
		m_scm.GetFileCache(sArchive).Append(index, data, size);
		OutputDebugPrintf("SolidCache::Append:memory %s %d %d bytes", sArchive.c_str(), index, size);
		if(m_scm.GetMaxMemory() > 0 && m_scm.GetSize() > m_scm.GetMaxMemoryInBytes()) {
			OutputDebugPrintf("SolidCache::Append:purge_memory2disk %s %d %d", sArchive.c_str(), index, m_scm.GetFileCache(sArchive).GetCurSize(index));
			m_scm.ReduceSize(std::min(m_scm.GetSize(), std::max(m_scm.GetPurgeMemoryInBytes(), m_scm.GetSize() - m_scm.GetMaxMemoryInBytes())), SolidCacheDiskCallback::ReduceSizeCallback, &m_scd, sArchive);
		}
	}
}

void SolidFileCacheMemory::AccessArchive_() const
{
	std::time(&m_atime);
}

struct Argument2
{
	std::string sArchive;
	void *pArg;
};

unsigned int SolidFileCacheMemory::ReduceSize_(unsigned int uiSize, void(*fCallback)(void*, const std::string&, unsigned int, void*, unsigned int, bool), void* pArg)
{
	Argument2 *pArg2 = static_cast<Argument2*>(pArg);
	while(uiSize > 0 && m_cache.size() > 0) {
		OutputDebugPrintf("SolidFileCacheMemory::ReduceSize %u ", uiSize);
		unsigned int index = m_cache.begin()->first;
		fCallback(pArg2->pArg, pArg2->sArchive, index, &m_cache[index].vBuffer[0], m_cache[index].vBuffer.size(), m_cache[index].fCached);
		if(uiSize < m_cache[index].vBuffer.size()) uiSize = 0;
		else uiSize -= m_cache[index].vBuffer.size();
		m_cache.erase(index);
	}
	return uiSize;
}

void SolidCacheMemory::AccessArchive_(const char* archive)
{
	std::time_t atime;
	std::time(&atime);
	m_mAccess[archive] = atime;
}

std::string SolidCacheMemory::GetLRUArchive_(const std::string& sExcludeArchive) const
{
	std::string sArchive = m_mAccess.begin()->first;
	std::time_t atime = m_mAccess.begin()->second;
	for(std::map<std::string, std::time_t>::const_iterator it = m_mAccess.begin(); it != m_mAccess.end(); ++it) {
		if(sArchive == sExcludeArchive || (it->first != sExcludeArchive && atime > it->second)) {
			sArchive = it->first;
			atime = it->second;
		}
	}
	return sArchive;
}

int SolidCacheMemory::GetSize_() const
{
	int nResult = 0;
	for(std::map<std::string, SolidFileCacheMemory::FileCache>::const_iterator it1 = m_mTable.begin(); it1 != m_mTable.end(); ++it1) {
		for(SolidFileCacheMemory::FileCache::const_iterator it2 = it1->second.begin(); it2 != it1->second.end(); ++it2) {
			nResult += it2->second.vBuffer.size();
		}
	}
	return nResult;
}

unsigned int SolidCacheMemory::ReduceSize_(unsigned int uiSize, void(*fCallback)(void*, const std::string&, unsigned int, void*, unsigned int, bool), void* pArg, const std::string& sExcludeArchive)
{
	OutputDebugPrintf("SolidCacheMemory::ReduceSize %u ", uiSize);
	while(uiSize > 0) {
		if(m_mTable.size() > 0) {
			std::string sArchive = GetLRUArchive_(sExcludeArchive);
			SolidFileCacheMemory scm = GetFileCache(sArchive);
			if(scm.GetCount_() > 0) {
				Argument2 arg2 = { sArchive, pArg };
				uiSize = scm.ReduceSize_(uiSize, fCallback, &arg2);
			} else {
				m_mTable.erase(sArchive);
				m_mAccess.erase(sArchive);
			}
		}
	}
	return uiSize;
}
