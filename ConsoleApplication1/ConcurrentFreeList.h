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

		// 스택에 넣을때 wrapper로 감싸서 자신의 T인지 확인하는 것을 넣어도 좋음
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
		//stack에 돌아온 부분을 지운다
		}
		*/

		// 반환하지않은 node는 지워지지 않는다.
		// 사용중에 호출시 문제가 발생한다.
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
		// 노드가 모두 사용중이면 새로할당하고 남은 노드가 있으면 남은 노드를 리턴한다.  
		//
		// Parameters: 없음.
		// Return: (DATA *) 데이타 블럭 포인터.
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
		// 사용중이던 블럭을 해제하고 여분 노드를 nextnode에 저장
		//
		// Parameters: (DATA *) 블럭 포인터.
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
		// 현재 확보 된 블럭 개수를 얻는다. (메모리풀 내부의 전체 개수)
		//
		// Parameters: 없음.
		// Return: (int) 메모리 풀 내부 전체 개수
		//////////////////////////////////////////////////////////////////////////
		int GetCapacityCount(void) { 
			
			int ret = 0;

			AcquireSRWLockShared(&srw_lock);
			ret = Capacity;
			ReleaseSRWLockShared(&srw_lock);
			
			return ret; 
		
		}

		//////////////////////////////////////////////////////////////////////////
		// 현재 사용중인 블럭 개수를 얻는다.
		//
		// Parameters: 없음.
		// Return: (int) 사용중인 블럭 개수.
		//////////////////////////////////////////////////////////////////////////
		int GetUseCount(void) { 

			int ret = 0;

			AcquireSRWLockShared(&srw_lock);
			ret = UseCount;
			ReleaseSRWLockShared(&srw_lock);
		
			return ret;
		}


		// 스택 방식으로 반환된 (미사용) 오브젝트 블럭을 관리.


	};


#endif __FreeList__