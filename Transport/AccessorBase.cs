using System;

namespace IPC.Bond.Managed
{
    using IPC.Managed;

    public abstract class AccessorBase<T, Interface, InterfaceImpl, AccessorImpl> : Disposable<AccessorImpl>, IAccessor<Interface>
        where Interface : class, IComponent
        where InterfaceImpl : class, IComponent
        where T : Interface
        where AccessorImpl : class, IAccessor<InterfaceImpl>
    {
        internal AccessorBase(AccessorImpl impl)
            : base(impl)
        {
            impl.Connected += OnConnected;
            impl.Disconnected += OnDisconnected;
            impl.Error += OnError;
        }

        protected abstract T ConnectComponent(InterfaceImpl impl);

        protected abstract T DisconnectComponent(InterfaceImpl impl);

        public event EventHandler<ComponentEventArgs<T>> Connected;

        private event EventHandler<ComponentEventArgs<Interface>> ConnectedInternal;
        event EventHandler<ComponentEventArgs<Interface>> IAccessor<Interface>.Connected
        {
            add { ConnectedInternal += value; }
            remove { ConnectedInternal -= value; }
        }

        public event EventHandler<ComponentEventArgs<T>> Disconnected;

        private event EventHandler<ComponentEventArgs<Interface>> DisconnectedInternal;
        event EventHandler<ComponentEventArgs<Interface>> IAccessor<Interface>.Disconnected
        {
            add { DisconnectedInternal += value; }
            remove { DisconnectedInternal -= value; }
        }

        public event EventHandler<ErrorEventArgs> Error;

        private void OnConnected(object sender, ComponentEventArgs<InterfaceImpl> args)
        {
            var component = ConnectComponent(args.Component);

            var connected = Connected;
            connected?.Invoke(this, new ComponentEventArgs<T>(component));

            var connectedInternal = ConnectedInternal;
            connectedInternal?.Invoke(this, new ComponentEventArgs<Interface>(component));
        }

        private void OnDisconnected(object sender, ComponentEventArgs<InterfaceImpl> args)
        {
            var component = DisconnectComponent(args.Component);

            var disconnected = Disconnected;
            disconnected?.Invoke(this, new ComponentEventArgs<T>(component));

            var disconnectedInternal = DisconnectedInternal;
            disconnectedInternal?.Invoke(this, new ComponentEventArgs<Interface>(component));
        }

        private void OnError(object sender, ErrorEventArgs args)
        {
            var error = Error;
            error?.Invoke(this, args);
        }
    }
}
