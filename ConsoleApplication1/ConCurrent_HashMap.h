#pragma once
#include <concurrent_unordered_map.h>
#pragma comment(lib, "winmm.lib")


template<class Key, class Val>
class Concurrent_Unordered_map
{
public:

	// size_threshold를 작게 잡으면 엄청 느려진다. 
	Concurrent_Unordered_map(int size_threshold, DWORD Del_time_ms)
	{
		InitializeSRWLock(&srw_lock);

		m_size_threshold = size_threshold;
		m_Del_time = Del_time_ms;

		
	
	}
	~Concurrent_Unordered_map()
	{
		
	}

	
	Val& at(const Key& KVal)
	{
		AcquireSRWLockShared(&srw_lock);
		Val* ret = HashMap.at(KVal);
		ReleaseSRWLockShared(&srw_lock);
		return *ret;
	}

	int size()
	{
		AcquireSRWLockShared(&srw_lock);
		int size = HashMap.size();
		ReleaseSRWLockShared(&srw_lock);
		return size;
	}

	int erase(Key& key)
	{
		int ret;

		AcquireSRWLockExclusive(&srw_lock);
		ret = HashMap.unsafe_erase(key);
		ReleaseSRWLockExclusive(&srw_lock);

		return ret;
	}

	void insert(Key& key, Val& val)
	{
		
	//	printf("%d %d\n", size(), m_size_threshold);

		if (size() >= m_size_threshold)
		{
		
			HashMap_Size_reduce();
		}
		
		AcquireSRWLockShared(&srw_lock);
		HashMap.insert(std::pair<Key,Val>(key,val));
		ReleaseSRWLockShared(&srw_lock);
	}

	//unsafe
	void Clear()
	{
		HashMap.clear();
	}


private:
	SRWLOCK srw_lock;
	DWORD time;
	Concurrency::concurrent_unordered_map<Key,Val> HashMap;
	int m_size_threshold;
	DWORD m_Del_time;

	void HashMap_Size_reduce()
	{
		time = timeGetTime();

		AcquireSRWLockExclusive(&srw_lock);
	
	for (typename Concurrency::concurrent_unordered_map<Key, Val>::iterator iter= HashMap.begin(); iter != HashMap.end();)
	{
		std::pair<Key, Val> tmp = *iter;

		if ((DWORD)(time - tmp.second ) > m_Del_time)
		{
		

			iter = HashMap.unsafe_erase(iter);
		}

		else
		{
			++iter;
		}
	}

		ReleaseSRWLockExclusive(&srw_lock);
	
	}

};