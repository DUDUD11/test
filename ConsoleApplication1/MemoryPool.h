#pragma once

#ifndef  __PROCADEMY_MEMORY_POOL__
#define  __PROCADEMY_MEMORY_POOL__
#include <new.h>

/*---------------------------------------------------------------

procademy MemoryPool.

�޸� Ǯ Ŭ���� (������Ʈ Ǯ / ��������Ʈ)
Ư�� ����Ÿ(����ü,Ŭ����,����)�� ������ �Ҵ� �� ��������.

- ����.

procademy::CMemoryPool<DATA> MemPool(300, FALSE);
DATA *pData = MemPool.Alloc();

pData ���

MemPool.Free(pData);

// placement new �̸� node �޺κп� �ʱ�ȭ���θ� �ִ´�.
----------------------------------------------------------------*/



	template <class T>
	class CMemoryPool
	{

		struct Node
		{
			T Data; //data�� �������ҵ�
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

		//PlaceMentNew �� ��� �����Ҵ��� ���°�� ����ϴ°��� ����.
		
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
		// �� �ϳ��� �Ҵ�޴´�.  
		//
		// Parameters: ����.
		// Return: (DATA *) ����Ÿ �� ������.
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
		// ������̴� ���� �����Ѵ�.
		//
		// Parameters: (DATA *) �� ������.
		// Return: (BOOL) TRUE, FALSE.
		//////////////////////////////////////////////////////////////////////////
		bool Free(T* pData)
		{
			Node* input = reinterpret_cast<Node*>(pData);
			// printf("%d %d\n", (Node*)input->Data,(T*)input);

			//placement new�� �Ҹ��� ȣ�⿩��
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
		// ���� Ȯ�� �� �� ������ ��´�. (�޸�Ǯ ������ ��ü ����)
		//
		// Parameters: ����.
		// Return: (int) �޸� Ǯ ���� ��ü ����
		//////////////////////////////////////////////////////////////////////////
		int GetCapacityCount(void) {
			return Capacity;
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

#endif __PROCADEMY_MEMORY_POOL__