using System;

namespace IPC.Bond.Managed
{
    public class Disposable<T> : IDisposable
        where T : class, IDisposable
    {
        private T _impl;

        internal Disposable(T impl)
        {
            _impl = impl;
        }

        internal T Impl
        {
            get { return _impl; }
        }

        protected virtual void Dispose(bool disposing)
        {
            if (_impl != null)
            {
                if (disposing)
                {
                    _impl.Dispose();
                }

                _impl = null;
            }
        }

        public void Dispose()
        {
            Dispose(true);
        }
    }
}
