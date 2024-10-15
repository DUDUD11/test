
#pragma once



template <class T>
class CFreeList
{
private:

	/* **************************************************************** */
	// 각 블럭 앞에 사용될 노드 구조체.
	/* **************************************************************** */
	struct Node
	{
		T data;
		Node* NextNode;
	};


	struct TOP_NODE
	{
		Node* pTopNode;
		LONG64 lCount;
	};
public:

	//////////////////////////////////////////////////////////////////////////
	// 생성자, 파괴자.
	//
	// Parameters:	(int) 초기 블럭 개수.
	//				(bool) Alloc 시 생성자 / Free 시 파괴자 호출 여부
	// Return:
	//////////////////////////////////////////////////////////////////////////
	CFreeList(int iBlockNum, bool bPlacementNew = false)
	{

		// 맴버 변수 초기화
		m_lMaxCount = iBlockNum;
		m_lUseCount = 0;
		m_UniqueNum = 0;
		_pTop = (TOP_NODE*)_aligned_malloc(sizeof(TOP_NODE), 16);
		memset(_pTop, 0, sizeof(_pTop));

		m_bUsingPlacementNew = bPlacementNew;

	}
	virtual	~CFreeList()
	{
		Node* temp;

		for (int i = 0; i < m_lMaxCount; i++)
		{
			temp = _pTop->pTopNode;
			_pTop->pTopNode = _pTop->pTopNode->NextNode;
			free(temp);
		}

		_aligned_free(_pTop);
	}


	//////////////////////////////////////////////////////////////////////////
	// 블럭 하나를 할당받는다.  
	//
	// Parameters: 없음.
	// Return: (T *) 데이타 블럭 포인터.
	//////////////////////////////////////////////////////////////////////////
	T* Alloc(void)
	{
		Node* newNode = nullptr;
		TOP_NODE CloneTop;

		LONG64 MaxCount = m_lMaxCount;
		InterlockedIncrement64(&m_lUseCount);
		// 새로 만들어야 한다면
		if (MaxCount < m_lUseCount)
		{
			newNode = (Node*)malloc(sizeof(Node));
			InterlockedIncrement64(&m_lMaxCount);
		}
		else
		{

			LONG64 newCount = InterlockedIncrement64(&m_UniqueNum);

			do
			{
				CloneTop.pTopNode = _pTop->pTopNode;
				CloneTop.lCount = _pTop->lCount;
			} while (!InterlockedCompareExchange128((LONG64*)_pTop, newCount, (LONG64)_pTop->pTopNode->NextNode, (LONG64*)&CloneTop));

			newNode = CloneTop.pTopNode;
		}


		if (m_bUsingPlacementNew)
			new (newNode)T;

		return reinterpret_cast<T*>(newNode);




	}

	//////////////////////////////////////////////////////////////////////////
	// 사용중이던 블럭을 해제한다.
	//
	// Parameters: (T *) 블럭 포인터.
	// Return: (BOOL) TRUE, FALSE.
	//////////////////////////////////////////////////////////////////////////
	bool	Free(T* pData)
	{
		Node* returnedBlock = ((Node*)pData);
		TOP_NODE CloneTop;

		if (m_bUsingPlacementNew)
			delete pData;

		LONG64 newCount = InterlockedIncrement64(&m_UniqueNum);
		do
		{
			CloneTop.pTopNode = _pTop->pTopNode;
			CloneTop.lCount = _pTop->lCount;
			returnedBlock->NextNode = _pTop->pTopNode;
		} while (!InterlockedCompareExchange128((LONG64*)_pTop, newCount, (LONG64)returnedBlock, (LONG64*)&CloneTop));

		InterlockedDecrement64(&m_lUseCount);

		return true;





	}


	//////////////////////////////////////////////////////////////////////////
	// 현재 확보 된 블럭 개수를 얻는다. (메모리풀 내부의 전체 개수)
	//
	// Parameters: 없음.
	// Return: (int) 메모리 풀 내부 전체 개수
	//////////////////////////////////////////////////////////////////////////
	LONG64		GetAllocCount(void) { return m_lMaxCount; }

	//////////////////////////////////////////////////////////////////////////
	// 현재 사용중인 블럭 개수를 얻는다.
	//
	// Parameters: 없음.
	// Return: (int) 사용중인 블럭 개수.
	//////////////////////////////////////////////////////////////////////////
	LONG64		GetUseCount(void) { return m_lUseCount; }


public:

private:
	// 스택 방식으로 반환된 (미사용) 오브젝트 블럭을 관리.

	TOP_NODE* _pTop;
	LONG64 m_lMaxCount;
	LONG64 m_lUseCount;
	bool  m_bUsingPlacementNew;
	LONG64 m_UniqueNum;
};

