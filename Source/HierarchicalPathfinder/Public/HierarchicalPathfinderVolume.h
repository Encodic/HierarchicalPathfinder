// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Volume.h"
#include "HierarchicalPathfinderVolume.generated.h"

UENUM()
enum EHierarchicalPF_State
{
	NotInitialised = 0,
	Initialised,
	GridGenerationDataReady,
	GridReady,
	ClustersReady,
	NodeDataReady,
	PathfinderVolumeReady
};

USTRUCT()
struct FNavCluster
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TMap<FName, FIntVector2> ClusterNeighbours;

	UPROPERTY()
	TArray<FIntVector2> ClusterNodeChildren;
	
};

USTRUCT()
struct FNavNode
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FIntVector2 NodeCoords;
	
	UPROPERTY()
	FIntVector2 ClusterID;

	UPROPERTY()
	int RegionID;

	UPROPERTY()
	TMap<FName, FIntVector2> NodeNeighbours;

	UPROPERTY()
	FVector2D NodeOffset;

	UPROPERTY()
	float NodeExtent;
};

UCLASS()
class HIERARCHICALPATHFINDER_API AHierarchicalPathfinderVolume : public AVolume
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AHierarchicalPathfinderVolume();

	UFUNCTION(CallInEditor, Category = "PathfinderSystemSetup")
	void Generate() { GenerateNavVolume(); };

	UFUNCTION(CallInEditor, Category = "PathfinderSystemSetup")
	void Clear() { ClearNavVolume(); };

	bool ClearNavVolume();
	bool GenerateNavVolume();
	bool GenerateGridData();
	bool PopulateNavGrid();
	bool GenerateNavClusters();
	bool GenerateNodeData();
	
	TMap<FName, FIntVector2> GetNodeNeighbours(const FIntVector2 Node) const;
	FVector2D GetNodeOffset(const FIntVector2 Node) const;
	FIntVector2 GetNodeClusterID(const FIntVector2 Node) const;
	

	FVector GetNodeWorldLocation(const FIntVector2 Node);
	FVector NodeOffsetToWorld(const FVector NodeOffset) const;
	static FVector RotateVectorAroundPivot(FVector InVector, FRotator Rotation, FVector Pivot);

	// Setup
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PathfinderSystemSetup")
	float NodeSize = 50.0f; // Determines size of a node.

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PathfinderSystemSetup")
	int ClusterSize = 8; // Determines size of a cluster. Example '8' will be a cluster of nodes in a 8x8 grid.

	// Debugging
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PathfinderSystemSetup|Debug")
	bool bDebug = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PathfinderSystemSetup|Debug|Nodes")
	bool bDrawNodes = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PathfinderSystemSetup|Debug|Nodes")
	float DrawNodeTime = 5.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PathfinderSystemSetup|Debug|Nodes")
	float DrawNodeLineThickness = 2.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PathfinderSystemSetup|Debug|Clusters")
	bool bDrawClusters = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PathfinderSystemSetup|Debug|Clusters")
	bool bDrawClusterNodes = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PathfinderSystemSetup|Debug|Clusters")
	float DrawClusterTime = 5.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PathfinderSystemSetup|Debug|Clusters")
	float DrawClusterLineThickness = 2.0f;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void SetHPVState(const EHierarchicalPF_State NewState) { HPVState = NewState; };
	EHierarchicalPF_State GetCurrentHPVState() const { return HPVState; };

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	
	EHierarchicalPF_State HPVState = NotInitialised;
	void UpdatePFState(const EHierarchicalPF_State DesiredState);

	// Used for calculating a Node's relative position
	FVector2D GridCornerOffset2D;
	
	FIntVector2 GridSize;
	
	FIntVector2 ClusterGridSize;

	TMap<FIntVector2, FNavNode> GridNodes;
	TMap<FIntVector2, FNavCluster> NavClusters;

	// Debugging
	float PFStartTime = 0.0f;
	float PreviousTime = 0.0f;
	static void GetStartTime(float& Time) { Time = FPlatformTime::Seconds(); };
	float DebugTime(float StartTime, FName ProcessName, const bool IncludeExecution) const;
	void DebugNumNodes() const;
	void DebugHPVolume();

	void DrawNode(const FIntVector2 Node);
	void DrawCluster(const FIntVector2 ClusterID);
	
};

