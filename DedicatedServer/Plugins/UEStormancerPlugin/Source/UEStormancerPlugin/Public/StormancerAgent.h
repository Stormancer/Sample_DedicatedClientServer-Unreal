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
	KEEP_ALIVE	 					UMETA(DisplayName = "Keep alive")
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

	/** 
		This event will be fired when the client fails it's connection. 
		@param reason the reason of the fail
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UEStormancerPlugin/Client")
	void OnClientConnectionFail(const FString& reason);
	void OnClientConnectionFail_Implementation(const FString& reason);

	/**
		This event will be fired when the client succeed it's connection.
		@param host the host you are connected to
		@param port the port on which you are connected to the host
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UEStormancerPlugin/Client")
	void OnClientConnectionSucces(const FString& host, int32 port);
	void OnClientConnectionSucces_Implementation(const FString& host, int32 port);

	/**
		This event will be fired when the server fails it's connection.
		@param reason the reason of the fail
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UEStormancerPlugin|Server")
	void OnServerConnectionFail(const FString& reason);
	void OnServerConnectionFail_Implementation(const FString& reason);

	/**
		This event will be fired when the server succeed it's connection.
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UEStormancerPlugin|Server")
	void OnServerConnectionSucces();
	void OnServerConnectionSucces_Implementation();
	
	/**
		This event will be fired when the server shutdown.
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UEStormancerPlugin|Server")
	void OnServerShutdown(const FString& reason);
	void OnServerShutdown_Implementation(const FString& reason);

	/**
		This event will be fired when the connection status change.
		@param connectionStatus The current connection status
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UEStormancerPlugin|Server")
	void OnConnectionStatusChange(EStormancerConnectionStatus connectionStatus);
	void OnConnectionStatusChange_Implementation(EStormancerConnectionStatus connectionStatus);

	/**
		Get the current connection status.
		@return The current connection status
	*/
	UFUNCTION(BlueprintCallable, Category = "UEStormancerPlugin|Server")
	EStormancerConnectionStatus GetConnectionStatus();

	/**
		Update the shutdown mode of the server. If mode is NoPlayerLeft, this parameter is not used. 
		This parameter cannot be greater than 1 hour (3600 seconds). 
		We recommend to set keepServerAliveFor to 3-5 minutes, to avoid to keep the server alive too long if there is a problem on the server.
		@param mode You can choose the shutdown mode between NoPlayerLeft (default behavior), and KeepAlive
		@param keepServerAliveFor The duration (in seconds) that the server will keep alive if the mode is set to KeepAlive. 
	*/
	UFUNCTION(BlueprintCallable, Category = "UEStormancerPlugin|Server")
	void UpdateShutdownMode(EStormancerShutdownMode mode, int32 keepServerAliveFor);

	/**
		Travel the player to a specific map. If there is no dedicated server on this map, the server application will start a new one.
		@param mapId The map name, by default unreal will search maps into Game/Maps. If the map is not in the Game/Maps folder, you have to put the full path of the map (Game/MapFolder/MyMap)
	*/
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "TravelAgentToMap", Keywords = "UEStormancerPlugin"), Category = "UEStormancerPlugin")
	void TravelAgentToMap(FString mapId);

	/**
		This will init the stormancer agent and create a client if run on the game build, or a server if run on a dedicated server
	*/
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Init", Keywords = "UEStormancerPlugin"), Category = "UEStormancerPlugin")
	void Init();
	
	UPROPERTY(config, VisibleAnywhere, BlueprintReadOnly, Category = "UEStormancerPlugin|Server")
	FString _endPoint;

	UPROPERTY(config, VisibleAnywhere, BlueprintReadOnly, Category = "UEStormancerPlugin|Server")
	FString _defaultMap;

	UPROPERTY(config, VisibleAnywhere, BlueprintReadOnly, Category = "UEStormancerPlugin|Server")
	int32 _maxPeerByShard;

	UPROPERTY(config, VisibleAnywhere, BlueprintReadOnly, Category = "UEStormancerPlugin|Server")
	FString _accountID;

	UPROPERTY(config, VisibleAnywhere, BlueprintReadOnly, Category = "UEStormancerPlugin|Server")
	FString _applicationName;

	UFUNCTION(Category = "UEStormancerPlugin|Server")
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
