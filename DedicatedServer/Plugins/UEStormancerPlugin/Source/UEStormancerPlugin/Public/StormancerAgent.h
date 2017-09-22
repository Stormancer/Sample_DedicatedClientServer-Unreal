// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "IServerDCS.h"
#include "IClientDCS.h"
#include "StormancerAgent.generated.h"
 
UENUM(BlueprintType)
enum class EStormancerConnectionStatus : uint8
{
	AUTHENTICATING 					UMETA(DisplayName = "Authenticating"),
	FINDING_LOCATOR 				UMETA(DisplayName = "Finding locator"),
	CONNECTING_LOCATOR				UMETA(DisplayName = "Connecting to locator"),
	FINDING_GAME_SESSION			UMETA(DisplayName = "Finding game session"),
	CONNECTING_TO_GAME_SESSION		UMETA(DisplayName = "Connecting to game session"),
	WAITING_SERVER					UMETA(DisplayName = "Waiting for server"),
	OPENING_CONNECTION				UMETA(DisplayName = "Opening connection"),
	CONNECTING						UMETA(DisplayName = "Connecting"),
	CONNECTED						UMETA(DisplayName = "Connected")
};

UENUM(BlueprintType)
enum class EStormancerShutdownMode : uint8
{
	NO_PLAYER_LEFT 					UMETA(DisplayName = "No player left"),
	SCENE_SHUTDOWN	 				UMETA(DisplayName = "Scene shutdown")
};

UCLASS(config = Game, defaultconfig)
class AStormancerAgent : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AStormancerAgent();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UEStormancerPlugin/Client")
	void OnClientConnectionFail(const FString& reason);
	void OnClientConnectionFail_Implementation(const FString& reason);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UEStormancerPlugin/Client")
	void OnClientConnectionSucces(const FString& host, int32 port);
	void OnClientConnectionSucces_Implementation(const FString& host, int32 port);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UEStormancerPlugin/Server")
	void OnServerConnectionFail(const FString& reason);
	void OnServerConnectionFail_Implementation(const FString& reason);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UEStormancerPlugin/Server")
	void OnServerConnectionSucces();
	void OnServerConnectionSucces_Implementation();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UEStormancerPlugin/Server")
	void OnServerShutdown(const FString& reason);
	void OnServerShutdown_Implementation(const FString& reason);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UEStormancerPlugin/Server")
	void OnConnectionStatusChange(EStormancerConnectionStatus connectionStatus);
	void OnConnectionStatusChange_Implementation(EStormancerConnectionStatus connectionStatus);

	UFUNCTION(BlueprintCallable, Category = "UEStormancerPlugin/Server")
	EStormancerConnectionStatus GetConnectionStatus();

	UFUNCTION(BlueprintCallable, Category = "UEStormancerPlugin/Server")
	void UpdateShutdownMode(EStormancerShutdownMode mode, uint8 keepServerAliveFor);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "TravelAgentToMap", Keywords = "UEStormancerPlugin"), Category = "UEStormancerPlugin")
	void TravelAgentToMap(FString mapId);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Init", Keywords = "UEStormancerPlugin"), Category = "UEStormancerPlugin")
	void Init();
	
	UPROPERTY(config, VisibleAnywhere, BlueprintReadOnly, Category = "UEStormancerPlugin/Server")
	FString _endPoint;

	UPROPERTY(config, VisibleAnywhere, BlueprintReadOnly, Category = "UEStormancerPlugin/Server")
	FString _defaultMap;

	UPROPERTY(config, VisibleAnywhere, BlueprintReadOnly, Category = "UEStormancerPlugin/Server")
	int32 _maxPeerByShard;

	UPROPERTY(config, VisibleAnywhere, BlueprintReadOnly, Category = "UEStormancerPlugin/Server")
	FString _accountID;

	UPROPERTY(config, VisibleAnywhere, BlueprintReadOnly, Category = "UEStormancerPlugin/Server")
	FString _applicationName;

	UFUNCTION( Category = "UEStormancerPlugin/Server")
	void ResetObject(AActor*  act);

	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	bool isRunning = false; 

	std::shared_ptr<SampleDCS::IClientDCS> _clientDCS;

	std::shared_ptr<SampleDCS::IServerDCS> _serverDCS;

	void _TravelAgentToMap();

	void SetStatusChangeFunction();

	std::shared_ptr<SampleDCS::IServerDCS> AStormancerAgent::CreateStormancerServer();

	void StartStormancerServer(char* buffer);

	std::shared_ptr<SampleDCS::IClientDCS> AStormancerAgent::CreateStormancerClient();

	void StartStormancerClient();
};
