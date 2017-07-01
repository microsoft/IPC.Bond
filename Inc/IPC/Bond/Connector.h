#pragma once

#include <IPC/Connector.h>
#include "detail/ComponentBase.h"
#include "DefaultTraits.h"


namespace IPC
{
namespace Bond
{
    template <typename Request, typename Response, typename Traits = DefaultTraits>
    using ClientConnector = detail::BufferComponent<ClientConnector, Request, Response, Traits>;

    template <typename Request, typename Response, typename Traits = DefaultTraits>
    using ServerConnector = detail::BufferComponent<ServerConnector, Request, Response, Traits>;

} // Bond
} // IPC
