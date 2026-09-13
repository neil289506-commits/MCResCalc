using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Media.Media3D;

namespace MinecraftResourceCalculator.Views;

/// <summary>
/// 用 WPF 內建的 Viewport3D（不需要任何第三方 3D 套件）畫一個貼了材質的立方體，
/// 用來預覽「立體方塊」外觀的物品。三個面各自可以是不同貼圖（上/側/下），
/// 對應遊戲裡草地、原木這類上下側不同貼圖的方塊。
///
/// 每個面的 Material 跟 BackMaterial 刻意設成同一份材質：三角形環繞方向在手刻網格時
/// 很容易搞錯方向，設成一樣可以確保無論從哪個環繞方向看，該面永遠看得到貼圖，
/// 不會因為背面剔除 (backface culling) 而變成看穿到方塊內部的透明面。
/// </summary>
public partial class Block3DPreview : UserControl
{
    public static readonly DependencyProperty TopTextureProperty =
        DependencyProperty.Register(nameof(TopTexture), typeof(BitmapImage), typeof(Block3DPreview),
            new PropertyMetadata(null, OnTextureChanged));

    public static readonly DependencyProperty SideTextureProperty =
        DependencyProperty.Register(nameof(SideTexture), typeof(BitmapImage), typeof(Block3DPreview),
            new PropertyMetadata(null, OnTextureChanged));

    public static readonly DependencyProperty BottomTextureProperty =
        DependencyProperty.Register(nameof(BottomTexture), typeof(BitmapImage), typeof(Block3DPreview),
            new PropertyMetadata(null, OnTextureChanged));

    public BitmapImage? TopTexture
    {
        get => (BitmapImage?)GetValue(TopTextureProperty);
        set => SetValue(TopTextureProperty, value);
    }

    public BitmapImage? SideTexture
    {
        get => (BitmapImage?)GetValue(SideTextureProperty);
        set => SetValue(SideTextureProperty, value);
    }

    public BitmapImage? BottomTexture
    {
        get => (BitmapImage?)GetValue(BottomTextureProperty);
        set => SetValue(BottomTextureProperty, value);
    }

    private Point _lastMousePos;
    private bool _dragging;

    public Block3DPreview()
    {
        InitializeComponent();
        MouseLeftButtonDown += (_, e) =>
        {
            _dragging = true;
            _lastMousePos = e.GetPosition(this);
            CaptureMouse();
        };
        MouseLeftButtonUp += (_, _) =>
        {
            _dragging = false;
            ReleaseMouseCapture();
        };
        MouseMove += OnMouseMove;
    }

    private void OnMouseMove(object sender, MouseEventArgs e)
    {
        if (!_dragging) return;

        var pos = e.GetPosition(this);
        var delta = pos - _lastMousePos;
        _lastMousePos = pos;

        RotationY.Angle += delta.X * 0.6;
        RotationX.Angle = Math.Clamp(RotationX.Angle - delta.Y * 0.6, -85, 85);
    }

    private static void OnTextureChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is Block3DPreview preview) preview.Rebuild();
    }

    private void Rebuild()
    {
        var side = SideTexture;
        if (side is null)
        {
            CubeVisual.Content = null;
            return;
        }

        var top = TopTexture ?? side;
        var bottom = BottomTexture ?? top;

        var group = new Model3DGroup();
        // 上
        group.Children.Add(BuildFace(
            new Point3D(-0.5, 0.5, 0.5), new Point3D(0.5, 0.5, 0.5), new Point3D(0.5, 0.5, -0.5), new Point3D(-0.5, 0.5, -0.5), top));
        // 下
        group.Children.Add(BuildFace(
            new Point3D(-0.5, -0.5, -0.5), new Point3D(0.5, -0.5, -0.5), new Point3D(0.5, -0.5, 0.5), new Point3D(-0.5, -0.5, 0.5), bottom));
        // 前
        group.Children.Add(BuildFace(
            new Point3D(-0.5, -0.5, 0.5), new Point3D(0.5, -0.5, 0.5), new Point3D(0.5, 0.5, 0.5), new Point3D(-0.5, 0.5, 0.5), side));
        // 後
        group.Children.Add(BuildFace(
            new Point3D(0.5, -0.5, -0.5), new Point3D(-0.5, -0.5, -0.5), new Point3D(-0.5, 0.5, -0.5), new Point3D(0.5, 0.5, -0.5), side));
        // 右
        group.Children.Add(BuildFace(
            new Point3D(0.5, -0.5, 0.5), new Point3D(0.5, -0.5, -0.5), new Point3D(0.5, 0.5, -0.5), new Point3D(0.5, 0.5, 0.5), side));
        // 左
        group.Children.Add(BuildFace(
            new Point3D(-0.5, -0.5, -0.5), new Point3D(-0.5, -0.5, 0.5), new Point3D(-0.5, 0.5, 0.5), new Point3D(-0.5, 0.5, -0.5), side));

        CubeVisual.Content = group;
    }

    private static GeometryModel3D BuildFace(Point3D p0, Point3D p1, Point3D p2, Point3D p3, BitmapImage texture)
    {
        var mesh = new MeshGeometry3D();
        mesh.Positions.Add(p0);
        mesh.Positions.Add(p1);
        mesh.Positions.Add(p2);
        mesh.Positions.Add(p3);

        mesh.TextureCoordinates.Add(new Point(0, 1));
        mesh.TextureCoordinates.Add(new Point(1, 1));
        mesh.TextureCoordinates.Add(new Point(1, 0));
        mesh.TextureCoordinates.Add(new Point(0, 0));

        mesh.TriangleIndices.Add(0);
        mesh.TriangleIndices.Add(1);
        mesh.TriangleIndices.Add(2);
        mesh.TriangleIndices.Add(0);
        mesh.TriangleIndices.Add(2);
        mesh.TriangleIndices.Add(3);

        var brush = new ImageBrush(texture);
        RenderOptions.SetBitmapScalingMode(brush, BitmapScalingMode.NearestNeighbor); // 保留 MC 貼圖的像素感，不要模糊

        var material = new DiffuseMaterial(brush);

        return new GeometryModel3D
        {
            Geometry = mesh,
            Material = material,
            BackMaterial = material
        };
    }
}
