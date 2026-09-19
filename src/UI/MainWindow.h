#pragma once

#include <QMainWindow>
#include <QVector>
#include <QSet>
#include <QFutureWatcher>
#include <memory>

#include "../Services/MojangApiService.h"
#include "../Services/RecipeIndex.h"
#include "../Services/TagResolver.h"
#include "../Services/LangService.h"
#include "../Services/TextureService.h"
#include "../Services/ModelShapeResolver.h"
#include "../Services/ResourceCalculatorService.h"
#include "../Services/JarRecipeExtractor.h"
#include "../Models/VersionModels.h"
#include "../Models/MaterialNode.h"
#include "../Models/CraftingStep.h"

class QComboBox;
class QPushButton;
class QCheckBox;
class QLabel;
class QProgressBar;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QSpinBox;
class QTabWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QVBoxLayout;
class QStackedWidget;

namespace UI {
class Block3DPreview;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onReleaseVersionsReady(QVector<Models::VersionEntry> versions);
    void onReleaseVersionsError(QString message);
    void onVersionDetailReady(Models::VersionDetail detail);
    void onVersionDetailError(QString message);
    void onJarDownloadProgress(double fraction);
    void onJarReady(QString jarPath);
    void onJarError(QString message);
    void onExtractionFinished();

    void onLoadVersionsClicked();
    void onLoadRecipeDataClicked();
    void onSearchTextChanged(const QString &text);
    void onSuggestionSelectionChanged();
    void onAddToBatchClicked();
    void onCalculateClicked();
    void onEnglishToggled(bool checked);
    void onRawMaterialSelectionChanged();
    void onTreeSelectionChanged();

private:
    struct BatchItem
    {
        QString itemId;
        QString displayName;
        int quantity;
    };

    struct RawMaterialRow
    {
        QString itemId;
        QString displayName;
        int quantity;
    };

    void buildUi();
    void setStatus(const QString &text);
    void setBusy(bool busy);
    void refreshCalculateEnabled();

    void updateSuggestions();
    void rebuildBatchListWidget();
    void rebuildRawMaterialsWidget();
    void rebuildTreeWidget();
    void rebuildCraftingStepsWidget();
    void collectCraftingSteps(const std::shared_ptr<Models::MaterialNode> &node, QSet<QString> &seen);
    QWidget *buildCraftingStepCard(const Models::CraftingStep &step);
    QWidget *buildIconCell(const QPixmap &icon, const QString &tooltip, int size, const QString &label = QString());
    QTreeWidgetItem *buildTreeItem(const std::shared_ptr<Models::MaterialNode> &node);
    void applyIcons(const std::shared_ptr<Models::MaterialNode> &node);
    void setPreview(const QString &itemId);
    QString stacksText(int quantity) const;

    // ---- services ----
    Services::MojangApiService *m_api = nullptr;
    QFutureWatcher<Services::ExtractedGameData> *m_extractWatcher = nullptr;

    std::unique_ptr<Services::RecipeIndex> m_recipeIndex;
    std::unique_ptr<Services::TagResolver> m_tagResolver;
    std::unique_ptr<Services::LangService> m_lang;
    std::unique_ptr<Services::TextureService> m_textures;
    std::unique_ptr<Services::ModelShapeResolver> m_shapeResolver;
    std::unique_ptr<Services::ResourceCalculatorService> m_calculator;
    Services::CalculationOptions m_calcOptions;

    QVector<Models::VersionEntry> m_versions;
    QString m_selectedVersionId;
    Models::VersionDetail m_pendingDetail;
    bool m_dataLoaded = false;

    QVector<BatchItem> m_batchItems;
    QVector<RawMaterialRow> m_rawMaterialRows;
    QVector<Models::CraftingStep> m_craftingSteps;
    QVector<std::shared_ptr<Models::MaterialNode>> m_resultRoots;
    QVector<QString> m_suggestionItemIds;

    // ---- widgets ----
    QComboBox *m_versionCombo = nullptr;
    QPushButton *m_refreshVersionsBtn = nullptr;
    QPushButton *m_loadDataBtn = nullptr;
    QCheckBox *m_englishCheck = nullptr;
    QLabel *m_statusLabel = nullptr;
    QProgressBar *m_progressBar = nullptr;

    QLineEdit *m_searchEdit = nullptr;
    QListWidget *m_suggestionsList = nullptr;
    QLabel *m_selectedSuggestionLabel = nullptr;
    QSpinBox *m_quantityToAddSpin = nullptr;
    QPushButton *m_addToBatchBtn = nullptr;
    QListWidget *m_batchListWidget = nullptr;

    QLabel *m_preview2DIcon = nullptr;
    UI::Block3DPreview *m_preview3D = nullptr;
    QStackedWidget *m_previewStack = nullptr;
    QLabel *m_previewNameLabel = nullptr;
    QLabel *m_previewIdLabel = nullptr;
    QLabel *m_previewHintLabel = nullptr;

    QPushButton *m_calculateBtn = nullptr;
    QTabWidget *m_tabs = nullptr;
    QListWidget *m_rawMaterialsListWidget = nullptr;
    QTreeWidget *m_resultTreeWidget = nullptr;
    QWidget *m_craftingStepsContainer = nullptr;
    QVBoxLayout *m_craftingStepsLayout = nullptr;
    QListWidget *m_warningsListWidget = nullptr;
};
