#pragma once

#include "BufferPoolHolder.h"
#include "BufferPoolFwd.h"
#include <type_traits>


namespace IPC
{
namespace Bond
{
    namespace detail
    {
        template <template <typename, typename, typename> typename Component, typename Request, typename Response, typename Traits>
        using BufferComponentOf = Component<
            ConstBuffer<typename Traits::BufferPool>,
            std::conditional_t<std::is_void<Response>::value, void, ConstBuffer<typename Traits::BufferPool>>,
            Traits>;


        template <template <typename, typename, typename> typename Component, typename RequestT, typename ResponseT, typename TraitsT>
        class BufferComponent : public BufferComponentOf<Component, RequestT, ResponseT, TraitsT>
        {
            using Base = BufferComponentOf<Component, RequestT, ResponseT, TraitsT>;

        public:
            using Request = RequestT;
            using Response = ResponseT;
            using Traits = TraitsT;

            template <typename... Args>
            BufferComponent(Args&&... args)                 // TODO: Inherit constructors instead when VC14 bugs are fixed.
                : Base{ std::forward<Args>(args)... }
            {}
        };


        template <template <typename, typename, typename> typename Component, typename Request, typename Response, typename Traits>
        class ComponentBase
            : public BufferComponent<Component, Request, Response, Traits>,
              public BufferPoolHolder<typename Traits::BufferPool>,
              public Traits::Serializer
        {
            using BufferComponent = BufferComponent<Component, Request, Response, Traits>;

        protected:
            using BufferPoolHolder = BufferPoolHolder<typename Traits::BufferPool>;
            using Serializer = typename Traits::Serializer;

        public:
            template <typename... Args>
            ComponentBase(BufferPoolHolder pools, Serializer serializer, Args&&... args)
                : BufferComponent{ std::forward<Args>(args)... },
                  BufferPoolHolder{ std::move(pools) },
                  Serializer{ std::move(serializer) }
            {}
        };

    } // detail
} // Bond
} // IPC
