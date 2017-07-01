using System;

namespace IPC.Bond.Managed
{
    using IPC.Managed;

    public partial class Transport<Request, Response>
    {
        public class ServerAcceptor :
            Disposable<IServerAcceptor<BufferPool.ConstBuffer, BufferPool.ConstBuffer>>,
            IServerAcceptor<Request, Response>
        {
            internal ServerAcceptor(IServerAcceptor<BufferPool.ConstBuffer, BufferPool.ConstBuffer> impl)
                : base(impl)
            {
                impl.Accepted += OnAccepted;
                impl.Error += OnError;
            }

            public event EventHandler<ComponentEventArgs<Server>> Accepted;

            private event EventHandler<ComponentEventArgs<IServer<Request, Response>>> AcceptedInternal;
            event EventHandler<ComponentEventArgs<IServer<Request, Response>>> IServerAcceptor<Request, Response>.Accepted
            {
                add { AcceptedInternal += value; }
                remove { AcceptedInternal -= value; }
            }

            public event EventHandler<ErrorEventArgs> Error;

            private void OnAccepted(object sender, ComponentEventArgs<IServer<BufferPool.ConstBuffer, BufferPool.ConstBuffer>> args)
            {
                var server = new Server(args.Component);

                var accepted = Accepted;
                accepted?.Invoke(this, new ComponentEventArgs<Server>(server));

                var acceptedInternal = AcceptedInternal;
                acceptedInternal?.Invoke(this, new ComponentEventArgs<IServer<Request, Response>>(server));
            }

            private void OnError(object sender, ErrorEventArgs args)
            {
                var error = Error;
                error?.Invoke(this, args);
            }
        }
    }
}
