using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;

namespace TradingServer;

class Program
{
    static async Task Main(string[] args)
    {
        var host = Host.CreateDefaultBuilder(args)
            .ConfigureServices((hostContext, services) =>
            {
                services.Configure<GatewayOptions>(opts => 
                {
                    opts.MarketDataAddress = "tcp://*:5555";
                    opts.OrderEntryAddress = "tcp://*:5556";
                });
                
                services.AddSingleton<MatchingEngineInterop>();
                services.AddHostedService<ZeroMQGateway>();
            })
            .Build();

        await host.RunAsync();
    }
}
