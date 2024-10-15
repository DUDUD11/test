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
			short			Flag;				// 0�̸� �ʱ����, 1�̸� ����, -1�̸� ����
			wchar_t			szName[name_len];			// �������� ���� �̸�.

			LARGE_INTEGER	lStartTime;			// �������� ���� ���� �ð�.

			double			iTotalTime;			// ��ü ���ð� ī���� Time.	(��½� ȣ��ȸ���� ������ ��� ����)
			double			iMin;			// �ּ� ���ð� ī���� Time.	(�ʴ����� ����Ͽ� ���� / [0] �����ּ� [1] ���� �ּ� [2])
			double			iMax;			// �ִ� ���ð� ī���� Time.	(�ʴ����� ����Ͽ� ���� / [0] �����ִ� [1] ���� �ִ� [2])

			__int64			iCall;				// ���� ȣ�� Ƚ��.


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
		// �迭ũ��  ->  map size
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
	static SRWLOCK tlspool_lock; // Ȯ�ο��

	static bool register_emptyPool(st_TLS*);

};
