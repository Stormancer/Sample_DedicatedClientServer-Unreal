// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "UEStormancerPluginBPLibrary.h"
#include "Endpoint.h"
#include "UEStormancerPlugin.h"

std::map<UGameInstance*, std::shared_ptr<SampleDCS::IClientDCS>> UUEStormancerPluginBPLibrary::clientDCSMap = std::map<UGameInstance *, std::shared_ptr<SampleDCS::IClientDCS>>();
std::map<UGameInstance*, std::shared_ptr<SampleDCS::IServerDCS>> UUEStormancerPluginBPLibrary::serverDCSMap = std::map<UGameInstance *, std::shared_ptr<SampleDCS::IServerDCS>>();

UUEStormancerPluginBPLibrary::UUEStormancerPluginBPLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{}


void UUEStormancerPluginBPLibrary::AddDCSClient(UGameInstance * ref, std::shared_ptr<SampleDCS::IClientDCS> client)
{
	clientDCSMap.insert(std::pair < UGameInstance *, std::shared_ptr<SampleDCS::IClientDCS>>(ref, client));
}

std::shared_ptr<SampleDCS::IClientDCS> UUEStormancerPluginBPLibrary::GetDCSClient(UGameInstance* ref)
{
	if (clientDCSMap.find(ref) == clientDCSMap.end())
		return nullptr;
	return clientDCSMap.at(ref);
}

void UUEStormancerPluginBPLibrary::AddDCSServer(UGameInstance * ref, std::shared_ptr<SampleDCS::IServerDCS> server)
{
	serverDCSMap.insert(std::pair < UGameInstance *, std::shared_ptr<SampleDCS::IServerDCS>>(ref, server));
}

std::shared_ptr<SampleDCS::IServerDCS> UUEStormancerPluginBPLibrary::GetDCSServer(UGameInstance* ref)
{
	if (serverDCSMap.find(ref) == serverDCSMap.end())
		return nullptr;
	return serverDCSMap.at(ref);
}

