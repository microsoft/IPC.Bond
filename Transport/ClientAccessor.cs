using System;
using System.Threading;

namespace IPC.Bond.Managed
{
    using IPC.Managed;

    public partial class Transport<Request, Response>
    {
        public class ClientAccessor :
            AccessorBase<
                Client,
                IClient<Request, Response>,
                IClient<BufferPool.ConstBuffer, BufferPool.ConstBuffer>,
                IClientAccessor<BufferPool.ConstBuffer, BufferPool.ConstBuffer>>,
            IClientAccessor<Request, Response>
        {
            private readonly Func<IComponent, Serializer> _serializerMaker;
            private Client _client;

            internal ClientAccessor(
                IClientAccessor<BufferPool.ConstBuffer, BufferPool.ConstBuffer> impl,
                Func<IComponent, Serializer> serializerMaker)
                : base(impl)
            {
                _serializerMaker = serializerMaker;
            }

            public Client Client
            {
                get
                {
                    return _client ?? ConnectComponent(Impl.Client);
                }
            }

            IClient<Request, Response> IClientAccessor<Request, Response>.Client
            {
                get { return Client; }
            }

            protected override Client ConnectComponent(IClient<BufferPool.ConstBuffer, BufferPool.ConstBuffer> impl)
            {
                var client = new Client(impl, _serializerMaker(impl));

                Interlocked.Exchange(ref _client, client);

                return client;
            }

            protected override Client DisconnectComponent(IClient<BufferPool.ConstBuffer, BufferPool.ConstBuffer> impl)
            {
                return Interlocked.Exchange(ref _client, null);
            }
        }
    }
}
