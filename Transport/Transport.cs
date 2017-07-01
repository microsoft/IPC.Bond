using System;

namespace IPC.Bond.Managed
{
    using IPC.Managed;

    public partial class Transport<Request, Response> :
        Disposable<ITransport<BufferPool.ConstBuffer, BufferPool.ConstBuffer>>,
        ITransport<Request, Response>
        where Request : new()
        where Response : new()
    {
        private static readonly TransportFactory _factory;

        private readonly Config _config;

        static Transport()
        {
            _factory = new TransportFactory();
            _factory.Register(typeof(BufferPool).Assembly);
        }

        public Transport()
            : this(new Config())
        { }

        public Transport(Config config)
            : base(_factory.Make<BufferPool.ConstBuffer, BufferPool.ConstBuffer>(config))
        {
            _config = config;
        }

        public ClientConnector MakeClientConnector()
        {
            return new ClientConnector(Impl.MakeClientConnector(), GetSerializerMaker());
        }

        IClientConnector<Request, Response> ITransport<Request, Response>.MakeClientConnector()
        {
            return MakeClientConnector();
        }

        public ServerAcceptor MakeServerAcceptor(string name, HandlerFactory<Request, Response> handlerFactory)
        {
            return new ServerAcceptor(Impl.MakeServerAcceptor(name, MakeHandlerFactory(handlerFactory)));
        }

        IServerAcceptor<Request, Response> ITransport<Request, Response>.MakeServerAcceptor(string name, HandlerFactory<Request, Response> handlerFactory)
        {
            return MakeServerAcceptor(name, handlerFactory);
        }

        public ClientAccessor ConnectClient(string name, bool async, TimeSpan timeout = default(TimeSpan), ClientConnector connector = null)
        {
            return new ClientAccessor(Impl.ConnectClient(name, async, timeout, connector?.Impl), GetSerializerMaker());
        }

        IClientAccessor<Request, Response> ITransport<Request, Response>.ConnectClient(
            string name, bool async, TimeSpan timeout, IClientConnector<Request, Response> connector)
        {
            return ConnectClient(name, async, timeout, connector as ClientConnector);
        }

        public ServersAccessor AcceptServers(string name, HandlerFactory<Request, Response> handlerFactory)
        {
            return new ServersAccessor(Impl.AcceptServers(name, MakeHandlerFactory(handlerFactory)));
        }

        IServersAccessor<Request, Response> ITransport<Request, Response>.AcceptServers(string name, HandlerFactory<Request, Response> handlerFactory)
        {
            return AcceptServers(name, handlerFactory);
        }

        private Func<IComponent, Serializer> GetSerializerMaker()
        {
            return component => MakeSerializer(component.InputMemory, component.OutputMemory);
        }

        private Serializer MakeSerializer(SharedMemory inputMemory, SharedMemory outputMemory)
        {
            return new Serializer(_config.ProtocolType, _config.Marshal, new BufferPool(outputMemory), inputMemory, _config.MinBlobSize);
        }

        private HandlerFactory<BufferPool.ConstBuffer, BufferPool.ConstBuffer> MakeHandlerFactory(HandlerFactory<Request, Response> handlerFactory)
        {
            return (inputMemory, outputMemory) =>
            {
                var handler = handlerFactory(inputMemory, outputMemory);
                var serializer = MakeSerializer(inputMemory, outputMemory);

                return async (requestBuffer) =>
                {
                    Request request;

                    using (requestBuffer)
                    {
                        request = serializer.Deserialize<Request>(requestBuffer);
                    }

                    return serializer.Serialize(await handler(request));
                };
            };
        }
    }
}
