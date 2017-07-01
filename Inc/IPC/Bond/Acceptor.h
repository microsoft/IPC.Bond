#pragma once

#include <IPC/Acceptor.h>
#include "detail/ComponentBase.h"
#include "DefaultTraits.h"


namespace IPC
{
namespace Bond
{
    template <typename Request, typename Response, typename Traits = DefaultTraits>
    using ClientAcceptor = detail::BufferComponent<ClientAcceptor, Request, Response, Traits>;

    template <typename Request, typename Response, typename Traits = DefaultTraits>
    using ServerAcceptor = detail::BufferComponent<ServerAcceptor, Request, Response, Traits>;

} // Bond
} // IPC
