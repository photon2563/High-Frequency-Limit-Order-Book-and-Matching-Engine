using System.Text.Json;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;
using Microsoft.Extensions.Options;
using NetMQ;
using NetMQ.Sockets;

namespace TradingServer;

public class GatewayOptions
{
    public string MarketDataAddress { get; set; } = "tcp://*:5555";
    public string OrderEntryAddress { get; set; } = "tcp://*:5556";
}

public class MarketUpdate
{
    public ulong BestBid { get; set; }
    public ulong BestAsk { get; set; }
}

public class OrderMessage
{
    public string Action { get; set; } = ""; // "ADD" or "CANCEL"
    public ulong Id { get; set; }
    public ulong Price { get; set; }
    public uint Quantity { get; set; }
    public byte Side { get; set; }
}

public class ZeroMQGateway : BackgroundService
{
    private readonly ILogger<ZeroMQGateway> _logger;
    private readonly MatchingEngineInterop _engine;
    private readonly GatewayOptions _options;

    public ZeroMQGateway(ILogger<ZeroMQGateway> logger, MatchingEngineInterop engine, IOptions<GatewayOptions> options)
    {
        _logger = logger;
        _engine = engine;
        _options = options.Value;
    }

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        _logger.LogInformation("Starting ZeroMQ Gateway...");
        
        using var pubSocket = new PublisherSocket();
        pubSocket.Bind(_options.MarketDataAddress);

        using var pullSocket = new PullSocket();
        pullSocket.Bind(_options.OrderEntryAddress);

        using var poller = new NetMQPoller { pullSocket };

        pullSocket.ReceiveReady += (s, e) =>
        {
            var msg = e.Socket.ReceiveFrameString();
            try
            {
                var orderMsg = JsonSerializer.Deserialize<OrderMessage>(msg);
                if (orderMsg != null)
                {
                    if (orderMsg.Action == "ADD")
                    {
                        _engine.AddOrder(orderMsg.Id, orderMsg.Price, orderMsg.Quantity, orderMsg.Side);
                    }
                    else if (orderMsg.Action == "CANCEL")
                    {
                        _engine.CancelOrder(orderMsg.Id);
                    }

                    // Publish L3 update or simply BBO update
                    var update = new MarketUpdate
                    {
                        BestBid = _engine.GetBestBid(),
                        BestAsk = _engine.GetBestAsk()
                    };
                    pubSocket.SendMoreFrame("MARKET_DATA").SendFrame(JsonSerializer.Serialize(update));
                }
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error processing order message");
            }
        };

        poller.RunAsync();

        while (!stoppingToken.IsCancellationRequested)
        {
            await Task.Delay(1000, stoppingToken);
        }

        poller.Stop();
        _logger.LogInformation("ZeroMQ Gateway stopped.");
    }
}
