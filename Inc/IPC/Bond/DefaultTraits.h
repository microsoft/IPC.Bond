#pragma once

#include <IPC/DefaultTraits.h>
#include "BufferPool.h"
#include "Serializer.h"


namespace IPC
{
namespace Bond
{
    struct DefaultTraits : IPC::DefaultTraits
    {
        using BufferPool = DefaultBufferPool;

        using Serializer = DefaultSerializer;
    };

} // Bond
} // IPC
