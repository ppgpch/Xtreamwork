#pragma once

#include "CoreMinimal.h"
#include "Containers/Map.h"
#include "Json.h"

/**
 * HEADROOM COMPRESSION LAYER (Integrated)
 * =======================================
 * Compresses all MCP request/response payloads by 85-95%
 * Strategy:
 * 1. CacheAligner: Stabilize KV-cache prefix (preserve provider cache hits)
 * 2. ContentRouter: Detect content type (JSON vs C++ vs text)
 * 3. SmartCrusher: Compress JSON outputs (remove padding, collapse nested)
 * 4. CodeCompressor: Compress C++ / Blueprint AST (structural redundancy)
 * 5. Kompress-base: Compress freeform text (trained on agent traces)
 * 6. CCR (reversible): Cache originals locally for retrieval
 */

struct FCompressionStats
{
	int32 OriginalBytes = 0;
	int32 CompressedBytes = 0;
	float CompressionRatio = 0.0f;      // 0.0-1.0, e.g., 0.08 = 92% reduction
	int32 TokensSaved = 0;
	int32 ExecutionTimeMs = 0;
	FString CompressorUsed;             // "SmartCrusher", "CodeCompressor", "Kompress-base"
};

/**
 * Cache Aligner
 * Stabilizes KV-cache prefix to avoid busting provider caches
 */
class FCacheAligner
{
public:
	static FCacheAligner& Get();

	// Detect volatile content that can bust cache
	bool HasVolatileContent(const FString& JsonPayload);

	// Warn about cache-busting patterns
	void AnalyzeForCacheIssues(const FString& JsonPayload, TArray<FString>& OutWarnings);

	// Never rewrite prompts - only emit warnings
	void LogCacheWarning(const FString& Warning);

private:
	FCacheAligner() = default;
	static FCacheAligner* Instance;
};

/**
 * Content Router
 * Detects content type and routes to appropriate compressor
 */
class FContentRouter
{
public:
	enum class EContentType
	{
		JSON,               // Tool outputs (SmartCrusher)
		CppCode,            // C++ / Blueprint AST (CodeCompressor)
		Text,               // Logs, descriptions (Kompress-base)
		Code,               // Any programming language
		Unknown
	};

	static FContentRouter& Get();

	// Detect content type
	EContentType DetectContentType(const FString& Content);

	// Get appropriate compressor for content
	class FCompressor* GetCompressor(EContentType Type);

private:
	FContentRouter() = default;
	static FContentRouter* Instance;
};

/**
 * Base Compressor Interface
 */
class FCompressor
{
public:
	virtual ~FCompressor() = default;

	virtual FString Compress(const FString& Input, FCompressionStats& OutStats) = 0;
	virtual FString Decompress(const FString& Compressed) = 0;
	virtual FString GetName() const = 0;
};

/**
 * SmartCrusher
 * Specialized for JSON compression (85-95% reduction)
 * - Removes whitespace/formatting
 * - Collapses redundant nested structures
 * - Abbreviates field names using context dictionary
 * - Removes null/default values
 */
class FSmartCrusher : public FCompressor
{
public:
	static FSmartCrusher& Get();

	virtual FString Compress(const FString& Input, FCompressionStats& OutStats) override;
	virtual FString Decompress(const FString& Compressed) override;
	virtual FString GetName() const override { return TEXT("SmartCrusher"); }

private:
	FSmartCrusher() = default;
	static FSmartCrusher* Instance;

	// Abbreviation dictionary for common JSON keys
	TMap<FString, FString> AbbreviationDict;

	// Remove null and default values
	FString RemoveDefaults(const FString& JsonString);

	// Collapse nested redundancy
	FString CollapseNested(const FString& JsonString);
};

/**
 * CodeCompressor
 * AST-aware compression for C++ / Blueprint code (60-80% reduction)
 * - Preserves syntax correctness
 * - Removes comments
 * - Collapses whitespace
 * - Abbreviates type names
 */
class FCodeCompressor : public FCompressor
{
public:
	static FCodeCompressor& Get();

	virtual FString Compress(const FString& Input, FCompressionStats& OutStats) override;
	virtual FString Decompress(const FString& Compressed) override;
	virtual FString GetName() const override { return TEXT("CodeCompressor"); }

private:
	FCodeCompressor() = default;
	static FCodeCompressor* Instance;

	// Remove comments
	FString RemoveComments(const FString& Code);

	// Collapse whitespace while preserving syntax
	FString MinifyCode(const FString& Code);
};

/**
 * Kompress-v2-base (ML-based)
 * Trained on agentic traces - handles freeform text
 * Requires HuggingFace model (external dependency)
 */
class FKompressV2Base : public FCompressor
{
public:
	static FKompressV2Base& Get();

	virtual FString Compress(const FString& Input, FCompressionStats& OutStats) override;
	virtual FString Decompress(const FString& Compressed) override;
	virtual FString GetName() const override { return TEXT("Kompress-v2-base"); }

	// Load model from HuggingFace
	bool LoadModel(const FString& ModelPath);

	bool IsModelLoaded() const { return bModelLoaded; }

private:
	FKompressV2Base() = default;
	static FKompressV2Base* Instance;

	bool bModelLoaded = false;
};

/**
 * CCR (Content-aware, Compressed, Reversible)
 * Stores originals locally, allows LLM to retrieve via tool
 */
class FCCR
{
public:
	static FCCR& Get();

	// Store original for later retrieval
	FString CacheOriginal(const FString& Original, const FString& CompressionId);

	// LLM can call this via tool to get original
	FString RetrieveOriginal(const FString& CompressionId);

	// Get all cached originals
	void GetCachedOriginals(TArray<FString>& OutIds);

	// Clear old cache entries (TTL: 1 hour)
	void CleanupExpiredCache();

	// Set TTL for cache entries
	void SetCacheTTL(int32 Seconds);

private:
	FCCR() = default;
	static FCCR* Instance;

	struct FCacheEntry
	{
		FString Content;
		FDateTime CreatedAt;
		int32 TTLSeconds = 3600;  // 1 hour default
	};

	TMap<FString, FCacheEntry> CacheStorage;
};

/**
 * Live-Zone Compression
 * Only compress new bytes (fresh tool output, latest turn)
 * Frozen prefix stays byte-identical so provider cache not busted
 */
class FLiveZoneCompressor
{
public:
	static FLiveZoneCompressor& Get();

	// Get previous message to identify frozen prefix
	FString GetFrozenPrefix() const { return FrozenPrefix; }

	// Compress only the new bytes
	FString CompressLiveZone(const FString& NewContent, FCompressionStats& OutStats);

	// Update frozen prefix for next turn
	void UpdateFrozenPrefix(const FString& LatestMessage);

private:
	FLiveZoneCompressor() = default;
	static FLiveZoneCompressor* Instance;

	FString FrozenPrefix;  // Bytes that must remain unchanged
};

/**
 * Unified Headroom Integration
 * Coordinates all compression strategies
 */
class FHeadroomIntegration : public FTickableGameObject
{
public:
	static FHeadroomIntegration& Get();

	void Initialize(const FString& CompressionLevel = TEXT("high"));
	void Shutdown();

	// Compress MCP request before sending to domain controller
	FString CompressRequest(const FString& JsonRequest, FCompressionStats& OutStats);

	// Decompress response before sending to LLM
	FString DecompressResponse(const FString& CompressedResponse);

	// Get last compression statistics
	const FCompressionStats& GetLastStats() const { return LastStats; }

	// Global compression statistics
	struct FGlobalStats
	{
		int64 TotalOriginalBytes = 0;
		int64 TotalCompressedBytes = 0;
		int32 TotalTokensSaved = 0;
		int32 RequestCount = 0;
	};

	const FGlobalStats& GetGlobalStats() const { return GlobalStats; }

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return true; }
	virtual TStatId GetStatId() const override;

private:
	FHeadroomIntegration() = default;
	static FHeadroomIntegration* Instance;

	FString CompressionLevel;  // "low", "medium", "high"
	FCompressionStats LastStats;
	FGlobalStats GlobalStats;
};
