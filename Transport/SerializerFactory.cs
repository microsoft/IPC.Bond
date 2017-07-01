namespace IPC.Bond.Managed
{
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

    public struct CompactBinarySerializer<Input, Output> : ISerializer<Input, Output>
        where Input : IInputStream, global::Bond.IO.ICloneable<Input>
        where Output : IOutputStream
    {
        public global::Bond.ProtocolType ProtocolType
        {
            get { return global::Bond.ProtocolType.COMPACT_PROTOCOL; }
        }

        public bool IsMarshaled
        {
            get { return false; }
        }

        public void Serialize<T>(Output output, T obj)
        {
            global::Bond.Serialize.To(new global::Bond.Protocols.CompactBinaryWriter<Output>(output), obj);
        }

        public T Deserialize<T>(Input input)
        {
            return global::Bond.Deserialize<T>.From(new global::Bond.Protocols.CompactBinaryReader<Input>(input));
        }
    }

    public struct FastBinarySerializer<Input, Output> : ISerializer<Input, Output>
        where Input : IInputStream, global::Bond.IO.ICloneable<Input>
        where Output : IOutputStream
    {
        public global::Bond.ProtocolType ProtocolType
        {
            get { return global::Bond.ProtocolType.FAST_PROTOCOL; }
        }

        public bool IsMarshaled
        {
            get { return false; }
        }

        public void Serialize<T>(Output output, T obj)
        {
            global::Bond.Serialize.To(new global::Bond.Protocols.FastBinaryWriter<Output>(output), obj);
        }

        public T Deserialize<T>(Input input)
        {
            return global::Bond.Deserialize<T>.From(new global::Bond.Protocols.FastBinaryReader<Input>(input));
        }
    }

    public struct SimpleBinarySerializer<Input, Output> : ISerializer<Input, Output>
        where Input : IInputStream, global::Bond.IO.ICloneable<Input>
        where Output : IOutputStream
    {
        public global::Bond.ProtocolType ProtocolType
        {
            get { return global::Bond.ProtocolType.SIMPLE_PROTOCOL; }
        }

        public bool IsMarshaled
        {
            get { return false; }
        }

        public void Serialize<T>(Output output, T obj)
        {
            global::Bond.Serialize.To(new global::Bond.Protocols.SimpleBinaryWriter<Output>(output), obj);
        }

        public T Deserialize<T>(Input input)
        {
            return global::Bond.Deserialize<T>.From(new global::Bond.Protocols.SimpleBinaryReader<Input>(input));
        }
    }

    public struct CompactBinaryMarshaler<Input, Output> : ISerializer<Input, Output>
        where Input : IInputStream, global::Bond.IO.ICloneable<Input>
        where Output : IOutputStream
    {
        public global::Bond.ProtocolType ProtocolType
        {
            get { return global::Bond.ProtocolType.COMPACT_PROTOCOL; }
        }

        public bool IsMarshaled
        {
            get { return true; }
        }

        public void Serialize<T>(Output output, T obj)
        {
            global::Bond.Marshal.To(new global::Bond.Protocols.CompactBinaryWriter<Output>(output), obj);
        }

        public T Deserialize<T>(Input input)
        {
            return global::Bond.Unmarshal<T>.From(input);
        }
    }

    public struct FastBinaryMarshaler<Input, Output> : ISerializer<Input, Output>
        where Input : IInputStream, global::Bond.IO.ICloneable<Input>
        where Output : IOutputStream
    {
        public global::Bond.ProtocolType ProtocolType
        {
            get { return global::Bond.ProtocolType.FAST_PROTOCOL; }
        }

        public bool IsMarshaled
        {
            get { return true; }
        }

        public void Serialize<T>(Output output, T obj)
        {
            global::Bond.Marshal.To(new global::Bond.Protocols.FastBinaryWriter<Output>(output), obj);
        }

        public T Deserialize<T>(Input input)
        {
            return global::Bond.Unmarshal<T>.From(input);
        }
    }

    public struct SimpleBinaryMarshaler<Input, Output> : ISerializer<Input, Output>
        where Input : IInputStream, global::Bond.IO.ICloneable<Input>
        where Output : IOutputStream
    {
        public global::Bond.ProtocolType ProtocolType
        {
            get { return global::Bond.ProtocolType.SIMPLE_PROTOCOL; }
        }

        public bool IsMarshaled
        {
            get { return true; }
        }

        public void Serialize<T>(Output output, T obj)
        {
            global::Bond.Marshal.To(new global::Bond.Protocols.SimpleBinaryWriter<Output>(output), obj);
        }

        public T Deserialize<T>(Input input)
        {
            return global::Bond.Unmarshal<T>.From(input);
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
                case global::Bond.ProtocolType.COMPACT_PROTOCOL:
                    return marshal ? (ISerializer<Input, Output>)new CompactBinaryMarshaler<Input, Output>() : new CompactBinarySerializer<Input, Output>();
                case global::Bond.ProtocolType.FAST_PROTOCOL:
                    return marshal ? (ISerializer<Input, Output>)new FastBinaryMarshaler<Input, Output>() : new FastBinarySerializer<Input, Output>();
                case global::Bond.ProtocolType.SIMPLE_PROTOCOL:
                    return marshal ? (ISerializer<Input, Output>)new SimpleBinaryMarshaler<Input, Output>() : new SimpleBinarySerializer<Input, Output>();
            }

            throw new IPC.Managed.Exception("Unknown protocol.");
        }
    }
}
