/****************************** Header ******************************\
Class Name: Robot inherits SampleRobot
File Name: Robot.cpp
Summary: Entry point from WPIlib, main class where robot routines are
started.
Project: BroncBotzFRC2019
Copyright (c) BroncBotz.
All rights reserved.

Author(s): Ryan Cooper, Dylan Watson, Chris Weeks
Email: cooper.ryan@centaurisoftware.co, dylantrwatson@gmail.com, 
chrisrweeks@aol.com
\********************************************************************/

#include <iostream>
#include "Robot.h"
#include "Util/RobotStatus.h"
#include "Util/FrameworkCommunication.h"
using namespace std;

#ifdef __Use_RobotBase__

/**
 * Constructor
 */
Robot::Robot() 
{
	m_activeCollection = new ActiveCollection();
	m_drive = new Drive(m_activeCollection);
	//extend the life of the active region's copy by adding a reference to this variable
	//TODO: Events
}

Robot::~Robot()
{
	delete m_drive;
	m_drive = nullptr;
	delete m_activeCollection;
	m_activeCollection = nullptr;
}

void Robot::LoadConfig(bool RobotRunning)
{
	nt::NetworkTableInstance::GetDefault().GetTable("SmartDashboard")->PutBoolean("RUN_ROBOT", false);

	if(m_Config != nullptr)
		delete m_Config;
	m_Config = new Config(m_activeCollection, m_drive); //!< Pointer to the configuration file of the robot

	if(nt::NetworkTableInstance::GetDefault().GetTable("SmartDashboard")->GetBoolean("0A-RESET_ROBOT_VALUES", false) && !RobotRunning)
	{
		vector<string> KeysNT = nt::NetworkTableInstance::GetDefault().GetTable("SmartDashboard")->GetKeys();
		for(int i = 0; i < KeysNT.size(); i++)
			if(KeysNT.at(i).compare("3A_Auto_Selector") != 0)
				nt::NetworkTableInstance::GetDefault().GetTable("SmartDashboard")->PutValue(KeysNT.at(i), 0);
	}
	nt::NetworkTableInstance::GetDefault().GetTable("SmartDashboard")->PutBoolean("0A-RESET_ROBOT_VALUES", false);
	
	nt::NetworkTableInstance::GetDefault().GetTable("SmartDashboard")->PutBoolean("RUN_ROBOT", RobotRunning);
}

/*
 * Initialization method of the robot
 * This runs right after Robot() when the code is started
 * Creates Config
 */
void Robot::RobotInit()
{
	FrameworkCommunication::GetInstance();
	Log::restartfile();
	Robot::LoadConfig(false);
	Log::General("Program Version: " + to_string(VERSION) + " Revision: " + REVISION, true);
	SmartDashboard::init(); //!< Must have this for smartdashboard to work properly
	m_inst = nt::NetworkTableInstance::GetDefault(); //!< Network tables
	m_dashboardTable = m_inst.GetTable("DASHBOARD_TABLE");
	m_dashboardTable->PutStringArray("AUTON_OPTIONS", m_autonOptions);
	m_dashboardTable->PutStringArray("POSITION_OPTIONS", m_positionOptions);
	nt::NetworkTableInstance::GetDefault().GetTable("SmartDashboard")->PutString("3A_Auto_Selector", "????");
	// Util::FrameworkCommunication::GetInstance().SendData("MESSAGE","yeetus");//? Temporary
}

/*
 * Called when the Test period starts
 */
void Robot::Test()
{
	Robot::LoadConfig(true);
	
	Util::RobotStatus::GetInstance().NotifyState(Util::RobotState::Test);
	nt::NetworkTableInstance::GetDefault().GetTable("SmartDashboard")->PutNumber("Loop", 0);
	while(!IsDisabled() && IsTest())
	{
		double I = nt::NetworkTableInstance::GetDefault().GetTable("SmartDashboard")->GetNumber("Loop", 0) + 1;
		nt::NetworkTableInstance::GetDefault().GetTable("SmartDashboard")->PutNumber("Loop", I);
		Log::General("Running TEST!!!!!!!!!!!!!!!!!!!!!!!!!: " + to_string(I));
		LoopWait(m_activeCollection->GetWaitTime());
	}

	/*
	m_masterGoal = new MultitaskGoal(m_activeCollection, false);
	Log::General("Autonomous Started");
	//TODO: Make defaults set now and call the active collection
	m_activeCollection->DoubleSolenoidDefault();
#ifndef _Win32
	string autoSelected = m_dashboardTable->GetString("AUTON_SELECTION", m_driveStraight);
	string positionSelected = m_dashboardTable->GetString("POSITION_SELECTION", "NONE"); // Default auto is drive striaght 
	if (!SelectAuton(m_activeCollection, m_masterGoal, autoSelected, positionSelected)) // Selection
	{
		m_dashboardTable->PutString("AUTON_FOUND", "UNDEFINED AUTON OR POSITION SELECTED");
	}
#endif
	m_masterGoal->AddGoal(new Goal_TimeOut(m_activeCollection, 15.0));
	m_masterGoal->AddGoal(new Goal_ControllerOverride(m_activeCollection)); //!< This is for FRC 2019 SANDSTORM! Be aware that if Sandstorm is removed, this NEEDS to be removed.
	//TODO: Make the auto configurable (turn on/turn off) OR add a no auton feature to the dashboard
	m_masterGoal->Activate();
	m_activeCollection->SetRobotGoal(m_masterGoal);
	double dTime = 0.010;
	double current_time = 0.0;
	while (m_masterGoal->GetStatus() == Goal::eActive && IsTest() && !IsDisabled())
	{
		m_drive->Update(dTime); //!< Check for controller input for controller override -> will be removed if sandstorm is removed
		m_masterGoal->Process(dTime); //!< Process the autonomous goal
		current_time += dTime;
#ifdef _Win32
		SmartDashboard::PutNumber("DELTA TIME", current_time);
		SmartDashboard::PutString("AUTONOMOUS", "RUNNING");
#endif		
		Wait(dTime);
	}
	/* THIS LOOP WILL HAVE TO BE REMOVED IF SANDSTORM IS REMOVED *
	while(IsTest() && !IsDisabled()) //!< Loop to run manual control once the auto function is done in sandstorm
	{
#ifdef _Win32
		SmartDashboard::PutString("AUTONOMOUS", "OVERRIDEN!");
#endif
		m_drive->Update(dTime); //!< Run robot teleOp
		Wait(dTime);
	}
	m_masterGoal->~MultitaskGoal(); //!< Destroy the masterGoal in order to prevent multiple ControllerOverrides running at once
	*/
}

/*
 * Called when teleop starts
 */
//test method doesnt work in rio for some reason...
//TODO: Potentially make a "test" tag in the config that can toggle this?
void Robot::Teleop()
{
	Robot::LoadConfig(true);

	Util::RobotStatus::GetInstance().NotifyState(Util::RobotState::Teleop);
	m_activeCollection->ResetSuperior_Goal(); //!< Destroy any pre-existing masterGoal that was not properly disposed of
	
	//m_teleOpMasterGoal->AddGoal(new Goal_TimeOut(m_activeCollection, 15));
	//m_teleOpMasterGoal->AddGoal(new Goal_ControllerOverride(m_activeCollection));
	// m_activeCollection->GetActiveGoal()->AddGoal(new Goal_TimeOut(m_activeCollection, 3000));
	// m_activeCollection->GetActiveGoal()->Activate();
	//TODO: Talk to Ian about this
	Log::restartfile();
	Log::General("Teleoperation Started.");
	double LastTime = GetTime();
	//We can test teleop auton goals here a bit later
	//PotentiometerItem* pot = (PotentiometerItem*)m_activeCollection->Get("pot");
	if (m_activeCollection->Get("LimeLight") != nullptr)
		limelight* lime = (limelight*)(m_activeCollection->Get("LimeLight"));
	if (m_activeCollection->GetNavX() != nullptr)
		m_activeCollection->GetNavX()->ResetNav();
	while (IsOperatorControl() && !IsDisabled())
	{
		const double CurrentTime = GetTime();
		#ifndef _Win32
		const double DeltaTime = CurrentTime - LastTime;
		#else
		const double DeltaTime=0.01;  //It's best to use sythetic time for simulation to step through code
		#endif
		LastTime = CurrentTime;
		if (DeltaTime == 0.0) continue;  //never send 0 time
		m_drive->Update(DeltaTime);
		LoopWait(m_activeCollection->GetWaitTime());
	}
}

/*
 * Method that runs at the beginning of the autonomous mode
 * Uses the SmartDashboard to select the proper Autonomous mode to run
 */
void Robot::Autonomous()
{
	Robot::LoadConfig(true);
	Util::RobotStatus::GetInstance().NotifyState(Util::RobotState::Auton);
	string SELECTED_AUTO = "";
	if (m_activeCollection->ConfigOverride())
	{
		SELECTED_AUTO = m_activeCollection->GetAuto();
	}
	else
	{
		SELECTED_AUTO = AutoTable->GetString("3A_Auto_Selector", "") + ".txt";
	}
	
	Log::General("!--------------- " + SELECTED_AUTO + " AUTO Selected---------------!");
	//! DO NOT CALL THE EVENT FOR NOTIFYROBOTSTATE AT THIS TIME!
	AutoPath* PathA = new AutoPath(m_activeCollection, Map(SELECTED_AUTO), 10, m_activeCollection->IsSwerveDrive(), m_activeCollection->GetAutoScale());
	PathA->Activate();
	while(PathA->GetStatus() == Goal::eActive && !IsDisabled() && _IsAutononomous())
	{
		PathA->Process(0.0001);
		LoopWait(m_activeCollection->GetWaitTime());
	}
	PathA->Terminate();
}

void Robot::StartCompetition() {
  auto& lw = *frc::LiveWindow::GetInstance();

  RobotInit();
  // Tell the DS that the robot is ready to be enabled
  HAL_ObserveUserProgramStarting();

  while (!m_exit) {
    if (IsDisabled()) {
      m_ds.InDisabled(true);
      Disabled();
      m_ds.InDisabled(false);
      while (IsDisabled()) m_ds.WaitForData();
    }
	else if (IsAutonomous()) {
      m_ds.InAutonomous(true);
      Autonomous();
      m_ds.InAutonomous(false);
      while (IsAutonomous() && IsEnabled()) m_ds.WaitForData();
    }
	else if (IsTest()) {
      lw.SetEnabled(true);
      frc::Shuffleboard::EnableActuatorWidgets();
      m_ds.InTest(true);
      Test();
      m_ds.InTest(false);
      while (IsTest() && IsEnabled()) m_ds.WaitForData();
      lw.SetEnabled(false);
      frc::Shuffleboard::DisableActuatorWidgets();
    }
	else {
      m_ds.InOperatorControl(true);
      Teleop();
      m_ds.InOperatorControl(false);
      while (IsOperatorControl() && IsEnabled()) m_ds.WaitForData();
    }
  }
}

void Robot::EndCompetition() { m_exit = true; }

void Robot::Disabled() { 
	nt::NetworkTableInstance::GetDefault().GetTable("SmartDashboard")->PutBoolean("RUN_ROBOT", false);
	Util::RobotStatus::GetInstance().NotifyState(Util::RobotState::Disabled);
	while(IsDisabled())
		if(nt::NetworkTableInstance::GetDefault().GetTable("SmartDashboard")->GetBoolean("0A-RESET_ROBOT_VALUES", false))
			Robot::LoadConfig(false);
 }

void Robot::LoopWait(double Time)
{
	LastClockRef = m_Time.GetFPGATimestamp();
	Wait(Time);
	if(LastClockRef == m_Time.GetFPGATimestamp())
	{
		LoopWait(Time);
	}
}

#ifndef RUNNING_FRC_TESTS
int main() { return frc::StartRobot<Robot>(); }  //!< This identifies Robot as the main Robot starting class
#endif
#endif