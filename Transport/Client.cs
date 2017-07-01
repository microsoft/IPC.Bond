using System;
using System.Threading.Tasks;

namespace IPC.Bond.Managed
{
    using IPC.Managed;

    public partial class Transport<Request, Response>
    {
        public class Client : Component, IClient<Request, Response>
        {
            private readonly Serializer _serializer;

            internal Client(IClient<BufferPool.ConstBuffer, BufferPool.ConstBuffer> impl, Serializer serializer)
                : base(impl)
            {
                _serializer = serializer;
            }

            /// <remarks>The caller is responsible for disposing the <param name="allocator"/></remarks>
            public async Task<Response> InvokeAsync(Request request, TimeSpan timeout = default(TimeSpan))
            {
                Task<BufferPool.ConstBuffer> responseBufferTask;

                using (var requestBuffer = _serializer.Serialize(request))
                {
                    responseBufferTask = Impl.InvokeAsync(requestBuffer, timeout);
                }

                using (var responseBuffer = await responseBufferTask)
                {
                    return _serializer.Deserialize<Response>(responseBuffer);
                }
            }

            /// <remarks>The supplied allocator will be reclaimed by GC.</remarks>
            Task<Response> IClient<Request, Response>.InvokeAsync(Request request, TimeSpan timeout)
            {
                return InvokeAsync(request, timeout);
            }

            private new IClient<BufferPool.ConstBuffer, BufferPool.ConstBuffer> Impl
            {
                get { return (IClient<BufferPool.ConstBuffer, BufferPool.ConstBuffer>)base.Impl; }
            }
        }
    }
}
