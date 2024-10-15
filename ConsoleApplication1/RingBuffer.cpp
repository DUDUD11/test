#pragma once

// 마지막부분인 size pointer의 경우 0과 같다고 볼수있다
// 접근불가능한곳을 가르키지만 사용하지는 않는다

#include "ringbuffer.h"
#include <memory.h>


RingBuffer::RingBuffer(void)
{
    Size = Ring_Buffer_Size;
    Buffer = new char[Size];

    UseSize = 0;
    FreeSize = Size;
    FrontPoint = Buffer;
    RearPoint = Buffer;


}

RingBuffer::~RingBuffer()
{
    delete[](Buffer);
}

RingBuffer::RingBuffer(int iBufferSize)
{
    if (iBufferSize <= 0) __debugbreak();
    Size = iBufferSize;
    UseSize = 0;
    FreeSize = iBufferSize;
    Buffer = new char[Size];
    FrontPoint = Buffer;
    RearPoint = Buffer;


}

void RingBuffer::Resize(int size)
{
    if (UseSize > size) __debugbreak();

    char* copy = new char[size];

    if (FrontPoint < RearPoint)
    {
        memcpy(copy, FrontPoint, UseSize);
    }

    else
    {

        if ((char*)(Buffer + Size) < (char*)(FrontPoint + UseSize))
        {
            int pos = (char*)(Buffer + Size) - (char*)(FrontPoint);
            memcpy(copy, FrontPoint, pos);
            memcpy((char*)(copy + pos), Buffer, UseSize - pos);
        }
        else
        {
            memcpy(copy, FrontPoint, UseSize);

        }


    }

    delete[](Buffer);

    FreeSize += size - Size;
    Size = size;
    Buffer = copy;
    FrontPoint = Buffer;
    RearPoint = (char*)(Buffer + UseSize);

}

int RingBuffer::GetBufferSize(void)
{
    return Size;
}

int RingBuffer::GetUseSize(void)
{
    return UseSize;
}

int RingBuffer::GetFreeSize(void)
{
    return FreeSize;
}

int RingBuffer::Enqueue(char* chpData, int iSize)
{

    int sz = iSize <= FreeSize ? iSize : FreeSize;
    UseSize += sz;
    FreeSize -= sz;

    int behind = (char*)(Buffer + Size) - RearPoint;

    if (behind >= sz)
    {
        memcpy(RearPoint, chpData, sz);
        RearPoint = (char*)(RearPoint + sz);
    }

    else
    {
        memcpy(RearPoint, chpData, behind);
        memcpy(Buffer, (char*)(chpData + behind), sz - behind);

        RearPoint = (char*)(Buffer + sz - behind);
    }

    return sz;
}

int RingBuffer::Dequeue(char* chpDest, int iSize)
{
    int sz = iSize <= UseSize ? iSize : UseSize;

    UseSize -= sz;
    FreeSize += sz;

    int behind = (char*)(Buffer + Size) - FrontPoint;

    if (behind >= sz)
    {
        memcpy(chpDest, FrontPoint, sz);
        FrontPoint = (char*)(FrontPoint + sz);
    }

    else
    {
        memcpy(chpDest, FrontPoint, behind);
        memcpy((char*)(chpDest + behind), Buffer, sz - behind);
        FrontPoint = (char*)(Buffer + sz - behind);
    }

    return sz;
}

int RingBuffer::Peek(char* chpDest, int iSize)
{
    int sz = iSize <= UseSize ? iSize : UseSize;

    int behind = (char*)(Buffer + Size) - FrontPoint;

    if (behind >= sz)
    {
        memcpy(chpDest, FrontPoint, sz);
    }

    else
    {
        memcpy(chpDest, FrontPoint, behind);
        memcpy((char*)(chpDest + behind), Buffer, sz - behind);
    }

    return sz;

}

void RingBuffer::ClearBuffer(void)
{

    UseSize = 0;
    FreeSize = Size;
    FrontPoint = Buffer;
    RearPoint = Buffer;

}

int RingBuffer::DirectEnqueueSize(void)
{
    int spare = (char*)(Buffer + Size) - RearPoint;
    if (spare <= GetFreeSize()) return spare;


    return GetFreeSize();
}

int RingBuffer::DirectDequeueSize(void)
{
    int spare = (char*)(Buffer + Size) - FrontPoint;
    if (spare <= GetUseSize()) return spare;
    return GetUseSize();
}

int RingBuffer::MoveRear(int iSize)
{
    int sz = iSize <= FreeSize ? iSize : FreeSize;
    UseSize += sz;
    FreeSize -= sz;

    int behind = (char*)(Buffer + Size) - RearPoint;

    if (behind >= sz)
    {
        RearPoint = (char*)(RearPoint + sz);
    }

    else
    {
        RearPoint = (char*)(Buffer + sz - behind);
    }

    return sz;
}

int RingBuffer::MoveFront(int iSize)
{
    int sz = iSize <= UseSize ? iSize : UseSize;

    UseSize -= sz;
    FreeSize += sz;

    int behind = (char*)(Buffer + Size) - FrontPoint;

    if (behind >= sz)
    {
        FrontPoint = (char*)(FrontPoint + sz);
    }

    else
    {
        FrontPoint = (char*)(Buffer + sz - behind);
    }

    return sz;
}

char* RingBuffer::GetFrontBufferPtr(void)
{
    if (FrontPoint == (char*)(Buffer + Size))
    {
        FrontPoint = Buffer;
    }

    return FrontPoint;
}

char* RingBuffer::GetRearBufferPtr(void)
{
    if (RearPoint == (char*)(Buffer + Size))
    {
        RearPoint = Buffer;
    }

    return RearPoint;
}
