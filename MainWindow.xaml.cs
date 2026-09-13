using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Interop;
using MinecraftResourceCalculator.Models;
using MinecraftResourceCalculator.ViewModels;

namespace MinecraftResourceCalculator;

public partial class MainWindow : Window
{
    public MainWindow()
    {
        InitializeComponent();
        DataContext = new MainViewModel();
        SourceInitialized += (_, _) => TryEnableDarkTitleBar();
    }

    // TreeView.SelectedItem 是唯讀相依屬性，沒辦法直接用 {Binding} 綁定，
    // 只能透過事件轉交給 ViewModel，更新下方的物品預覽面板。
    private void ResultTreeView_SelectedItemChanged(object sender, RoutedPropertyChangedEventArgs<object> e)
    {
        if (DataContext is MainViewModel vm && e.NewValue is MaterialNode node)
            vm.SetPreview(node.ItemId);
    }

    // Windows 10 1809+ / 11：讓原生標題列也跟著走深色模式，
    // 不然標題列會是系統預設的亮色，跟視窗內容的深色主題很不搭。
    // 純裝飾用途，抓不到就安靜放棄，不影響程式其他功能。
    [DllImport("dwmapi.dll", PreserveSig = true)]
    private static extern int DwmSetWindowAttribute(IntPtr hwnd, int attribute, ref int pvAttribute, int cbAttribute);

    private const int DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
    private const int DWMWA_USE_IMMERSIVE_DARK_MODE_OLD = 19;

    private void TryEnableDarkTitleBar()
    {
        try
        {
            var hwnd = new WindowInteropHelper(this).Handle;
            if (hwnd == IntPtr.Zero) return;

            int useDark = 1;
            if (DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, ref useDark, sizeof(int)) != 0)
                DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE_OLD, ref useDark, sizeof(int));
        }
        catch
        {
            // 純美觀加分項，失敗就算了，不能讓它擋到程式啟動。
        }
    }
}
