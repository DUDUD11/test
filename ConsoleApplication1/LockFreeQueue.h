#pragma once

#include "LockFreeList.h"

template <typename T>
class CLockFreeQueue
{
private:
	LONG64 m_QueueSize;


	struct st_NODE
	{
		T data;
		st_NODE* NextNode;
	};

	struct TOP_NODE
	{
		st_NODE* pTopNode;
		LONG64 lCount;
	};

	TOP_NODE* m_pHead;
	TOP_NODE* m_pTail;

	CFreeList<st_NODE>* m_MemoryPool;

	st_NODE* m_pDummyNode;


	LONG64 m_HeadUniqueNum;
	LONG64 m_TailUniqueNum;


public:
	CLockFreeQueue()
	{



		m_QueueSize = m_HeadUniqueNum = m_TailUniqueNum = 0;
		m_MemoryPool = new CFreeList<st_NODE>(0, FALSE);

		m_pDummyNode = m_MemoryPool->Alloc();
		m_pDummyNode->data;
		new (m_pDummyNode)T;
		m_pDummyNode->NextNode = nullptr;

		m_pHead = (TOP_NODE*)_aligned_malloc(sizeof(TOP_NODE), 16);
		m_pHead->lCount = 0;
		m_pHead->pTopNode = m_pDummyNode;

		m_pTail = (TOP_NODE*)_aligned_malloc(sizeof(TOP_NODE), 16);
		m_pTail->lCount = 0;
		m_pTail->pTopNode = m_pDummyNode;
	}

	~CLockFreeQueue()
	{
		st_NODE* temp;
		while (m_pHead->pTopNode != nullptr)
		{
			temp = m_pHead->pTopNode;
			m_pHead->pTopNode = m_pHead->pTopNode->NextNode;
			m_MemoryPool->Free(temp);
		}

		delete m_MemoryPool;

		_aligned_free(m_pHead);
		_aligned_free(m_pTail);

	}

	void Clear()
	{
		
	
	
	}

	BOOL Enqueue()
	{
		st_NODE* newNode = m_MemoryPool->Alloc();
		//test
		new (newNode)T;


		//	newNode->data = data;
		newNode->NextNode = nullptr;

		TOP_NODE tempTailNode;
		st_NODE* nextNode;


		while (true)
		{
			LONG64 newCount = InterlockedIncrement64(&m_TailUniqueNum);

			tempTailNode.pTopNode = m_pTail->pTopNode;
			tempTailNode.lCount = m_pTail->lCount;
			nextNode = m_pTail->pTopNode->NextNode;

			if (nextNode == nullptr)
			{
				// tail의 next가 null이고 현재 미리 얻어논 tail을 복사한 노드가 가리키는 다음 노드 주소값이 tail이 가리키고 있는 노드의 다음 노드 주소값과 같고 이게 nullptr이면.
				if (_InterlockedCompareExchangePointer((PVOID*)&tempTailNode.pTopNode->NextNode, newNode, nextNode) == nullptr)
				{
					InterlockedIncrement64(&m_QueueSize);

					InterlockedCompareExchange128((LONG64*)m_pTail, newCount, (LONG64)tempTailNode.pTopNode->NextNode, (LONG64*)&tempTailNode);

					break;
				}
			}
			// newNode 가 다른 스레드에서 삽입되었으나 m_pTail 작업이 마무리 되지 않은채로 컨텍스트 스위칭이 일어난 경우에는
			// m_pTail 로 복사를 시도하고 성공한 경우 다음 루프에서 내가 삽입과정을 마무리할수 있다.
			else
			{
				int x = InterlockedCompareExchange128((LONG64*)m_pTail, newCount, (LONG64)tempTailNode.pTopNode->NextNode, (LONG64*)&tempTailNode);
			}
		}



		return TRUE;
	}

	BOOL Dequeue(T* data)
	{
		TOP_NODE tempHeadNode, tempTailNode;
		st_NODE* nextNode;

		LONG64 newHead = InterlockedIncrement64(&m_HeadUniqueNum);


		while (true)
		{
			// 헤드 저장
			tempHeadNode.pTopNode = m_pHead->pTopNode;
			tempHeadNode.lCount = m_pHead->lCount;

			// 테일 저장
			tempTailNode.pTopNode = m_pTail->pTopNode;
			tempTailNode.lCount = m_pTail->lCount;

			// 헤드의 next저장
			nextNode = tempHeadNode.pTopNode->NextNode;

			// 비었다면 이곳도 수정해야 한다.
			if (m_QueueSize == 0 && (m_pHead->pTopNode->NextNode == nullptr))
			{
				data = nullptr;
				__debugbreak();
				return FALSE;
			}
			else
			{


				if (tempTailNode.pTopNode->NextNode != nullptr)
				{

					LONG64 newCount = InterlockedIncrement64(&m_TailUniqueNum);
					InterlockedCompareExchange128((LONG64*)m_pTail, newCount, (LONG64)tempTailNode.pTopNode->NextNode, (LONG64*)&tempTailNode);
				}

				// 헤드의 next에 노드가 존재한다면
				if (nextNode != nullptr)
				{
					*data = nextNode->data;
					if (InterlockedCompareExchange128((LONG64*)m_pHead, newHead, (LONG64)tempHeadNode.pTopNode->NextNode, (LONG64*)&tempHeadNode))
					{

						//test
						tempHeadNode.pTopNode->data.~T();

						m_MemoryPool->Free(tempHeadNode.pTopNode);



						InterlockedDecrement64(&m_QueueSize);

						break;
					}

				}
			}
		}




		return TRUE;
	}

	LONG64 GetUsingCount(void)
	{
		return m_QueueSize;
	}

	LONG64 GetAllocCount(void)
	{
		return m_MemoryPool->GetAllocCount();
	}
};




// Enqeueue
// 삽입의 단계는 2단계
// tail뒤에 새 노드 연결
// tail을 새 노드로 변경

// tail뒤에 새 노드를 연결하는 와중에 다른 스레드에서 enqeueue를 시도. tail의 next노드는 새 노드가 있는 상태.
// 다른 스레드는 tail의 next를 확인하지만 tail을 옮기는 2번째 cas를 실행하지 않아서 tail에 새 노드를 연결 할 수 없음
// 그러면 다른 스레드가 tail을 한칸 옮기고 다시 tail의 next를 다시 확인하자.
// 기존 스레드는 2번째 cas가 성공하던 실패하던 상관없이 실행하고 빠져나온다.
// tail의 위치는 어짜피 다른 스레드에서 옮겨줄 것이기 때문.