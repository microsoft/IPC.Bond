#include "IPC/Managed/detail/TransportImpl.h"
#include "BufferPool.h"


namespace IPC
{
namespace Managed
{
    namespace detail
    {
        template <>
        struct Convert<IPC::Bond::DefaultConstBuffer>
        {
            using type = IPC::Bond::Managed::BufferPool::ConstBuffer^;

            static const IPC::Bond::DefaultConstBuffer& From(type% from)
            {
                return *from->Impl;
            }
        };

        template <>
        struct Convert<IPC::Bond::Managed::BufferPool::ConstBuffer^>
        {
            using type = IPC::Bond::DefaultConstBuffer;

            static IPC::Bond::Managed::BufferPool::ConstBuffer^ From(const type& from)
            {
                return gcnew IPC::Bond::Managed::BufferPool::ConstBuffer{ from };
            }
        };

        template Transport<IPC::Bond::Managed::BufferPool::ConstBuffer^, IPC::Bond::Managed::BufferPool::ConstBuffer^>;

    } // detail

} // Managed
} // IPC
