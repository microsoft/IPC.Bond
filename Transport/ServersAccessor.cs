using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;

namespace IPC.Bond.Managed
{
    using IPC.Managed;

    public partial class Transport<Request, Response>
    {
        public class ServersAccessor :
            AccessorBase<
                Server,
                IServer<Request, Response>,
                IServer<BufferPool.ConstBuffer, BufferPool.ConstBuffer>,
                IServersAccessor<BufferPool.ConstBuffer, BufferPool.ConstBuffer>>,
            IServersAccessor<Request, Response>
        {
            internal ServersAccessor(IServersAccessor<BufferPool.ConstBuffer, BufferPool.ConstBuffer> impl)
                : base(impl)
            { }

            public IReadOnlyCollection<Server> Servers
            {
                get
                {
                    return new ReadOnlyCollection<Server>(Impl.Servers.Select(server => new Server(server)).ToList());
                }
            }

            IReadOnlyCollection<IServer<Request, Response>> IServersAccessor<Request, Response>.Servers
            {
                get { return Servers; }
            }

            protected override Server ConnectComponent(IServer<BufferPool.ConstBuffer, BufferPool.ConstBuffer> impl)
            {
                return new Server(impl);
            }

            protected override Server DisconnectComponent(IServer<BufferPool.ConstBuffer, BufferPool.ConstBuffer> impl)
            {
                return new Server(impl);
            }
        }
    }
}
