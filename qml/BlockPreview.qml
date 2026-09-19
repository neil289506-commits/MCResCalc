import QtQuick
import QtQuick3D

// 用 6 個壓扁的內建 "#Cube" 疊成一個方塊的 6 個面，每個面各自貼不同材質
// （上/側/下），藉此在不用手刻自訂 mesh 的情況下做出「不同面不同貼圖」的效果。
// 已知小限制：6 片薄板在稜邊交界處會有極細微的 z-fighting（閃爍細線），
// 純粹是這個做法的取捨，不影響材質本身看起來對不對，先求穩再說。
Item {
    id: root
    property real dragLastX: 0
    property real dragLastY: 0

    View3D {
        id: view3d
        anchors.fill: parent

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Transparent
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 260)
            fieldOfView: 35
        }

        DirectionalLight {
            eulerRotation: Qt.vector3d(-30, -40, 0)
            brightness: 1.1
        }
        DirectionalLight {
            eulerRotation: Qt.vector3d(30, 140, 0)
            brightness: 0.35
        }

        Node {
            id: cubeRoot
            eulerRotation: Qt.vector3d(-20, 35, 0)

            // 上
            Model {
                source: "#Cube"
                position: Qt.vector3d(0, 49, 0)
                scale: Qt.vector3d(1, 0.02, 1)
                materials: PrincipledMaterial { baseColorMap: Texture { source: topTexUrl } }
            }
            // 下
            Model {
                source: "#Cube"
                position: Qt.vector3d(0, -49, 0)
                scale: Qt.vector3d(1, 0.02, 1)
                materials: PrincipledMaterial { baseColorMap: Texture { source: bottomTexUrl } }
            }
            // 前
            Model {
                source: "#Cube"
                position: Qt.vector3d(0, 0, 49)
                scale: Qt.vector3d(1, 1, 0.02)
                materials: PrincipledMaterial { baseColorMap: Texture { source: sideTexUrl } }
            }
            // 後
            Model {
                source: "#Cube"
                position: Qt.vector3d(0, 0, -49)
                scale: Qt.vector3d(1, 1, 0.02)
                materials: PrincipledMaterial { baseColorMap: Texture { source: sideTexUrl } }
            }
            // 右
            Model {
                source: "#Cube"
                position: Qt.vector3d(49, 0, 0)
                scale: Qt.vector3d(0.02, 1, 1)
                materials: PrincipledMaterial { baseColorMap: Texture { source: sideTexUrl } }
            }
            // 左
            Model {
                source: "#Cube"
                position: Qt.vector3d(-49, 0, 0)
                scale: Qt.vector3d(0.02, 1, 1)
                materials: PrincipledMaterial { baseColorMap: Texture { source: sideTexUrl } }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onPressed: (mouse) => { root.dragLastX = mouse.x; root.dragLastY = mouse.y }
        onPositionChanged: (mouse) => {
            if (!pressed) return
            const dx = mouse.x - root.dragLastX
            const dy = mouse.y - root.dragLastY
            cubeRoot.eulerRotation.y += dx * 0.6
            cubeRoot.eulerRotation.x = Math.max(-85, Math.min(85, cubeRoot.eulerRotation.x - dy * 0.6))
            root.dragLastX = mouse.x
            root.dragLastY = mouse.y
        }
    }
}
