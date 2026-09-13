namespace MinecraftResourceCalculator.ViewModels;

public sealed class BatchItem : ObservableObject
{
    public string ItemId { get; }
    public string DisplayName { get; }

    private int _quantity;
    public int Quantity
    {
        get => _quantity;
        set => SetField(ref _quantity, Math.Max(1, value));
    }

    public BatchItem(string itemId, string displayName, int quantity)
    {
        ItemId = itemId;
        DisplayName = displayName;
        _quantity = Math.Max(1, quantity);
    }
}
