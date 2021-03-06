/****************************** Header ******************************\
Class Name: TalonSRXItem inherits OutputComponent
File Name:	TalonSRXItem.cpp
Summary: Abstraction for the WPIlib TalonSRX that extends to include
some helper and control methods.
Project:     BroncBotzFRC2019
Copyright (c) BroncBotz.
All rights reserved.

Author(s): Dylan Watson
Email: dylantrwatson@gmail.com
\********************************************************************/

#include <iostream>

#include "TalonSRXItem.h"

using namespace std;
using namespace frc;
using namespace Components;

TalonSRXItem::TalonSRXItem() {}

TalonSRXItem::TalonSRXItem(int _channel, string _name, bool _reversed, bool enableEncoder, bool Real)
	: Motor(_name, _reversed){
	channel = _channel;
	reversed = _reversed;
	talon = new WPI_TalonSRX(channel);
	encoderEnabled = enableEncoder;
	FromTable(Real);
	Offset = OutputTable->GetNumber(name + "-Encoder", 0);
	if(enableEncoder)
	{
		talon->ConfigSelectedFeedbackSensor(FeedbackDevice::QuadEncoder, 0, 10);
		{
			Log::General("Using Table values");
			OutputTable->PutNumber(name + "-Encoder", 0);
			OutputTable->PutBoolean(name + "-Reset", true);
		}
	}
}

double TalonSRXItem::Get(){
    if (reversed)
        return talon->GetMotorOutputPercent() * -1;
    return talon->GetMotorOutputPercent();
}

int TalonSRXItem::GetQuadraturePosition(){
	return (encoderEnabled ? (UseTable ? OutputTable->GetNumber(name + "-Encoder", 0) : talon->GetSensorCollection().GetQuadraturePosition()) : -1) - Offset;
}

void TalonSRXItem::SetQuadraturePosition(int val){
	talon->GetSensorCollection().SetQuadraturePosition(val, 10);
	if (UseTable)
	{
		OutputTable->PutBoolean(name + "-Reset", true);
	}
}
void TalonSRXItem::sim_SetQuadratureRawPosition(double new_pos)
{
	//we factor our own inverse DPP
	//talon->GetSimCollection().SetQuadratureRawPosition((int)(new_pos*(1.0/m_DPP)));    
	//For now DPP must be managed by the caller
	talon->GetSimCollection().SetQuadratureRawPosition((int)(new_pos));    
}
void TalonSRXItem::sim_SetQuadratureVelocity(double newRate_s)
{
	//in native units per 100ms
	talon->GetSimCollection().SetQuadratureVelocity((int)(newRate_s*10.0));
}

void TalonSRXItem::Set(double val){
	val = CalculateVal(val);
	//Log::General(name+" : "  + to_string(val));
	if((val<0 || val>0) && !inUse)
	{
		inUse = true;
		if(reversed) talon->Set(ControlMode::PercentOutput, -val);
		else talon->Set(ControlMode::PercentOutput, val);
		inUse = false;
	}
	else if(!inUse){
		inUse = true;
		talon->Set(ControlMode::PercentOutput, 0);
		inUse = false;
	}
}

void TalonSRXItem::DefaultSet(){
	Log::Error("WHY DID YOU CALL THE DEFAULT SET FOR A MOTOR?!? Yell at your programmers!");
}

void TalonSRXItem::Set(DoubleSolenoid::Value value){
	Log::Error("WHY DID YOU CALL THE DEFAULT SET FOR A MOTOR?!? Yell at your programmers!");
}

void TalonSRXItem::Stop()
{
	TalonSRXItem::Set(0);
}

void TalonSRXItem::DeleteComponent()
{
	CleanUpProfiles();
	delete talon;
	delete this;
}

void TalonSRXItem::UpdateComponent()
{
	if (!UseTable)
	{
		OutputTable->PutNumber(name + "-Encoder", TalonSRXItem::GetQuadraturePosition());
	}
}

TalonSRXItem::~TalonSRXItem() {}

