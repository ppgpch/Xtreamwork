#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * XTREAMWORK CORE MODULE
 * =====================
 * Main plugin entry point
 */

class FXtreamworkCoreModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	// Get module instance
	static FXtreamworkCoreModule& Get();

	// Check if plugin is fully initialized
	bool IsInitialized() const { return bIsInitialized; }

private:
	bool bIsInitialized = false;

	// Initialize all layers
	void InitializeLayer1();
	void InitializeLayer2();
	void InitializeLayer3();
	void InitializeLayer4();
	void InitializeLayer5();
	void InitializeHeadroom();
};
