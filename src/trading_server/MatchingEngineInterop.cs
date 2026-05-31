using System.Runtime.InteropServices;
using Microsoft.Extensions.Logging;

namespace TradingServer;

public class MatchingEngineInterop : IDisposable
{
    private const string LibPath = "../cpp_engine/libmatchingengine.dylib";

    [DllImport(LibPath, EntryPoint = "create_engine")]
    private static extern IntPtr CreateEngine();

    [DllImport(LibPath, EntryPoint = "destroy_engine")]
    private static extern void DestroyEngine(IntPtr engine);

    [DllImport(LibPath, EntryPoint = "engine_add_order")]
    private static extern void EngineAddOrder(IntPtr engine, ulong id, ulong price, uint quantity, byte side);

    [DllImport(LibPath, EntryPoint = "engine_cancel_order")]
    private static extern void EngineCancelOrder(IntPtr engine, ulong id);

    [DllImport(LibPath, EntryPoint = "engine_get_best_bid")]
    private static extern ulong EngineGetBestBid(IntPtr engine);

    [DllImport(LibPath, EntryPoint = "engine_get_best_ask")]
    private static extern ulong EngineGetBestAsk(IntPtr engine);

    [DllImport(LibPath, EntryPoint = "engine_get_volume_at_price")]
    private static extern ulong EngineGetVolumeAtPrice(IntPtr engine, ulong price, byte side);

    private readonly IntPtr _enginePtr;
    private readonly ILogger<MatchingEngineInterop> _logger;

    public MatchingEngineInterop(ILogger<MatchingEngineInterop> logger)
    {
        _logger = logger;
        _enginePtr = CreateEngine();
        if (_enginePtr == IntPtr.Zero)
        {
            throw new InvalidOperationException("Failed to instantiate C++ matching engine.");
        }
        _logger.LogInformation("C++ Level 3 Matching Engine initialized natively.");
    }

    public void AddOrder(ulong id, ulong price, uint quantity, byte side)
    {
        EngineAddOrder(_enginePtr, id, price, quantity, side);
    }

    public void CancelOrder(ulong id)
    {
        EngineCancelOrder(_enginePtr, id);
    }

    public ulong GetBestBid() => EngineGetBestBid(_enginePtr);
    public ulong GetBestAsk() => EngineGetBestAsk(_enginePtr);
    public ulong GetVolumeAtPrice(ulong price, byte side) => EngineGetVolumeAtPrice(_enginePtr, price, side);

    public void Dispose()
    {
        if (_enginePtr != IntPtr.Zero)
        {
            DestroyEngine(_enginePtr);
            _logger.LogInformation("C++ Level 3 Matching Engine destroyed.");
        }
        GC.SuppressFinalize(this);
    }

    ~MatchingEngineInterop()
    {
        Dispose();
    }
}
