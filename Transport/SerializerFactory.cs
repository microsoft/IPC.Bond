namespace IPC.Bond.Managed
{
    using System;
    using System.Linq.Expressions;
    using IInputStream = global::Bond.IO.IInputStream;
    using IOutputStream = global::Bond.IO.IOutputStream;

    public interface ISerializer<Input, Output>
        where Input : IInputStream, global::Bond.IO.ICloneable<Input>
        where Output : IOutputStream
    {
        global::Bond.ProtocolType ProtocolType { get; }

        bool IsMarshaled { get; }

        void Serialize<T>(Output output, T obj);

        T Deserialize<T>(Input input);
    }

    internal static class ProtocolFactory<Protocol>
    {
        private static class Factory<Stream>
        {
            public static readonly Func<Stream, Protocol> Instance = new Func<Func<Stream, Protocol>>(() =>
                {
                    var stream = Expression.Parameter(typeof(Stream));

                    return Expression.Lambda<Func<Stream, Protocol>>(
                        Expression.New(typeof(Protocol).GetConstructor(new[] { typeof(Stream), typeof(ushort) }), stream, Expression.Constant((ushort)1)),
                        stream).Compile();
                })();
        }

        public static Protocol Make<Stream>(Stream stream)
        {
            return Factory<Stream>.Instance(stream);
        }
    }

    internal struct Marshaler<Input, Output, Writer> : ISerializer<Input, Output>
        where Input : IInputStream, global::Bond.IO.ICloneable<Input>
        where Output : IOutputStream
        where Writer : global::Bond.Protocols.IProtocolWriter
    {
        public Marshaler(global::Bond.ProtocolType protocol)
        {
            ProtocolType = protocol;
        }

        public global::Bond.ProtocolType ProtocolType { get; }

        public bool IsMarshaled
        {
            get { return true; }
        }

        public void Serialize<T>(Output output, T obj)
        {
            global::Bond.Marshal.To(ProtocolFactory<Writer>.Make(output), obj);
        }

        public T Deserialize<T>(Input input)
        {
            return global::Bond.Unmarshal<T>.From(input);
        }
    }

    internal struct Serializer<Input, Reader, Output, Writer> : ISerializer<Input, Output>
        where Input : IInputStream, global::Bond.IO.ICloneable<Input>
        where Output : IOutputStream
        where Writer : global::Bond.Protocols.IProtocolWriter
    {
        public Serializer(global::Bond.ProtocolType protocol)
        {
            ProtocolType = protocol;
        }

        public global::Bond.ProtocolType ProtocolType { get; }

        public bool IsMarshaled
        {
            get { return false; }
        }

        public void Serialize<T>(Output output, T obj)
        {
            global::Bond.Serialize.To(ProtocolFactory<Writer>.Make(output), obj);
        }

        public T Deserialize<T>(Input input)
        {
            return global::Bond.Deserialize<T>.From(ProtocolFactory<Reader>.Make(input));
        }
    }

    public static class SerializerFactory
    {
        public static ISerializer<Input, Output> Create<Input, Output>(global::Bond.ProtocolType protocol, bool marshal)
            where Input : IInputStream, global::Bond.IO.ICloneable<Input>
            where Output : IOutputStream
        {
            switch (protocol)
            {
                case global::Bond.ProtocolType.FAST_PROTOCOL:
                    return marshal
                        ? (ISerializer<Input, Output>)new Marshaler<Input, Output, global::Bond.Protocols.FastBinaryWriter<Output>>(protocol)
                        : new Serializer<Input, global::Bond.Protocols.FastBinaryReader<Input>, Output, global::Bond.Protocols.FastBinaryWriter<Output>>(protocol);

                case global::Bond.ProtocolType.COMPACT_PROTOCOL:
                    return marshal
                        ? (ISerializer<Input, Output>)new Marshaler<Input, Output, global::Bond.Protocols.CompactBinaryWriter<Output>>(protocol)
                        : new Serializer<Input, global::Bond.Protocols.CompactBinaryReader<Input>, Output, global::Bond.Protocols.CompactBinaryWriter<Output>>(protocol);

                case global::Bond.ProtocolType.SIMPLE_PROTOCOL:
                    return marshal
                        ? (ISerializer<Input, Output>)new Marshaler<Input, Output, global::Bond.Protocols.SimpleBinaryWriter<Output>>(protocol)
                        : new Serializer<Input, global::Bond.Protocols.SimpleBinaryReader<Input>, Output, global::Bond.Protocols.SimpleBinaryWriter<Output>>(protocol);
            }

            throw new IPC.Managed.Exception("Unknown protocol.");
        }
    }
}
