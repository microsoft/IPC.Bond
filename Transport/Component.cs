using System;

namespace IPC.Bond.Managed
{
    using IPC.Managed;

    public partial class Transport<Request, Response>
    {
        public abstract class Component : Disposable<IComponent>, IComponent
        {
            internal Component(IComponent impl)
                : base(impl)
            {
                impl.Closed += OnClosed;
            }

            public SharedMemory InputMemory
            {
                get { return Impl.InputMemory; }
            }

            public SharedMemory OutputMemory
            {
                get { return Impl.OutputMemory; }
            }

            public bool IsClosed
            {
                get { return Impl.IsClosed; }
            }

            public event EventHandler Closed;

            public void Close()
            {
                Impl.Close();
            }

            private void OnClosed(object sender, EventArgs args)
            {
                var closed = Closed;
                closed?.Invoke(this, args);
            }
        }
    }
}
