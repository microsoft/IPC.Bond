using System;
using System.Threading.Tasks;

namespace IPC.Bond.Managed
{
    using IPC.Managed;

    public partial class Transport<Request, Response>
    {
        public class ClientConnector :
            Disposable<IClientConnector<BufferPool.ConstBuffer, BufferPool.ConstBuffer>>,
            IClientConnector<Request, Response>
        {
            private readonly Func<IComponent, Serializer> _serializerMaker;

            internal ClientConnector(
                IClientConnector<BufferPool.ConstBuffer, BufferPool.ConstBuffer> impl,
                Func<IComponent, Serializer> serializerMaker)
                : base(impl)
            {
                _serializerMaker = serializerMaker;
            }

            public async Task<Client> ConnectAsync(string acceptorName, TimeSpan timeout = default(TimeSpan))
            {
                var client = await Impl.ConnectAsync(acceptorName, timeout);
                return new Client(client, _serializerMaker(client));
            }

            async Task<IClient<Request, Response>> IClientConnector<Request, Response>.ConnectAsync(string acceptorName, TimeSpan timeout)
            {
                return await ConnectAsync(acceptorName, timeout);
            }
        }
    }
}
