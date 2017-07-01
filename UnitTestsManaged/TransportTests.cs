using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using System.Linq;
using NUnit.Framework;

namespace IPC.Bond.Managed.UnitTests
{
    [TestFixture(Category = "ManagedTests")]
    public class TransportTests
    {
        [Test]
        public async Task AcceptorConnectorTest()
        {
            var address = Guid.NewGuid().ToString();
            bool isClientClosed = false;
            bool isServerClosed = false;

            var config = new Config { DefaultRequestTimeout = System.Threading.Timeout.InfiniteTimeSpan };

            using (var transport = new Transport<global::Bond.Box<int>, global::Bond.Box<int>>(config))
            using (var acceptor = transport.MakeServerAcceptor(address, (inMemory, outMemory) => x => Task.FromResult(x)))
            using (var connector = transport.MakeClientConnector())
            {
                var servers = new List<Transport<global::Bond.Box<int>, global::Bond.Box<int>>.Server>();
                var newServerEvent = new System.Threading.ManualResetEventSlim(false);

                acceptor.Accepted += (sender, args) =>
                {
                    lock (servers)
                    {
                        servers.Add(args.Component);
                    }

                    newServerEvent.Set();
                };

                Transport<global::Bond.Box<int>, global::Bond.Box<int>>.Server server;

                using (var client = await connector.ConnectAsync(address))
                {
                    newServerEvent.Wait();

                    lock (servers)
                    {
                        Assert.AreEqual(1, servers.Count);
                        server = servers.First();
                    }

                    Assert.IsFalse(server.IsClosed);
                    server.Closed += (sender, args) => { isServerClosed = true; };

                    Assert.IsFalse(client.IsClosed);
                    client.Closed += (sender, args) => { isClientClosed = true; };

                    var request = new global::Bond.Box<int> { value = 100 };
                    var response = await client.InvokeAsync(request);

                    Assert.IsTrue(global::Bond.Comparer.Equal(request, response));
                }

                server.Dispose();
            }

            Assert.IsTrue(isClientClosed);
            Assert.IsTrue(isServerClosed);
        }

        [Test]
        public async Task AcceptConnectTest()
        {
            var address = Guid.NewGuid().ToString();

            var config = new Config { DefaultRequestTimeout = System.Threading.Timeout.InfiniteTimeSpan };

            using (var transport = new Transport<global::Bond.Box<int>, global::Bond.Box<int>>(config))
            using (var serversAccessor = transport.AcceptServers(address, (inMemory, outMemory) => x => Task.FromResult(x)))
            using (var clientAccessor = transport.ConnectClient(address, false))
            {
                var request = new global::Bond.Box<int> { value = 200 };
                var response = await clientAccessor.Client.InvokeAsync(request);

                Assert.IsTrue(global::Bond.Comparer.Equal(request, response));
            }
        }
    }
}
