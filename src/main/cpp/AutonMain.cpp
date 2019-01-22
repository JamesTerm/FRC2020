#include "Base/Base_Includes.h"
#include <math.h>
#include <assert.h>
#include "Base/Vec2d.h"
#include "Base/Misc.h"
#include "Base/Event.h"
#include "Base/EventMap.h"
#include "Base/Script.h"
#include "Common/Entity_Properties.h"
#include "Common/Physics_1D.h"
#include "Common/Physics_2D.h"
#include "Common/Entity2D.h"
#include "Common/Goal.h"
#include "Common/Ship_1D.h"
#include "Common/Ship.h"
#include "Common/AI_Base_Controller.h"
#include "Common/Vehicle_Drive.h"
#include "Common/PIDController.h"
#include "Common/Poly.h"
#include "Common/Robot_Control_Interface.h"
#include "Common/Rotary_System.h"
#include "Common/Servo_System.h"
#include "Base/Joystick.h"
#include "Base/JoystickBinder.h"
#include "Common/UI_Controller.h"
#include "Common/PIDController.h"
#include "Base/Joystick.h"
#include "Base/JoystickBinder.h"
#ifndef _Win32
#include <frc/WPILib.h>
#include "Common/InOut_Interface.h"
#else
#include "Common/Calibration_Testing.h"
#endif
#include "Common/Debug.h"
#include "Common/Robot_Control_Common.h"
#include "TankDrive/Tank_Robot.h"

#include "FRC2019_Robot.h"
#include "Common/SmartDashboard.h"


#include "AutonMain.h"

//This is only for diagnostics... if there are problems with the robot control
class FRC_2019_Control : public FRC2019_Control_Interface
{
public:
	//from Tank_Drive_Control_Interface,
	virtual void Tank_Drive_Control_TimeChange(double dTime_s) {}
	//We need to pass the properties to the Robot Control to be able to make proper conversions.
	//The client code may cast the properties to obtain the specific data 
	virtual void Initialize(const Entity_Properties *props) {}
	virtual void Reset_Encoders() {}

	//Encoders populate this with current velocity of motors
	virtual void GetLeftRightVelocity(double &LeftVelocity, double &RightVelocity) { LeftVelocity = RightVelocity = 0.0; }
	virtual void UpdateLeftRightVoltage(double LeftVoltage, double RightVoltage) {}

	//from Robot_Control_Interface,
	virtual void UpdateVoltage(size_t index, double Voltage) {}
	//Having both Open and Close makes it easier to make the desired call without applying the not operator
	virtual void OpenSolenoid(size_t index, bool Open = true) {}
	virtual void CloseSolenoid(size_t index, bool Close = true) { OpenSolenoid(index, !Close); }
	virtual bool GetIsSolenoidOpen(size_t index) const { return false; }
	virtual bool GetIsSolenoidClosed(size_t index) const { return !GetIsSolenoidOpen(index); }
	/// \ret true if contact is made 
	virtual bool GetBoolSensorState(size_t index) const { return false; }

	//from  Rotary_Control_Interface
	virtual void Reset_Rotary(size_t index = 0) {}
	virtual double GetRotaryCurrentPorV(size_t index = 0) { return 0.0; }
	virtual void UpdateRotaryVoltage(size_t index, double Voltage) {}

	//This is primarily used for updates to dashboard and driver station during a test build
	virtual void Robot_Control_TimeChange(double dTime_s) {}
	//We need to pass the properties to the Robot Control to be able to make proper conversions.
	//The client code may cast the properties to obtain the specific data 
	//virtual void Initialize(const Entity_Properties *props)	{}
};

class AutonMain_Internal
{
private:
#if 0
	//keep around for diagnostics //taking away control stress
	FRC_2019_Control m_Control;
#else
	FRC2019_Robot_Control m_Control;
#endif
	FRC2019_Robot_Properties m_RobotProps;
	FRC2019_Robot *m_pRobot = nullptr;
	//Framework::UI::JoyStick_Binder m_JoyBinder;
	UI_Controller *m_pUI;

	Framework::Base::EventMap m_EventMap;
	std::string m_LuaPath;  //keep a copy of the lua path
public:
	void InitRobot()
	{
		const bool UseEncoders = false;
		m_pRobot = new FRC2019_Robot("FRC2019_Robot", &m_Control, UseEncoders);
		{
			Framework::Scripting::Script script;
			script.LoadScript(m_LuaPath.c_str(), true);
			script.NameMap["EXISTING_ENTITIES"] = "EXISTING_SHIPS";
			m_RobotProps.SetUpGlobalTable(script);
			m_RobotProps.LoadFromScript(script);
			//m_Joystick.SetSlotList(m_RobotProps.Get_RobotControls());
			m_pRobot->Initialize(m_EventMap, &m_RobotProps);
		}
		//The script must be loaded before initializing control since the wiring assignments are set there
		//m_Control.AsControlInterface().Initialize(&m_RobotProps);
					//Bind the ship's eventmap to the joystick
		//m_JoyBinder.SetControlledEventMap(m_pRobot->GetEventMap());

		//To to bind the UI controller to the robot
		AI_Base_Controller *controller = m_pRobot->GetController();
		assert(controller);
		//m_pUI = new UI_Controller(m_JoyBinder, controller);
		m_pUI = new UI_Controller(nullptr, controller);
		if (controller->Try_SetUIController(m_pUI))
		{
			//Success... now to let the entity set things up
			m_pUI->HookUpUI(true);
		}
		else
		{
			m_pUI->Set_AI_Base_Controller(NULL);   //no luck... flush ship association
			assert(false);
		}

		//start in auton (can manage this later)
		SmartDashboard::PutBoolean("Test_Auton", true);
		SmartDashboard::PutNumber("AutonTest", 0.0);
	}

	AutonMain_Internal(const char *RobotLua)
	{
		m_LuaPath = RobotLua;
		//Hook in our own victor allocator here
		m_Control.SetExternalVictorHook(
		[&](size_t module, size_t Channel, const char *Name)
		{
			//TODO hook our active collection here
			//printf("Robot: Get External Victor %s[%d,%d]\n",Name,module,Channel);
			return nullptr;
		});
		//Note: For Simulation, Tank_Robot_Control needs __Tank_TestControlAssignments__ defined; otherwise there are no hooks to set
		//Swerve drive merged both techniques, and eventually Tank could do the same, for now, they are still separate
		m_Control.SetDriveExternalVictorHook(
			[&](size_t module, size_t Channel, const char *Name)
		{
			//TODO hook our active collection here
			//printf("Drive: Get External Victor %s[%d,%d]\n", Name, module, Channel);
			return nullptr;
		});

		//We can call init now:
		InitRobot();
	}
	~AutonMain_Internal()
	{
		delete m_pRobot;  //checks for null implicitly 
		m_pRobot = nullptr;
	}

	void Update(double dTime_s)
	{
		if (m_pRobot)
			m_pRobot->TimeChange(dTime_s);
	}
};

void AutonMain::AutonMain_init(const char *RobotLua)
{
	m_p_AutonMain = std::make_shared<AutonMain_Internal>(RobotLua);
}

void AutonMain::Update(double dTime_s)
{
	m_p_AutonMain->Update(dTime_s);
}