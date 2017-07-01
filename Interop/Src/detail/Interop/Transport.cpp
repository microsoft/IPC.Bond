#include "stdafx.h"
#include "IPC/Managed/detail/Interop/TransportImpl.h"
#include <IPC/Bond/BufferPool.h>


namespace IPC
{
namespace Managed
{
    namespace detail
    {
    namespace Interop
    {
        template Transport<IPC::Bond::DefaultConstBuffer, IPC::Bond::DefaultConstBuffer>;

    } // Interop
    } // detail

} // Managed
} // IPC
