namespace IPC.Bond.Managed
{
    using IPC.Managed;

    public class Serializer
    {
        private readonly BufferPool _pool;
        private readonly SharedMemory _inputMemory;
        private readonly uint _minBlobSize;
        private readonly ISerializer<InputStream, OutputStream> _serializer;

        public Serializer(global::Bond.ProtocolType protocol, bool marshal, BufferPool pool, SharedMemory inputMemory, uint minBlobSize = 0)
            : this(SerializerFactory.Create<InputStream, OutputStream>(protocol, marshal), pool, inputMemory, minBlobSize)
        { }

        public Serializer(ISerializer<InputStream, OutputStream> serializer, BufferPool pool, SharedMemory inputMemory, uint minBlobSize = 0)
        {
            _pool = pool;
            _inputMemory = inputMemory;
            _minBlobSize = minBlobSize;
            _serializer = serializer;
        }

        public global::Bond.ProtocolType ProtocolType
        {
            get { return _serializer.ProtocolType; }
        }

        public bool IsMarshaled
        {
            get { return _serializer.IsMarshaled; }
        }

        public BufferPool.ConstBuffer Serialize<T>(T obj)
        {
            using (var output = new OutputStream(_pool, _minBlobSize))
            {
                _serializer.Serialize(output, obj);
                return output.GetBuffer();
            }
        }

        public T Deserialize<T>(BufferPool.ConstBuffer buffer)
        {
            using (var input = new InputStream(buffer, _inputMemory))
            {
                return _serializer.Deserialize<T>(input);
            }
        }
    }
}
