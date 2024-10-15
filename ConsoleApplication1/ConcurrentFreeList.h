#pragma once

#ifndef  __FreeList__
#define  __FreeList__
#include <new.h>


	template <class T>
	class ConcurrentFreeList
	{

		struct Node
		{
			T Data;
			Node* NextNode;
		};

	private:

		Node* m_head;
		Node* m_tail;
		SRWLOCK srw_lock;
		int UseCount;
		int Capacity;

		// ���ÿ� ������ wrapper�� ���μ� �ڽ��� T���� Ȯ���ϴ� ���� �־ ����
	public:

	

		ConcurrentFreeList()
		{
			InitializeSRWLock(&srw_lock);

			UseCount = 0;
			Capacity = 0;
	
			m_head = nullptr;
			m_tail = nullptr;
			
		}

		/*
		virtual ~CMemoryPool()
		{
		//stack�� ���ƿ� �κ��� �����
		}
		*/

		// ��ȯ�������� node�� �������� �ʴ´�.
		// ����߿� ȣ��� ������ �߻��Ѵ�.
		void Clear()
		{

			while (UseCount-- > 0)
			{
				Node* tmp = m_head;
				m_head = m_head->NextNode;
				free(tmp);
			}

			m_head = nullptr;
			m_tail = nullptr;

			UseCount = 0;
			Capacity = 0;
		}


		~ConcurrentFreeList()
		{
			Clear();	
		}

		//////////////////////////////////////////////////////////////////////////
		// ��尡 ��� ������̸� �����Ҵ��ϰ� ���� ��尡 ������ ���� ��带 �����Ѵ�.  
		//
		// Parameters: ����.
		// Return: (DATA *) ����Ÿ �� ������.
		//////////////////////////////////////////////////////////////////////////
		T* Alloc(void)
		{
			Node* ret;

			AcquireSRWLockExclusive(&srw_lock);

			if (UseCount == Capacity)
			{
				ret = (Node*)malloc(sizeof(Node));
	
				UseCount++;
				Capacity++;
			}

			else
			{
				ret = m_head;
				m_head = m_head->NextNode;
	
				UseCount++;
			}

			ReleaseSRWLockExclusive(&srw_lock);

		

			// for test
			new (ret) T;

			return reinterpret_cast<T*>(ret);

		}

		//////////////////////////////////////////////////////////////////////////
		// ������̴� ���� �����ϰ� ���� ��带 nextnode�� ����
		//
		// Parameters: (DATA *) �� ������.
		// Return: (BOOL) TRUE, FALSE.
		//////////////////////////////////////////////////////////////////////////
		bool Free(T* pData)
		{
			if (UseCount <= 0) __debugbreak();

		

			Node* input = reinterpret_cast<Node*>(pData);
		
			// for test
			pData->~T();
		
			AcquireSRWLockExclusive(&srw_lock);

			if (Capacity == UseCount)
			{
				m_head = input;
				m_tail = input;
				

			}

			else
			{
				m_tail->NextNode = input;
				m_tail = input;

			}

			UseCount--;

			ReleaseSRWLockExclusive(&srw_lock);


			return true;
		}


		//////////////////////////////////////////////////////////////////////////
		// ���� Ȯ�� �� �� ������ ��´�. (�޸�Ǯ ������ ��ü ����)
		//
		// Parameters: ����.
		// Return: (int) �޸� Ǯ ���� ��ü ����
		//////////////////////////////////////////////////////////////////////////
		int GetCapacityCount(void) { 
			
			int ret = 0;

			AcquireSRWLockShared(&srw_lock);
			ret = Capacity;
			ReleaseSRWLockShared(&srw_lock);
			
			return ret; 
		
		}

		//////////////////////////////////////////////////////////////////////////
		// ���� ������� �� ������ ��´�.
		//
		// Parameters: ����.
		// Return: (int) ������� �� ����.
		//////////////////////////////////////////////////////////////////////////
		int GetUseCount(void) { 

			int ret = 0;

			AcquireSRWLockShared(&srw_lock);
			ret = UseCount;
			ReleaseSRWLockShared(&srw_lock);
		
			return ret;
		}


		// ���� ������� ��ȯ�� (�̻��) ������Ʈ ���� ����.


	};


#endif __FreeList__