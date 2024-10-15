#pragma once

#include "MemoryPool.h"

	template<class T>
	class MemoryPool_TLS
	{

	public:

		//////////////////////////////////////////////////////////////////////////
		// ������, �ı���.
		//
		// Parameters:	(int) �ִ� �� ����.
		//				(bool) ������ ȣ�� ����.
		// Return:
		//////////////////////////////////////////////////////////////////////////
		MemoryPool_TLS(int thread_num, int iBlockNum, bool bPlacementNew = false)
		{
			_dwTlsIndex = TlsAlloc();
			if (_dwTlsIndex == TLS_OUT_OF_INDEXES)
			{
				__debugbreak();

			}

			InitializeSRWLock(&TLSPool_lock);

			m_Thread_Num = thread_num;
			Capacity = iBlockNum;
			PlacementNew = bPlacementNew;
			m_TLS_useCount = 0;

			TLSPool_idx = new TLS_struct[thread_num];

			for (int i = 0; i < thread_num; i++)
			{
				TLSPool_idx[i].MemoryPoolTLS = new CMemoryPool<T>(iBlockNum, bPlacementNew);
				TLSPool_idx[i].Alloc_Index = i;
				TLSPool_idx[i].Using = false;
			}

		}
		virtual ~MemoryPool_TLS()
		{

			TlsFree(_dwTlsIndex);
			delete[] TLSPool_idx;
		}

		//////////////////////////////////////////////////////////////////////////
		// CHUNK �Ҵ�, ����
		//
		// Parameters: ����.
		// Return: (DATA *) ������ �� ������.
		//////////////////////////////////////////////////////////////////////////
		T* Alloc()
		{
			TLS_struct* tls_struct = (TLS_struct*)TlsGetValue(_dwTlsIndex);
			if (tls_struct == nullptr)
			{
				int idx = 0;

				if (!Init(idx))
				{
					__debugbreak();
				}

				TlsSetValue(_dwTlsIndex, &TLSPool_idx[idx]);
				tls_struct = &TLSPool_idx[idx];

				m_TLS_useCount++;
			}

			return tls_struct->MemoryPoolTLS->Alloc();

		}

		bool Init(int& ret_idx)
		{
			// search empty memorypool

			int idx = -1;

			AcquireSRWLockExclusive(&TLSPool_lock);

			for (int i = 0; i < m_Thread_Num; i++)
			{
				if (TLSPool_idx[i].Using == false)
				{
					idx = i;
					TLSPool_idx[i].Using = true;
					break;
				}
			}

			ReleaseSRWLockExclusive(&TLSPool_lock);

			if (idx == -1) return false;

			ret_idx = idx;

			return true;

		}

		bool	Free(T* pData)
		{
			TLS_struct* tls_struct = (TLS_struct*)TlsGetValue(_dwTlsIndex);
			return tls_struct->MemoryPoolTLS->Free(pData);
		}

		// ��ü�̸��� �����ϰ� ������ ��� ex) �����尳���� �ٲ㰡�� �׽�Ʈ�ϰ� ������
		void Clear(int thread_num, int iBlockNum, bool bPlacementNew = false)
		{
			delete[] TLSPool_idx;

			m_Thread_Num = thread_num;
			Capacity = iBlockNum;
			PlacementNew = bPlacementNew;
			m_TLS_useCount = 0;

			TLSPool_idx = new TLS_struct[thread_num];

			for (int i = 0; i < thread_num; i++)
			{
				TLSPool_idx[i].MemoryPoolTLS = new CMemoryPool<T>(iBlockNum, bPlacementNew);
				TLSPool_idx[i].Alloc_Index = i;
				TLSPool_idx[i].Using = false;
			}
		
		}

	private:

		struct TLS_struct
		{
			CMemoryPool<T>* MemoryPoolTLS;
			int Alloc_Index;
			bool Using;
			bool PlacementNew;


			~TLS_struct()
			{
				delete MemoryPoolTLS;
			}

		};




	public:
		//////////////////////////////////////////////////////////////////////////
		// ��� ���� �� ����
		//
		// Parameters: ����.
		// Return: (int) �޸� Ǯ ���� ��ü ����,
		//////////////////////////////////////////////////////////////////////////
		int	GetUseThreadCount() { return m_TLS_useCount; }

		int GetWholUseSize()
		{
			int size = 0;

			for (int i = 0; i < m_Thread_Num; i++)
			{
				if (TLSPool_idx[i].Using)
				{
					size += TLSPool_idx[i].MemoryPoolTLS->GetUseCount();
				}

			}

			return size;
		}

		int GetUseSize()
		{
			TLS_struct* tls_struct = (TLS_struct*)TlsGetValue(_dwTlsIndex);
			if (tls_struct == nullptr) return 0;
			return tls_struct->MemoryPoolTLS->GetUseCount();
		}

		//////////////////////////////////////////////////////////////////////////
		// Ȯ���� ûũ ����
		//
		// Parameters: ����.
		// Return: (int) �޸� Ǯ ���� ��ü ����,
		//////////////////////////////////////////////////////////////////////////
		int	GetAllocSize() {	
			return Capacity;
			
		}

		int GetWholeAllocSize()
		{

			return m_Thread_Num * Capacity;

		}

	private:
		
		DWORD	_dwTlsIndex;

		int m_Thread_Num;
		int m_TLS_useCount;
		bool PlacementNew;
		int Capacity;

		TLS_struct* TLSPool_idx;

		SRWLOCK TLSPool_lock;

	};