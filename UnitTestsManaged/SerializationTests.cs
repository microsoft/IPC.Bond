using System;
using System.Collections.Generic;
using System.Linq;
using NUnit.Framework;
using NUnit.Framework.Internal;

namespace IPC.Bond.Managed.UnitTests
{
    using SharedMemory = IPC.Managed.SharedMemory;

    [TestFixture(Category = "ManagedTests")]
    public class SerializationTests
    {
        private readonly Randomizer _rand = new Randomizer();

        private byte[] MakeRandomBytes()
        {
            var buffer = new byte[_rand.Next(1, 1024)];
            _rand.NextBytes(buffer);
            return buffer;
        }

        private ValueStruct MakeRandomValueStruct()
        {
            return new ValueStruct
            {
                FieldBool = _rand.Next() % 2 == 0,
                FieldInt8 = (sbyte)_rand.Next(sbyte.MinValue, sbyte.MaxValue),
                FieldUInt8 = (byte)_rand.Next(byte.MinValue, byte.MaxValue),
                FieldInt16 = (short)_rand.Next(short.MinValue, short.MaxValue),
                FieldUInt16 = (ushort)_rand.Next(ushort.MinValue, ushort.MaxValue),
                FieldInt32 = _rand.Next(),
                FieldUInt32 = (uint)_rand.NextDouble(),
                FieldInt64 = (long)_rand.NextDouble(),
                FieldUInt64 = (ulong)_rand.NextDouble(),
                FieldFloat = (float)_rand.NextDouble(),
                FieldDouble = _rand.NextDouble(),
                FieldEnum = (TestEnum)Enum.GetValues(typeof(TestEnum)).GetValue(_rand.Next() % Enum.GetValues(typeof(TestEnum)).Length),
                FieldString = System.Text.Encoding.ASCII.GetString(MakeRandomBytes()),
                FieldWString = System.Text.Encoding.Unicode.GetString(MakeRandomBytes()),
                FieldBlob = new ArraySegment<byte>(MakeRandomBytes())
            };
        }

        private IEnumerable<ValueStruct> MakeRandomValueStructs()
        {
            var count = _rand.Next(0, _rand.Next(0, 100));
            for (int i = 0; i < count; ++i)
            {
                yield return MakeRandomValueStruct();
            }
        }

        private Dictionary<string, ValueStruct> MakeRandomValueStructMap()
        {
            var map = new Dictionary<string, ValueStruct>();
            var count = _rand.Next(0, _rand.Next(0, 100));
            for (int i = 0; i < count; ++i)
            {
                map[Guid.NewGuid().ToString()] = MakeRandomValueStruct();
            }
            return map;
        }

        private TestStruct MakeRandomStruct()
        {
            return new TestStruct
            {
                FieldList = new LinkedList<ValueStruct>(MakeRandomValueStructs()),
                FieldVector = new List<ValueStruct>(MakeRandomValueStructs()),
                FieldSet = new HashSet<int>(Enumerable.Repeat(_rand.Next(), 100)),
                FieldMap = new Dictionary<string, ValueStruct>(MakeRandomValueStructMap()),
                FieldNullable = _rand.Next() % 2 == 0 ? MakeRandomValueStruct() : null,
                FieldBonded = new global::Bond.Bonded<ValueStruct>(MakeRandomValueStruct())
            };
        }

        [Test]
        public void SerializerTest()
        {
            using (var memory = SharedMemory.Create(Guid.NewGuid().ToString(), 4 * 1024 * 1024))
            using (var pool = new BufferPool(memory))
            {
                foreach (var protocol in new global::Bond.ProtocolType[] { global::Bond.ProtocolType.COMPACT_PROTOCOL, global::Bond.ProtocolType.FAST_PROTOCOL, global::Bond.ProtocolType.SIMPLE_PROTOCOL })
                {
                    foreach (var marshal in new bool[] { true, false })
                    {
                        var serializer = new Serializer(protocol, marshal, pool, memory);

                        Assert.AreEqual(protocol, serializer.ProtocolType);
                        Assert.AreEqual(marshal, serializer.IsMarshaled);

                        var s1 = MakeRandomStruct();
                        TestStruct s2;

                        using (var buffer = serializer.Serialize(s1))
                        {
                            s2 = serializer.Deserialize<TestStruct>(buffer);
                        }

                        // Bond.Comparer.Equal does not properly work with Bonded<T> fields.
                        Assert.IsTrue(global::Bond.Comparer.Equal(s1.FieldBonded.Deserialize(), s2.FieldBonded.Deserialize()));
                        s1.FieldBonded = s2.FieldBonded = global::Bond.Bonded<ValueStruct>.Empty;

                        Assert.IsTrue(global::Bond.Comparer.Equal(s1, s2));
                    }
                }
            }
        }
    }
}
