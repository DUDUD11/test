// ConsoleApplication1.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <Windows.h>
#include <iostream>
#include <mutex>
#include <thread>
#include "MemoryPool.h"
#include "ConcurrentFreeList.h"
#include "LockFreeQueue.h"
#include "MemoryPoolTLS.h"
#include "ConCurrent_HashMap.h"

#include "concurrentqueue-master/concurrentqueue.h"


#include "Profiler.h"
LARGE_INTEGER Profiler_Manager::Profile_Freq;
INT64 Profiler_Manager::Profile_total_Counter;
FILE* Profiler_Manager::Profile_fp;
DWORD Profiler_Manager::Profile_dwTlsIndex;
Profiler_Manager::st_TLS* Profiler_Manager::Profile_Tlspool[ThreadNum];
SRWLOCK Profiler_Manager::tlspool_lock;

using namespace std;
using namespace moodycamel;


#define bufsize 1000
int cnt = 0;


struct test
{
    char t[bufsize];
    long long int  q = 0;
  

    test()
    {
        /*
        int save[bufsize / 4];
       
        for (int i = 0; i < bufsize / 4; i++)
        {
           
            save[i] = rand();
            q += save[i];

        }
        memcpy(t, save, bufsize);
        */
       
   
    }

    ~test()
    {
       /*
        int save[bufsize / 4];
        memcpy(save, t, bufsize);

        for (int i = 0; i < bufsize / 4; i++)
        {  
          
            q -= save[i];
         
        }
    
        if (q != 0) __debugbreak();
      */
    }

};


unsigned int dummy = 0;

volatile long long * target = new long long ();


HANDLE hTread[12] = { 0, };

int thread_num = std::thread::hardware_concurrency()/2;

LARGE_INTEGER timer;

unsigned long long int goal;
unsigned long long int PoolListGoal = 2*3*4*5*6*9;



CRITICAL_SECTION critical_section;

SRWLOCK srw_lock;

mutex mut;

DWORD TimeOut = 30000;

CMemoryPool<test> MemPool(PoolListGoal, FALSE);

ConcurrentFreeList<test> FreeList;
CLockFreeQueue<test> LockFreeList;

moodycamel::ConcurrentQueue<test> moodycamel_queue;

Concurrent_Unordered_map<int, DWORD> HashMap((int)PoolListGoal, 10);

MemoryPool_TLS<test> TLSPool(0,0,0);



UINT __stdcall InterLockIncrease(void* arg)
{
    int x = *reinterpret_cast<int*>(arg);

    for (int i = 0; i < goal / (x + 1); i++)
    {
        InterlockedIncrement64(target);

    }

    return 0;
}

UINT __stdcall CriticalSectionIncrease(void* arg)
{
    int x = *reinterpret_cast<int*>(arg);

    for (int i = 0; i < goal / (x + 1); i++)
    {
        EnterCriticalSection(&critical_section);
        ++*target;
        LeaveCriticalSection(&critical_section);

    }

    return 0;
}

UINT __stdcall SRWLockIncrease(void* arg)
{
    int x = *reinterpret_cast<int*>(arg);

    for (int i = 0; i < goal / (x + 1); i++)
    {
        AcquireSRWLockExclusive(&srw_lock);
        ++* target;
        ReleaseSRWLockExclusive(&srw_lock);

    }

    return 0;
}

UINT __stdcall MutexIncrease(void* arg)
{
    int x = *reinterpret_cast<int*>(arg);

    for (int i = 0; i < goal / (x + 1); i++)
    {
        mut.lock();
        ++* target;
        mut.unlock();

    }

    return 0;
}

void InterLockedTest()
{
    LARGE_INTEGER start, end;
    float DeltaTime;

    for (int i = 0; i < thread_num; i++)
    {
        *target = 0;
        memset(hTread, 0, sizeof(HANDLE) * (thread_num));

        for (int j = 0; j <= i; j++)
        {
            hTread[j] = (HANDLE)_beginthreadex(NULL, 0, InterLockIncrease, &i, CREATE_SUSPENDED, &dummy);
        }

        QueryPerformanceCounter(&start);

        for (int j = 0; j <= i; j++)
        {
            ResumeThread(hTread[j]);
        }

        WaitForMultipleObjects((i + 1), hTread, true, INFINITE);

        QueryPerformanceCounter(&end);
     
        for (int j = 0; j <= i; j++)
        {
            CloseHandle(hTread[j]);
        }

        printf("InterLockedTest\n");
        printf("target value= %lld\n ", *target);
        printf("Thread num = %d DeltaTime = %f seconds\n", i + 1, (end.QuadPart - start.QuadPart) / (float)timer.QuadPart);

    }

}

void CriticalSectionTest()
{
    LARGE_INTEGER start, end;
    float DeltaTime;

    for (int i = 0; i < thread_num; i++)
    {
    
        *target = 0;
        memset(hTread, 0, sizeof(HANDLE) * (thread_num));

        for (int j = 0; j <= i; j++)
        {
            hTread[j] = (HANDLE)_beginthreadex(NULL, 0, CriticalSectionIncrease, &i, CREATE_SUSPENDED, &dummy);
        }

        QueryPerformanceCounter(&start);

        for (int j = 0; j <= i; j++)
        {
            ResumeThread(hTread[j]);
        }

        WaitForMultipleObjects((i + 1), hTread, true, INFINITE);

        QueryPerformanceCounter(&end);

        for (int j = 0; j <= i; j++)
        {
            CloseHandle(hTread[j]);
        }

        printf("CriticalSectionTest \n");
        printf("target value= %lld\n ", *target);
        printf("Thread num = %d DeltaTime = %f seconds\n", i + 1, (end.QuadPart - start.QuadPart) / (float)timer.QuadPart);
    }

}

void SRWLockTest()
{
    LARGE_INTEGER start, end;
    float DeltaTime;

    for (int i = 0; i < thread_num; i++)
    {

        *target = 0;
        memset(hTread, 0, sizeof(HANDLE) * (thread_num));

        for (int j = 0; j <= i; j++)
        {
            hTread[j] = (HANDLE)_beginthreadex(NULL, 0, SRWLockIncrease, &i, CREATE_SUSPENDED, &dummy);
        }

        QueryPerformanceCounter(&start);

        for (int j = 0; j <= i; j++)
        {
            ResumeThread(hTread[j]);
        }

        WaitForMultipleObjects((i + 1), hTread, true, INFINITE);

        QueryPerformanceCounter(&end);

        for (int j = 0; j <= i; j++)
        {
            CloseHandle(hTread[j]);
        }

        printf("SRWLOCKTest \n");
        printf("target value= %lld\n ", *target);
        printf("Thread num = %d DeltaTime = %f seconds\n", i + 1, (end.QuadPart - start.QuadPart) / (float)timer.QuadPart);
    }

}

void MutexTest()
{
    LARGE_INTEGER start, end;
    float DeltaTime;

    for (int i = 0; i < thread_num; i++)
    {
        *target = 0;
        memset(hTread, 0, sizeof(HANDLE) * (thread_num));

        for (int j = 0; j <= i; j++)
        {
            hTread[j] = (HANDLE)_beginthreadex(NULL, 0, MutexIncrease, &i, CREATE_SUSPENDED, &dummy);
        }

        QueryPerformanceCounter(&start);

        for (int j = 0; j <= i; j++)
        {
            ResumeThread(hTread[j]);
        }

        WaitForMultipleObjects((i + 1), hTread, true, INFINITE);

        QueryPerformanceCounter(&end);

        for (int j = 0; j <= i; j++)
        {
            CloseHandle(hTread[j]);
        }

        printf("MutexTest\n");
        printf("target value= %lld\n ", *target);
        printf("Thread num = %d DeltaTime = %f seconds\n", i + 1, (end.QuadPart - start.QuadPart) / (float)timer.QuadPart);

    }

}

UINT __stdcall MemoryPoolThread(void* arg)
{
    int x = *reinterpret_cast<int*>(arg);

 
    test** pData = (test**)malloc(sizeof(test*) * PoolListGoal/(x+1));



    for (int i = 0; i < PoolListGoal / (2*(x + 1)); i++)
    {
        pData[i]=MemPool.Alloc();   
    }

 

    for (int i = 0; i < PoolListGoal / (2 * (x + 1)); i++)
    {
          MemPool.Free(pData[i]);

    }

    for (int i = 0; i < PoolListGoal / (2 * (x + 1)); i++)
    {

        pData[i] = MemPool.Alloc();


    }

    for (int i = 0; i < PoolListGoal / (2 * (x + 1)); i++)
    {
        MemPool.Free(pData[i]);

    }


    free(pData);


    return 0;
}

UINT __stdcall FreeListThread(void* arg)
{
    int x = *reinterpret_cast<int*>(arg);

    test** pData = (test**)malloc(sizeof(test*) * PoolListGoal / (x + 1));

    for (int i = 0; i < PoolListGoal / (2* (x + 1)); i++)
    {

        pData[i] = FreeList.Alloc();
    }


    for (int i = 0; i < PoolListGoal / (2 * (x + 1)); i++)
    {
        FreeList.Free(pData[i]);

    }

    for (int i = 0; i < PoolListGoal / (2 * (x + 1)); i++)
    {

        pData[i] = FreeList.Alloc();
    }

    for (int i = 0; i < PoolListGoal / (2 * (x + 1)); i++)
    {
        FreeList.Free(pData[i]);

    }



    free(pData);

    return 0;
}

UINT __stdcall LockFreeListThread(void* arg)
{
    int x = *reinterpret_cast<int*>(arg);

    test** pData = (test**)malloc(sizeof(test*) * PoolListGoal / (x + 1));
    
    for (int i = 0; i < PoolListGoal / (2 * (x + 1)); i++)
    {
       LockFreeList.Enqueue();
    }


    for (int i = 0; i < PoolListGoal / (2 * (x + 1)); i++)
    {
        pData[i] = (test*)malloc(sizeof(test));
        LockFreeList.Dequeue(pData[i]);       
    }

    for (int i = 0; i < PoolListGoal / (2 * (x + 1)); i++)
    {
        LockFreeList.Enqueue();
    }

    for (int i = 0; i < PoolListGoal / (2 * (x + 1)); i++)
    {
        pData[i] = (test*)malloc(sizeof(test));
        LockFreeList.Dequeue(pData[i]);
    }


    free(pData);

    return 0;
}


UINT __stdcall moodycamelQueueThread(void* arg)
{
    int x = *reinterpret_cast<int*>(arg);

    test** pData = (test**)malloc(sizeof(test*) * PoolListGoal / (x + 1));

    for (int i = 0; i < PoolListGoal / (2 * (x + 1)); i++)
    {
        pData[i] = (test*)malloc(sizeof(test));
        new(pData[i]) test;
        moodycamel_queue.enqueue(*pData[i]);
    }


    for (int i = 0; i < PoolListGoal / (2 * (x + 1)); i++)
    {
    
        moodycamel_queue.try_dequeue(*pData[i]);
    }

    for (int i = 0; i < PoolListGoal / (2 * (x + 1)); i++)
    {
        pData[i] = (test*)malloc(sizeof(test));
        new(pData[i]) test;
        moodycamel_queue.enqueue(*pData[i]);
    }

    for (int i = 0; i < PoolListGoal / (2 * (x + 1)); i++)
    {
   
        moodycamel_queue.try_dequeue(*pData[i]);
    }


    free(pData);

    return 0;
}




void MemoryPoolTest()
{
    
    LARGE_INTEGER start, end;
    float DeltaTime;

    for (int i = 0; i < thread_num; i++)
    {
        *target = 0;
        memset(hTread, 0, sizeof(HANDLE) * (thread_num));

        MemPool.Clear();



        for (int j = 0; j <= i; j++)
        {
            hTread[j] = (HANDLE)_beginthreadex(NULL, 0, MemoryPoolThread, &i, CREATE_SUSPENDED, &dummy);
        }

        QueryPerformanceCounter(&start);

        for (int j = 0; j <= i; j++)
        {
            ResumeThread(hTread[j]);
        }

        WaitForMultipleObjects((i + 1), hTread, true, INFINITE);

        QueryPerformanceCounter(&end);

        for (int j = 0; j <= i; j++)
        {
            CloseHandle(hTread[j]);
        }

        printf("MemoryPoolTest\n");
        printf("target value= %d capacity = %d\n ", MemPool.GetUseCount(),MemPool.GetCapacityCount());
        printf("Thread num = %d DeltaTime = %f seconds\n", i + 1, (end.QuadPart - start.QuadPart) / (float)timer.QuadPart);

    }
}

void FreeListTest()
{

    LARGE_INTEGER start, end;
    float DeltaTime;

    for (int i = 0; i < thread_num; i++)
    {
        *target = 0;
        memset(hTread, 0, sizeof(HANDLE) * (thread_num));

        FreeList.Clear();

        for (int j = 0; j <= i; j++)
        {
            hTread[j] = (HANDLE)_beginthreadex(NULL, 0, FreeListThread, &i, CREATE_SUSPENDED, &dummy);
        }

        QueryPerformanceCounter(&start);

        for (int j = 0; j <= i; j++)
        {
            ResumeThread(hTread[j]);
        }

        WaitForMultipleObjects((i + 1), hTread, true, INFINITE);

        QueryPerformanceCounter(&end);

        for (int j = 0; j <= i; j++)
        {
            CloseHandle(hTread[j]);
        }

        printf("FreeListTest\n");
        printf("target value= %d capacity = %d\n ", FreeList.GetUseCount(), FreeList.GetCapacityCount());
        printf("Thread num = %d DeltaTime = %f seconds\n", i + 1, (end.QuadPart - start.QuadPart) / (float)timer.QuadPart);

    }




}

void moodycamelQueue()
{
    
    LARGE_INTEGER start, end;
    float DeltaTime;

    for (int i = 0; i < thread_num; i++)
    {
        *target = 0;
        memset(hTread, 0, sizeof(HANDLE) * (thread_num));

        //  LockFreeList.Clear();

        for (int j = 0; j <= i; j++)
        {
            hTread[j] = (HANDLE)_beginthreadex(NULL, 0, moodycamelQueueThread, &i, CREATE_SUSPENDED, &dummy);
        }

        QueryPerformanceCounter(&start);

        for (int j = 0; j <= i; j++)
        {
            ResumeThread(hTread[j]);
        }

        WaitForMultipleObjects((i + 1), hTread, true, INFINITE);

        QueryPerformanceCounter(&end);

        for (int j = 0; j <= i; j++)
        {
            CloseHandle(hTread[j]);
        }

        printf("moodycamelQueueTest\n");
//        printf("target value= %d capacity = %d\n ", LockFreeList.GetUsingCount(), LockFreeList.GetAllocCount());
        printf("Thread num = %d DeltaTime = %f seconds\n", i + 1, (end.QuadPart - start.QuadPart) / (float)timer.QuadPart);

    }



}

void LockFreeQueueTest()
{

    LARGE_INTEGER start, end;
    float DeltaTime;

    for (int i = 0; i < thread_num; i++)
    {
        *target = 0;
        memset(hTread, 0, sizeof(HANDLE) * (thread_num));

      //  LockFreeList.Clear();

        for (int j = 0; j <= i; j++)
        {
            hTread[j] = (HANDLE)_beginthreadex(NULL, 0, LockFreeListThread, &i, CREATE_SUSPENDED, &dummy);
        }

        QueryPerformanceCounter(&start);

        for (int j = 0; j <= i; j++)
        {
            ResumeThread(hTread[j]);
        }

        WaitForMultipleObjects((i + 1), hTread, true, INFINITE);

        QueryPerformanceCounter(&end);

        for (int j = 0; j <= i; j++)
        {
            CloseHandle(hTread[j]);
        }

        printf("LockFreeQueueTest\n");
        printf("target value= %lld capacity = %lld\n ", LockFreeList.GetUsingCount(), LockFreeList.GetAllocCount());
        printf("Thread num = %d DeltaTime = %f seconds\n", i + 1, (end.QuadPart - start.QuadPart) / (float)timer.QuadPart);
       
    }

}


UINT __stdcall TLSPoolThread(void* arg)
{
    int x = *reinterpret_cast<int*>(arg);

    test** pData = (test**)malloc(sizeof(test*) * PoolListGoal / (x + 1));

    for (int i = 0; i < PoolListGoal / (2 * (x + 1)); i++)
    {
        pData[i] = TLSPool.Alloc();

    }


    for (int i = 0; i < PoolListGoal / (2 * (x + 1)); i++)
    {

        TLSPool.Free(pData[i]);
    }

    for (int i = 0; i < PoolListGoal / (2 * (x + 1)); i++)
    {
        pData[i] = TLSPool.Alloc();
    }

    for (int i = 0; i < PoolListGoal / (2 * (x + 1)); i++)
    {

        TLSPool.Free(pData[i]);
    }


    free(pData);

    return 0;
}



void TLSPoolTest()
{

    LARGE_INTEGER start, end;
    float DeltaTime;

    for (int i = 0; i < thread_num; i++)
    {
        *target = 0;
        memset(hTread, 0, sizeof(HANDLE) * (thread_num));

        TLSPool.Clear(i+1, PoolListGoal / (i + 1));

        for (int j = 0; j <= i; j++)
        {
            hTread[j] = (HANDLE)_beginthreadex(NULL, 0, TLSPoolThread, &i, CREATE_SUSPENDED, &dummy);
        }

        QueryPerformanceCounter(&start);

        for (int j = 0; j <= i; j++)
        {
            ResumeThread(hTread[j]);
        }

        WaitForMultipleObjects((i + 1), hTread, true, INFINITE);

        QueryPerformanceCounter(&end);

        for (int j = 0; j <= i; j++)
        {
            CloseHandle(hTread[j]);
        }

        printf("TLSPoolTest\n");
        printf("target value= %lld capacity = %lld\n ", TLSPool.GetWholUseSize(), TLSPool.GetWholeAllocSize());
        printf("Thread num = %d DeltaTime = %f seconds\n", i + 1, (end.QuadPart - start.QuadPart) / (float)timer.QuadPart);

    }




}






UINT __stdcall HashMapTestThread(void* arg)
{
    int x = *reinterpret_cast<int*>(arg);

    DWORD p = timeGetTime();
  
    unsigned int tmp =10000* GetCurrentThreadId();
 
 
    for (int i = tmp; i < tmp+PoolListGoal / (x + 1); i++)
    {
        if (p % 100 == 0) p++;   
        HashMap.insert(i, p);
    }


    return 0;
}




void HashMapTest()
{



    LARGE_INTEGER start, end;
    float DeltaTime;

    for (int i = 0; i < thread_num; i++)
    {


        *target = 0;
        memset(hTread, 0, sizeof(HANDLE) * (thread_num));

          HashMap.Clear();

        for (int j = 0; j <= i; j++)
        {
            hTread[j] = (HANDLE)_beginthreadex(NULL, 0, HashMapTestThread, &i, CREATE_SUSPENDED, &dummy);
        }

        QueryPerformanceCounter(&start);

        for (int j = 0; j <= i; j++)
        {
            ResumeThread(hTread[j]);
        }

        WaitForMultipleObjects((i + 1), hTread, true, INFINITE);

        QueryPerformanceCounter(&end);

        for (int j = 0; j <= i; j++)
        {
            CloseHandle(hTread[j]);
        }

        printf("HashMapTest\n");
        printf("capacity = %d\n ", HashMap.size());
        printf("Thread num = %d DeltaTime = %f seconds\n", i + 1, (end.QuadPart - start.QuadPart) / (float)timer.QuadPart);

    }




}




int main()
{
    
    
    QueryPerformanceFrequency(&timer);
    
    goal = 1 * 4 * 5 * 6 * 7  * 9 * 10* 11 * 12;
  
    
    std::cout << "Thread Number = " << std::thread::hardware_concurrency() << "  Thread " << std::endl;

    InitializeCriticalSection(&critical_section);

    CriticalSectionTest();

    DeleteCriticalSection(&critical_section);

    InterLockedTest();

    InitializeSRWLock(&srw_lock);

    SRWLockTest();

    MutexTest();
   
  //  MemoryPoolTest();

    

 //   FreeListTest();

  //  LockFreeQueueTest();

 //   moodycamelQueue();

  //  HashMapTest();
//    MemoryPoolTest();
   
  //  TLSPoolTest();
    

};

