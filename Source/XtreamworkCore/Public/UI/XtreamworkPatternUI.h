#pragma once

#include "CoreMinimal.h"
#include "Layer3/XtreamworkLayer3.h"
#include "Containers/Map.h"

/**
 * PATTERN UI MANAGER
 * ==================
 * Provides UI panel for pattern browsing, configuration, and execution
 * Works in conjunction with AI (Pattern Matcher) and manual (Game Designer) workflows
 */

struct FPatternUICategory
{
	FString CategoryName;      // "Game Systems", "Core Gameplay", "Combat", etc.
	FLinearColor CategoryColor;  // For UI cards
	TArray<FGamePattern> Patterns;
};

/**
 * Pattern Card UI State
 */
struct FPatternCardState
{
	FString PatternId;
	FString DisplayName;
	FString Description;
	FString Category;
	FLinearColor CardColor;
	int32 EstimatedTimeMs = 0;
	int32 EstimatedTokens = 0;
	bool bIsHovered = false;
	bool bIsSelected = false;
	FString PrerequisiteStatus;  // "✓ Compatible" or "✗ Missing: UCharacterMovementComponent"
};

/**
 * Pattern Execution UI State
 */
struct FPatternExecutionUIState
{
	FString PatternId;
	FString TargetAsset;        // e.g., "BP_Hero"
	TMap<FString, FString> Parameters;  // User-configured values
	TArray<FString> PreviewChanges;     // What will be created/modified
	bool bPreviewMode = false;
	FCompressionStats CompressionStats;  // Headroom compression results
};

/**
 * Pattern UI Manager
 */
class FPatternUIManager : public FTickableGameObject
{
public:
	static FPatternUIManager& Get();

	void Initialize();
	void Shutdown();

	// Get categorized patterns for UI grid
	void GetCategorizedPatterns(TArray<FPatternUICategory>& OutCategories);

	// Get pattern card state
	FPatternCardState GetPatternCardState(const FString& PatternId) const;

	// Start pattern configuration workflow
	FPatternExecutionUIState* StartPatternConfiguration(const FString& PatternId);

	// Validate target asset compatibility
	void ValidateTargetAsset(const FString& PatternId, const FString& TargetAssetPath, FString& OutCompatibility);

	// Get parameter widget for UI
	struct FParameterWidget
	{
		FString ParameterName;
		FString ParameterType;  // "slider", "input", "dropdown", "checkbox"
		FString DefaultValue;
		TArray<FString> DropdownOptions;  // If type == dropdown
		float MinValue = 0.0f;
		float MaxValue = 1.0f;
	};

	void GetParameterWidgets(const FString& PatternId, TArray<FParameterWidget>& OutWidgets);

	// Preview pattern (dry run, show what will happen)
	void PreviewPattern(FPatternExecutionUIState& State, TArray<FString>& OutPreviewChanges, FCompressionStats& OutStats);

	// Execute pattern
	struct FExecutionResult
	{
		bool bSuccess = false;
		FString Message;
		int64 ExecutionTimeMs = 0;
		FCompressionStats CompressionStats;
	};

	FExecutionResult ExecutePattern(FPatternExecutionUIState& State);

	// Undo last pattern execution
	bool UndoLastPattern();

	// Get execution history
	struct FExecutionHistoryEntry
	{
		FString PatternId;
		FString TargetAsset;
		FDateTime ExecutedAt;
		bool bSuccess = false;
	};

	void GetExecutionHistory(TArray<FExecutionHistoryEntry>& OutHistory, int32 MaxEntries = 10);

	// Create custom pattern from current execution
	bool CreateCustomPattern(const FString& PatternName, const TArray<FDomainQuery>& Queries);

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return true; }
	virtual TStatId GetStatId() const override;

private:
	FPatternUIManager() = default;
	static FPatternUIManager* Instance;

	TArray<FPatternExecutionUIState> ConfigurationStack;
	TArray<FExecutionHistoryEntry> ExecutionHistory;
};

/**
 * Pattern Grid Widget (Visual Container)
 * Displayed as cards in the editor panel
 */
class FPatternGridWidget
{
public:
	struct FGridConfig
	{
		int32 CardsPerRow = 3;
		int32 CardWidth = 250;
		int32 CardHeight = 180;
	};

	// Render pattern cards to UI
	void RenderPatternGrid(const TArray<FPatternUICategory>& Categories, const FGridConfig& Config);

	// Handle user interactions
	void OnCardClicked(const FString& PatternId);
	void OnCardHovered(const FString& PatternId, bool bHovered);
	void OnInstallButtonPressed(const FString& PatternId);
};
