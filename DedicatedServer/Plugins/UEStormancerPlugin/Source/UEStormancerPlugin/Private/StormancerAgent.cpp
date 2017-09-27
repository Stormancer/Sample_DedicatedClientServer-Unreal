// Fill out your copyright notice in the Description page of Project Settings.

#include "UEStormancerPlugin/Public/StormancerAgent.h"
#include "Kismet/GameplayStatics.h"
#include "UEStormancerPluginBPLibrary.h"

#include "IServerDCS.h"
#include "Base64.h"
#include <thread>
 
// Sets default values
AStormancerAgent::AStormancerAgent()
{
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AStormancerAgent::BeginPlay() 
{
	Super::BeginPlay();
	this->OnDestroyed.AddDynamic(this, &AStormancerAgent::ResetObject);

}

// Called every frame
void AStormancerAgent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (isRunning)
	{
		if (_clientDCS != nullptr)
			_clientDCS->Tick();
		else
			_serverDCS->Tick();
	}
}

void AStormancerAgent::Init()
{
	size_t len = 256;
	char *buffer = new char[256];
	//Get connection token passed as environment variable when the Stormancer app starts the server
	auto err_no = _dupenv_s(&buffer, &len, "connectionToken");
	

	if (err_no || !len)
	{
		UE_LOG(LogTemp, Warning, TEXT("Initialize Client"));
		_clientDCS = UUEStormancerPluginBPLibrary::GetDCSClient(GetGameInstance());
		if (_clientDCS == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("Create new client"));
			_clientDCS = CreateStormancerClient();
			SetStatusChangeFunction();
			StartStormancerClient();
			UUEStormancerPluginBPLibrary::AddDCSClient(GetGameInstance(), _clientDCS);
		}
		else
		{
			SetStatusChangeFunction(); 
			if (_clientDCS->GetNextMap().length() > 0)
			{
				_TravelAgentToMap();
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("Client initalized"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Initialize Server"));
		_serverDCS = UUEStormancerPluginBPLibrary::GetDCSServer(GetGameInstance());
		if (_serverDCS == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("Create new Server"));
			_serverDCS = CreateStormancerServer();
			StartStormancerServer(buffer);
			UUEStormancerPluginBPLibrary::AddDCSServer(GetGameInstance(), _serverDCS);
		}
		UE_LOG(LogTemp, Warning, TEXT("Server initalized"));
	}
	isRunning = true;
}

void AStormancerAgent::OnClientConnectionFail_Implementation(const FString& reason)
{
	UE_LOG(LogTemp, Error, TEXT("Client Connection Fail : %s"), *reason); 
}

void AStormancerAgent::OnClientConnectionSucces_Implementation(const FString& host, int32 port)
{
	APlayerController* playerController = UGameplayStatics::GetPlayerController(this, 0);
	
	if (playerController == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerController not found"));
		return;
	}

	FString url = host + ":" + FString::FromInt(port);	
	UE_LOG(LogTemp, Log, TEXT("Client Connection Succes starting travel"));
	playerController->ClientTravel(url, ETravelType::TRAVEL_Relative);

}

void AStormancerAgent::OnServerConnectionFail_Implementation(const FString& reason)
{
	UE_LOG(LogTemp, Error, TEXT("Server connection fail : %s"), *reason);
}

void AStormancerAgent::OnServerConnectionSucces_Implementation()
{
	size_t len = 256;
	char *buffer = new char[256];
	//Get connection token passed as environment variable when the Stormancer app starts the server
	auto err_no = _dupenv_s(&buffer, &len, "userData");
	FString bufferString = FString(buffer);
	FString mapId;
	FBase64::Decode(bufferString, mapId);

	// put server in listen mode
	auto world = GetWorld();

	if (!world)
	{
		FString reason = "Game world not found";
		OnServerConnectionFail(reason);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Launch server game map"));
	
	//OnClientConnectionSucces should be called only when the server have traveled
	world->ServerTravel(mapId, false);
}

void AStormancerAgent::OnServerShutdown_Implementation(const FString& reason)
{
	FWindowsPlatformMisc::RequestExit(false);
	UE_LOG(LogTemp, Error, TEXT("Shutdown reason : %s"), *reason);
}

void AStormancerAgent::OnConnectionStatusChange_Implementation(EStormancerConnectionStatus connectionStatus)
{
	const UEnum* statusEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EStormancerConnectionStatus"));
	UE_LOG(LogTemp, Warning, TEXT("OnConnectionStatus change : %s"), *(statusEnum ? statusEnum->GetNameStringByIndex((int32)connectionStatus) : TEXT("<Invalid Enum>")));
}

EStormancerConnectionStatus AStormancerAgent::GetConnectionStatus()
{
	return _clientDCS ? (EStormancerConnectionStatus)_clientDCS->GetConnectionStatus() : (EStormancerConnectionStatus)0;
}

void AStormancerAgent::UpdateShutdownMode(EStormancerShutdownMode shutdownMode, int32 keepServerAliveFor)
{
	if (_serverDCS != nullptr)
	{
		SampleDCS::UpdateShutdownModeParameter param = SampleDCS::UpdateShutdownModeParameter();
		param.mode = (SampleDCS::ShutdownMode)shutdownMode;
		param.keepServerAliveFor = keepServerAliveFor;
		_serverDCS->UpdateShutdownMode(param);
	}
}

void AStormancerAgent::TravelAgentToMap(FString mapId)
{
	UE_LOG(LogTemp, Warning, TEXT("StartTravelAgentToMap"));
	if (_clientDCS == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("client dcs is null"));
		return;
	}

	_clientDCS->SetNextMap(std::string(TCHAR_TO_UTF8(*mapId)));

	UGameplayStatics::OpenLevel((UObject*)GetGameInstance(), FName(TEXT("EntryMap")));

}

std::shared_ptr<SampleDCS::IServerDCS> AStormancerAgent::CreateStormancerServer()
{
	size_t len = 256;
	char *mapIdBuffer = new char[256];
	//Get connection token passed as environment variable when the Stormancer app starts the server
	auto err_no = _dupenv_s(&mapIdBuffer, &len, "userData");
	FString bufferString = FString(mapIdBuffer);
	FString mapId;
	FBase64::Decode(bufferString, mapId);

	FString reason = "";

	size_t world = (size_t)GetWorld();

	std::string endPoint = TCHAR_TO_UTF8(*(_endPoint));
	// Check end point
	if (endPoint.length() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Endpoint property not set"));
		return nullptr;
	}

	std::string accountID = TCHAR_TO_UTF8(*(_accountID));
	std::string appName = TCHAR_TO_UTF8(*(_applicationName));
	std::shared_ptr<SampleDCS::IServerDCS> serverDCS = SampleDCS::IServerDCS::MakeServerDCS(180, endPoint, accountID, appName, _maxPeerByShard);
	return serverDCS;
}


void AStormancerAgent::StartStormancerServer(char* buffer)
{
	if (_serverDCS == nullptr)
	{
		OnServerConnectionFail("server not set");
		return;
	}
	
	//Start server callback
	std::function<void(SampleDCS::Endpoint)> startServer = [this](SampleDCS::Endpoint e)
	{
		UE_LOG(LogTemp, Warning, TEXT("call back start server"));
		OnServerConnectionSucces();
	};

	std::function<void()> shutdownStormancer = [this]()
	{
		FString reason = "Stormancer server request shutdown";
		UE_LOG(LogTemp, Warning, TEXT("Stormancer server request shutdown"));
		OnServerShutdown(reason);
	};
	
	UE_LOG(LogTemp, Warning, TEXT("Server init"));

	_serverDCS->RunServer(std::string(buffer), startServer, shutdownStormancer)->Then([](StormancerResult<void> r)
	{
		if (r.Success())
		{
			UE_LOG(LogTemp, Warning, TEXT("Server init"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Server start failed"));
		}
	});
}

std::shared_ptr<SampleDCS::IClientDCS> AStormancerAgent::CreateStormancerClient()
{
	FString reason = "";
	FString endpoint;
	if (FParse::Value(FCommandLine::Get(), TEXT("Endpoint"), endpoint)) {
		endpoint = endpoint.Replace(TEXT("="), TEXT("")).Replace(TEXT("\""), TEXT("")); // replace quotes
		_endPoint = endpoint;
		UE_LOG(LogTemp, Warning, TEXT("Endpoint : %s"), *_endPoint);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No Endpoint"));
	}
	FString accountName;
	if (FParse::Value(FCommandLine::Get(), TEXT("AccountName"), accountName)) {
		accountName = accountName.Replace(TEXT("="), TEXT("")).Replace(TEXT("\""), TEXT("")); // replace quotes
		_accountID = accountName;
		UE_LOG(LogTemp, Warning, TEXT("AccountId : %s"), *_accountID);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No AccountId"));
	}
	FString applicationName;
	if (FParse::Value(FCommandLine::Get(), TEXT("ApplicationName"), applicationName)) {
		applicationName = applicationName.Replace(TEXT("="), TEXT("")).Replace(TEXT("\""), TEXT("")); // replace quotes
		_applicationName = applicationName;
		UE_LOG(LogTemp, Warning, TEXT("ApplicationName : %s"), *_applicationName);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No ApplicationName"));
	}
	FString _connectionToken;
	if (FParse::Value(FCommandLine::Get(), TEXT("ConnectionToken"), _connectionToken)) {
		_connectionToken = _connectionToken.Replace(TEXT("="), TEXT("")).Replace(TEXT("\""), TEXT("")); // replace quotes
		UE_LOG(LogTemp, Warning, TEXT("ConnectionToken : %s"), *_connectionToken);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No ConnectionToken"));
	}

	std::string endPoint = TCHAR_TO_UTF8(*(_endPoint));
	// Check end point
	if (endPoint.length() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Endpoint property not set"));
		return nullptr;
	}

	std::string accountID = TCHAR_TO_UTF8(*(_accountID));
	std::string appName = TCHAR_TO_UTF8(*(_applicationName));
	std::string connectionToken = TCHAR_TO_UTF8(*(_connectionToken));
	std::shared_ptr <SampleDCS::IClientDCS > clientDCS = SampleDCS::IClientDCS::MakeClientDCS(180, endPoint, accountID, appName, 10, connectionToken);

	return clientDCS;
}

void AStormancerAgent::StartStormancerClient()
{

	FString reason = "";

	// Check client is not null
	if (_clientDCS == nullptr)
	{
		OnClientConnectionFail("clientDCS is not set");
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("client start client"));
	std::string user = std::to_string(rand() % 1000);
	_clientDCS->RunClient(user)->Then([this](StormancerResult<void> r)
	{
		if (r.Success())
		{
			//Travel the client to a first map (lobby like map)
			UE_LOG(LogTemp, Warning, TEXT("Client RunClient"));
			_clientDCS->SetNextMap(TCHAR_TO_UTF8(*(_defaultMap)));
			_TravelAgentToMap();			
		}
		else
		{
			const FString reason = "Run Client task failed";
			OnClientConnectionFail(reason);
		}
	});
}

void AStormancerAgent::ResetObject(AActor* act)
{
	_clientDCS->OnConnectionStatusChange = [](int) {};
	this->OnDestroyed.RemoveDynamic(this, &AStormancerAgent::ResetObject);
}
void AStormancerAgent::SetStatusChangeFunction()
{
	if (_clientDCS == nullptr)
		return;

	_clientDCS->OnConnectionStatusChange = [this](int status) 
	{
		// Call OnConnectionStatusChange so we can catch this event in the blueprint
		_clientDCS->SetConnectionStatus(status);
		OnConnectionStatusChange((EStormancerConnectionStatus)status);
	};
}

void AStormancerAgent::_TravelAgentToMap()
{

	// Protection to avoid travelling before connection
	if (GetConnectionStatus() < EStormancerConnectionStatus::CONNECTING_LOCATOR)
		return;
	
	std::function<void(StormancerResult<SampleDCS::Endpoint>)> callbackEndpoint = [this](StormancerResult<SampleDCS::Endpoint> e) {

		UE_LOG(LogTemp, Warning, TEXT("Call back start client"));

		SampleDCS::Endpoint point = e.Get();
		FString reason = "";
		if (e.Success())
		{
			// Check host
			if (point.host.length() == 0)
			{
				reason = "Host not set in callback endpoint";
				OnClientConnectionFail(reason);
				return;
			}
			// Check port
			if (point.port == 0)
			{
				reason = "Port not set in callback endpoint";
				OnClientConnectionFail(reason);
				return;
			}

			//Travel
			FString hostString = FString(point.host.c_str());
			OnClientConnectionSucces(hostString, int32(point.port));
		}
		else
		{
			reason = FString(e.Reason().c_str());
			OnClientConnectionFail(reason);
		}
	};
	OnConnectionStatusChange(EStormancerConnectionStatus::CONNECTING_LOCATOR);
	_clientDCS->TravelToMap(_clientDCS->GetNextMap())->Then([callbackEndpoint](StormancerResult<SampleDCS::Endpoint> e)
	{
		callbackEndpoint(e);
	});
	_clientDCS->SetNextMap(""); // Reset next map 
}

void AStormancerAgent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);	
	if (EndPlayReason == EEndPlayReason::LevelTransition)
	{
		// _clientDCS is null if called from the server, so we need to check if it exist
		if(_clientDCS != nullptr)
			_clientDCS->OnConnectionStatusChange = [](int){};
	}
	if (EndPlayReason == EEndPlayReason::EndPlayInEditor || EndPlayReason == EEndPlayReason::Quit)
	{
		UUEStormancerPluginBPLibrary::clientDCSMap.clear();
		UUEStormancerPluginBPLibrary::serverDCSMap.clear();
	}
}