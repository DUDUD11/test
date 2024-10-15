

#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS
#include "Profiler.h"



void Profiler_Manager::Init()
{
	QueryPerformanceFrequency(&Profile_Freq);
	InitializeSRWLock(&tlspool_lock);
	Profile_dwTlsIndex = TlsAlloc();
	if (Profile_dwTlsIndex == TLS_OUT_OF_INDEXES)
	{
		__debugbreak();

	}

}

//특정 Thread만 Stop
void Profiler_Manager::Profile_Stop()
{
	//초기화

	st_TLS* tls_ = (st_TLS*)TlsGetValue(Profile_dwTlsIndex);

	if (tls_ == 0 && GetLastError() != ERROR_SUCCESS)
	{
		__debugbreak();
	}

	int idx = -1;

	AcquireSRWLockExclusive(&tlspool_lock);

	for (int i = 0; i < ThreadNum; i++)
	{
		if (tls_ == Profile_Tlspool[i])
		{
			idx = i;
			Profile_Tlspool[i] = nullptr;
			break;
		}
	}

	ReleaseSRWLockExclusive(&tlspool_lock);
	if (idx == -1)
	{
		__debugbreak();
	}

	delete tls_;
}

Profiler_Manager::Profile::st_profile* Profiler_Manager::Profile_find(const wchar_t* tag, st_TLS* tls)
{



	for (int i = 0; i < ProfileSize; i++)
	{
		if (wcscmp(tls->profile_map[i].szName, tag) == 0)
		{
			return &tls->profile_map[i];
		}

	}

	return nullptr;

}

bool Profiler_Manager::File_Write()
{

	Profile_fp = fopen(Profile_File_Name, "w");
	wchar_t buffer[100000] = { 0, };

	int num = 0;

	AcquireSRWLockShared(&tlspool_lock);

	for (int thread_idx = 0; thread_idx < ThreadNum; thread_idx++)
	{


		if (Profile_Tlspool[thread_idx] == nullptr) continue;



		num += swprintf(buffer + num, L"\tThread ID = %d\n", thread_idx);
		num += swprintf(buffer + num, L"        Name      |     Average  |        Min   |        Max   |      Call |\n");
		num += swprintf(buffer + num, L"---------------------------------------------------------------------------\n");

		for (int i = 0; i < ProfileSize; i++)
		{
			if (Profile_Tlspool[thread_idx]->profile_map[i].Flag != 0)
			{


				num += swprintf(buffer + num, L"%18s", Profile_Tlspool[thread_idx]->profile_map[i].szName);
				num += swprintf(buffer + num, L"%12fms", Profile_Tlspool[thread_idx]->profile_map[i].iTotalTime / (Profile_Tlspool[thread_idx]->profile_map[i].iCall));
				num += swprintf(buffer + num, L"%12fms", Profile_Tlspool[thread_idx]->profile_map[i].iMin);
				num += swprintf(buffer + num, L"%12fms", Profile_Tlspool[thread_idx]->profile_map[i].iMax);
				num += swprintf(buffer + num, L"%10lld\n", Profile_Tlspool[thread_idx]->profile_map[i].iCall);
			}
		}
	}

	ReleaseSRWLockShared(&tlspool_lock);

	fwrite(buffer, sizeof(buffer), 1, Profile_fp);



	fclose(Profile_fp);



	return true;
}

bool Profiler_Manager::Add(Profiler_Manager::Profile::st_profile* data, st_TLS* tls)
{
	for (int i = 0; i < ProfileSize; i++)
	{
		if (tls->profile_map[i].Flag == 0)
		{
			memcpy(&tls->profile_map[i], data, sizeof(Profiler_Manager::Profile::st_profile));
			return true;
		}

	}

	return false;
}

bool Profiler_Manager::register_emptyPool(st_TLS* src)
{
	int idx = -1;

	AcquireSRWLockExclusive(&tlspool_lock);

	for (int i = 0; i < ThreadNum; i++)
	{
		if (Profile_Tlspool[i] == nullptr)
		{
			Profile_Tlspool[i] = src;
			idx = i;
			break;
		}
	}

	ReleaseSRWLockExclusive(&tlspool_lock);

	return idx >= 0;

}



void Profiler_Manager::Profile::Start(const wchar_t* tag)
{
	// tls 없으면 초기화

	st_TLS* tls_ = (st_TLS*)TlsGetValue(Profile_dwTlsIndex);
	if (tls_ == nullptr)
	{
		tls_ = new st_TLS();
		TlsSetValue(Profile_dwTlsIndex, tls_);


		if (!register_emptyPool(tls_))
		{
			__debugbreak();
		}
	}
	// 필요한가
	name = tag;

	st_profile* temp = Profile_find(tag, tls_);

	if (temp == nullptr)
	{
		st_profile obj;
		obj.Flag = 1;
		wcsncpy(obj.szName, tag, name_len);

		QueryPerformanceCounter(&obj.lStartTime);
		obj.iTotalTime = 0;
		obj.iMax = 0;
		obj.iMin = DBL_MAX;
		obj.iCall = 0;

		if (!Add(&obj, tls_))
		{
			throw std::runtime_error("Profile Map Not Write");
		}
	}

	else
	{

		if (temp->Flag == 1)
		{
			throw std::runtime_error("Profile Not Closed");

		}

		temp->Flag = 1;
		QueryPerformanceCounter(&temp->lStartTime);
	}



}

void Profiler_Manager::Profile::End()
{

	st_TLS* tls_ = (st_TLS*)TlsGetValue(Profile_dwTlsIndex);

	LARGE_INTEGER end;
	double DeltaTime;
	st_profile* temp = Profile_find(name, tls_);

	if (temp == nullptr)
	{
		throw std::runtime_error("Profile Not Found");

	}

	if (temp->Flag != 1)
	{
		throw std::runtime_error("Profile Not Opened");
	}

	QueryPerformanceCounter(&end);
	temp->Flag = -1;
	DeltaTime = (double)(end.QuadPart - temp->lStartTime.QuadPart) / (double)Profile_Freq.QuadPart;
	DeltaTime *= 1000; // second 에서 millisecond로 변환
	temp->iTotalTime += DeltaTime;
	temp->iCall = temp->iCall + 1;
	temp->iMax = max(temp->iMax, DeltaTime);
	temp->iMin = min(temp->iMin, DeltaTime);

	int lock_int = InterlockedIncrement64(&Profile_total_Counter);

	if (lock_int % Profile_Writing_Period == 0)
	{
		if (!File_Write())
		{
			throw std::runtime_error("File Not Writed");

		}

	}




}

Profiler_Manager::Profile::Profile(const wchar_t* tag, int period)
{
	Start(tag);
	name = tag;
	Profile_Writing_Period = period;
}

Profiler_Manager::Profile::~Profile()
{
	End();
}


