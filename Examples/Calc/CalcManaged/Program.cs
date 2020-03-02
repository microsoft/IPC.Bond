using System;

namespace Calc.Managed
{
    static class Program
    {
        static int Main(string[] args)
        {
            var address = "ipc://calc";

            switch (args.Length)
            {
                case 1:
                    if (args[0] == "--server")
                    {
                        Server.Run(address);
                        break;
                    }
                    else if (args[0] == "--client")
                    {
                        Client.Run(address);
                        break;
                    }
                    goto default;

                default:
                    Console.WriteLine("Pass --server or --client option.");
                    return 1;
            }

            return 0;
        }
    }
}
