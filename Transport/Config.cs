namespace IPC.Bond.Managed
{
    public class Config : IPC.Managed.Config
    {
        public global::Bond.ProtocolType ProtocolType { get; set; } = global::Bond.ProtocolType.COMPACT_PROTOCOL;

        public bool Marshal { get; set; } = true;

        public uint MinBlobSize { get; set; } = 0;
    }
}
