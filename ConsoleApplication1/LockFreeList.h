
#pragma once



template <class T>
class CFreeList
{
private:

	/* **************************************************************** */
	// �� �� �տ� ���� ��� ����ü.
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
	// ������, �ı���.
	//
	// Parameters:	(int) �ʱ� �� ����.
	//				(bool) Alloc �� ������ / Free �� �ı��� ȣ�� ����
	// Return:
	//////////////////////////////////////////////////////////////////////////
	CFreeList(int iBlockNum, bool bPlacementNew = false)
	{

		// �ɹ� ���� �ʱ�ȭ
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
	// �� �ϳ��� �Ҵ�޴´�.  
	//
	// Parameters: ����.
	// Return: (T *) ����Ÿ �� ������.
	//////////////////////////////////////////////////////////////////////////
	T* Alloc(void)
	{
		Node* newNode = nullptr;
		TOP_NODE CloneTop;

		LONG64 MaxCount = m_lMaxCount;
		InterlockedIncrement64(&m_lUseCount);
		// ���� ������ �Ѵٸ�
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
	// ������̴� ���� �����Ѵ�.
	//
	// Parameters: (T *) �� ������.
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
	// ���� Ȯ�� �� �� ������ ��´�. (�޸�Ǯ ������ ��ü ����)
	//
	// Parameters: ����.
	// Return: (int) �޸� Ǯ ���� ��ü ����
	//////////////////////////////////////////////////////////////////////////
	LONG64		GetAllocCount(void) { return m_lMaxCount; }

	//////////////////////////////////////////////////////////////////////////
	// ���� ������� �� ������ ��´�.
	//
	// Parameters: ����.
	// Return: (int) ������� �� ����.
	//////////////////////////////////////////////////////////////////////////
	LONG64		GetUseCount(void) { return m_lUseCount; }


public:

private:
	// ���� ������� ��ȯ�� (�̻��) ������Ʈ ���� ����.

	TOP_NODE* _pTop;
	LONG64 m_lMaxCount;
	LONG64 m_lUseCount;
	bool  m_bUsingPlacementNew;
	LONG64 m_UniqueNum;
};

