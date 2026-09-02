#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EditorSubsystem.h"
#include "UI/XtreamworkPatternUI.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SButton.h"

/**
 * XTREAMWORK EDITOR PANEL
 * =======================
 * Main UI hub in Unreal Editor for pattern browsing, configuration, and AI instructions
 */

class SXtreamworkPanel : public SCompoundWidget
{
public:
	DECLARE_SLATE_ARGUMENTS_BEGIN(SXtreamworkPanel)
	END_SLATE_ARGUMENTS

	void Construct(const FArguments& InArgs);

	// Update panel when patterns change
	void RefreshPatterns();

private:
	// Left sidebar: pattern categories and search
	TSharedPtr<SVerticalBox> LeftSidebar;

	// Center: pattern grid cards
	TSharedPtr<SGridPanel> PatternGrid;

	// Right: target asset selection and parameters
	TSharedPtr<SVerticalBox> RightPanel;

	// Bottom: execution log and compression stats
	TSharedPtr<SMultiLineEditableTextBox> ExecutionLog;
	TSharedPtr<STextBlock> CompressionStatsDisplay;

	// Callbacks
	FReply OnPatternCardClicked(const FString& PatternId);
	FReply OnInstallButtonPressed();
	FReply OnPreviewButtonPressed();
	FReply OnUndoButtonPressed();

	void UpdateCompressionStats(const FCompressionStats& Stats);
	void LogExecutionMessage(const FString& Message, bool bIsError = false);

	// Current state
	FPatternExecutionUIState* CurrentPatternState = nullptr;
};

/**
 * Xtreamwork Editor Panel Tab
 */
class FXtreamworkEditorPanel : public FEditorSubsystem
{
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Create and show panel
	void ShowPanel();
	void HidePanel();

	// Check if panel is visible
	bool IsPanelVisible() const { return bIsPanelVisible; }

private:
	TSharedPtr<SXtreamworkPanel> PanelWidget;
	bool bIsPanelVisible = false;
};
