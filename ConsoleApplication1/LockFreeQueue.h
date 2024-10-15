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
				// tail�� next�� null�̰� ���� �̸� ���� tail�� ������ ��尡 ����Ű�� ���� ��� �ּҰ��� tail�� ����Ű�� �ִ� ����� ���� ��� �ּҰ��� ���� �̰� nullptr�̸�.
				if (_InterlockedCompareExchangePointer((PVOID*)&tempTailNode.pTopNode->NextNode, newNode, nextNode) == nullptr)
				{
					InterlockedIncrement64(&m_QueueSize);

					InterlockedCompareExchange128((LONG64*)m_pTail, newCount, (LONG64)tempTailNode.pTopNode->NextNode, (LONG64*)&tempTailNode);

					break;
				}
			}
			// newNode �� �ٸ� �����忡�� ���ԵǾ����� m_pTail �۾��� ������ ���� ����ä�� ���ؽ�Ʈ ����Ī�� �Ͼ ��쿡��
			// m_pTail �� ���縦 �õ��ϰ� ������ ��� ���� �������� ���� ���԰����� �������Ҽ� �ִ�.
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
			// ��� ����
			tempHeadNode.pTopNode = m_pHead->pTopNode;
			tempHeadNode.lCount = m_pHead->lCount;

			// ���� ����
			tempTailNode.pTopNode = m_pTail->pTopNode;
			tempTailNode.lCount = m_pTail->lCount;

			// ����� next����
			nextNode = tempHeadNode.pTopNode->NextNode;

			// ����ٸ� �̰��� �����ؾ� �Ѵ�.
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

				// ����� next�� ��尡 �����Ѵٸ�
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
// ������ �ܰ�� 2�ܰ�
// tail�ڿ� �� ��� ����
// tail�� �� ���� ����

// tail�ڿ� �� ��带 �����ϴ� ���߿� �ٸ� �����忡�� enqeueue�� �õ�. tail�� next���� �� ��尡 �ִ� ����.
// �ٸ� ������� tail�� next�� Ȯ�������� tail�� �ű�� 2��° cas�� �������� �ʾƼ� tail�� �� ��带 ���� �� �� ����
// �׷��� �ٸ� �����尡 tail�� ��ĭ �ű�� �ٽ� tail�� next�� �ٽ� Ȯ������.
// ���� ������� 2��° cas�� �����ϴ� �����ϴ� ������� �����ϰ� �������´�.
// tail�� ��ġ�� ��¥�� �ٸ� �����忡�� �Ű��� ���̱� ����.