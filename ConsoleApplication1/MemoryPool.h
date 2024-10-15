#pragma once

#ifndef  __PROCADEMY_MEMORY_POOL__
#define  __PROCADEMY_MEMORY_POOL__
#include <new.h>

/*---------------------------------------------------------------

procademy MemoryPool.

메모리 풀 클래스 (오브젝트 풀 / 프리리스트)
특정 데이타(구조체,클래스,변수)를 일정량 할당 후 나눠쓴다.

- 사용법.

procademy::CMemoryPool<DATA> MemPool(300, FALSE);
DATA *pData = MemPool.Alloc();

pData 사용

MemPool.Free(pData);

// placement new 이면 node 뒷부분에 초기화여부를 넣는다.
----------------------------------------------------------------*/



	template <class T>
	class CMemoryPool
	{

		struct Node
		{
			T Data; //data를 가져야할듯
			int Node_Used_Cnt;
			Node* NextNode;
		};

	private:
		bool PlacementNew;
		int Capacity;
		int UseCount;
		Node* curNode;
		Node* AllocPoint;
		SRWLOCK srw_lock;

	
	public:

		//PlaceMentNew 에 경우 동적할당이 없는경우 사용하는것이 좋다.
		
		CMemoryPool(int iBlockNum, bool bPlacementNew = false)
		{
			Capacity = iBlockNum;
			PlacementNew = bPlacementNew;
			UseCount = 0;

			AllocPoint = (Node*)malloc(sizeof(Node) * iBlockNum);
			curNode = AllocPoint;

			for (int i = 0; i < iBlockNum; i++)
			{
				Node* node = new (curNode) Node;
				node->NextNode = (Node*)(++curNode);
				node->Node_Used_Cnt = 0;
					
			}
			InitializeSRWLock(&srw_lock);
			curNode = AllocPoint;
		}

		
		virtual ~CMemoryPool()
		{
			curNode = AllocPoint;

			if (PlacementNew)
			{
				for (int i = 0; i < Capacity; i++)
				{
					if (curNode->Node_Used_Cnt == 0) continue;

					T* node = reinterpret_cast<T*>(curNode);
					node->~T();

					(Node*)(++curNode);
				}


			}

			free(AllocPoint);
		}
	

		void Clear()
		{
			curNode = AllocPoint;
			UseCount = 0;

			for (int i = 0; i < Capacity; i++)
			{
				if (PlacementNew)
				{
					T* node = reinterpret_cast<T*>(curNode);
					node->~T();
				}

				Node* node = new (curNode) Node;

	

				node->NextNode = (Node*)(++curNode);
				node->Node_Used_Cnt = 0;


			}

			curNode = AllocPoint;
		}

		
		

		//////////////////////////////////////////////////////////////////////////
		// 블럭 하나를 할당받는다.  
		//
		// Parameters: 없음.
		// Return: (DATA *) 데이타 블럭 포인터.
		//////////////////////////////////////////////////////////////////////////
		T* Alloc(void)
		{
		
			

			AcquireSRWLockExclusive(&srw_lock);

			Node* ret = curNode;
			curNode = curNode->NextNode;
			UseCount++;

			ReleaseSRWLockExclusive(&srw_lock);

			if (ret -> Node_Used_Cnt !=0 || !PlacementNew)
			{
				new (ret) T;
			}

			

			ret->Node_Used_Cnt++;

			if (UseCount > Capacity) __debugbreak();

			return reinterpret_cast<T*>(ret);

		}

		//////////////////////////////////////////////////////////////////////////
		// 사용중이던 블럭을 해제한다.
		//
		// Parameters: (DATA *) 블럭 포인터.
		// Return: (BOOL) TRUE, FALSE.
		//////////////////////////////////////////////////////////////////////////
		bool Free(T* pData)
		{
			Node* input = reinterpret_cast<Node*>(pData);
			// printf("%d %d\n", (Node*)input->Data,(T*)input);

			//placement new면 소멸자 호출여부
			if (!PlacementNew)
			{
				pData->~T();
			}

			AcquireSRWLockExclusive(&srw_lock);

			input->NextNode = curNode;
			curNode = input;

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
			return Capacity;
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

#endif __PROCADEMY_MEMORY_POOL__