#pragma once
#include <Windows.h>
#include <wchar.h>
#include <cstdint>
#include <stdexcept>
#define ProfileSize 100
#define ThreadNum 50
#define name_len 64
#define Default_Writing_Period 100000
#define Profile_File_Name "Profiler.txt"

class Profiler_Manager
{

	struct st_TLS;



public:

	class Profile
	{
		friend class Profiler_Manager;


	private:
		const wchar_t* name;
		void Start(const wchar_t* tag);
		void End();
		int Profile_Writing_Period = Default_Writing_Period;


	public:
		Profile(const wchar_t* tag, int period = Default_Writing_Period);
		~Profile();

		struct st_profile
		{
			short			Flag;				// 0이면 초기상태, 1이면 열린, -1이면 닫힌
			wchar_t			szName[name_len];			// 프로파일 샘플 이름.

			LARGE_INTEGER	lStartTime;			// 프로파일 샘플 실행 시간.

			double			iTotalTime;			// 전체 사용시간 카운터 Time.	(출력시 호출회수로 나누어 평균 구함)
			double			iMin;			// 최소 사용시간 카운터 Time.	(초단위로 계산하여 저장 / [0] 가장최소 [1] 다음 최소 [2])
			double			iMax;			// 최대 사용시간 카운터 Time.	(초단위로 계산하여 저장 / [0] 가장최대 [1] 다음 최대 [2])

			__int64			iCall;				// 누적 호출 횟수.


		};


	};

	void Init();
	void Profile_Stop();
	static Profile::st_profile* Profile_find(const wchar_t* tag, st_TLS*);
	static bool File_Write();
	static bool Add(Profile::st_profile* data, st_TLS*);

	~Profiler_Manager()
	{
		TlsFree(Profile_dwTlsIndex);
		for (int i = 0; i < ThreadNum; i++)
		{
			if (Profile_Tlspool[i] != nullptr)
			{
				delete(Profile_Tlspool[i]);
			}

		}
	}


	struct st_TLS
	{
		// 배열크기  ->  map size
		Profile::st_profile* profile_map;

		~st_TLS()
		{
			delete[] profile_map;
		}

		st_TLS()
		{
			profile_map = new Profile::st_profile[ProfileSize];
			memset(profile_map, 0, sizeof(Profile::st_profile) * ProfileSize);
		}

	};


private:



	static LARGE_INTEGER Profile_Freq;
	static INT64 Profile_total_Counter;
	static FILE* Profile_fp;
	static DWORD Profile_dwTlsIndex;
	static st_TLS* Profile_Tlspool[ThreadNum];
	static SRWLOCK tlspool_lock; // 확인요망

	static bool register_emptyPool(st_TLS*);

};
