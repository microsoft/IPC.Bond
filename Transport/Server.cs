using System;

namespace IPC.Bond.Managed
{
    using IPC.Managed;

    public partial class Transport<Request, Response>
    {
        public class Server : Component, IServer<Request, Response>
        {
            internal Server(IServer<BufferPool.ConstBuffer, BufferPool.ConstBuffer> impl)
                : base(impl)
            {
                impl.Error += OnError;
            }

            public event EventHandler<ErrorEventArgs> Error;

            private void OnError(object sender, ErrorEventArgs args)
            {
                var error = Error;
                error?.Invoke(this, args);
            }
        }
    }
}
