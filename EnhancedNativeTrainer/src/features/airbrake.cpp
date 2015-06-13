/*
Part of the Enhanced Native Trainer project.
https://github.com/gtav-ent/GTAV-EnhancedNativeTrainer
(C) Sondai Smith and fellow contributors 2015
*/

#include "..\ui_support\menu_functions.h"
#include "airbrake.h"
#include "..\io\keyboard.h"
#include "..\io\config_io.h"
#include "script.h"

bool exitFlag = false;

char* AIRBRAKE_ANIM_A = "amb@world_human_stand_impatient@male@no_sign@base";
char* AIRBRAKE_ANIM_B = "base";

int travelSpeed = 0;

bool in_airbrake_mode = false;

bool frozen_time = false;

//Converts Radians to Degrees
float degToRad(float degs)
{
	return degs*3.141592653589793 / 180;
}

std::string airbrakeStatusLines[7];

DWORD airbrakeStatusTextDrawTicksMax;
bool airbrakeStatusTextGxtEntry;

void exit_airbrake_menu_if_showing()
{
	exitFlag = true;
}

//Test for airbrake command.
void process_airbrake_menu()
{
	exitFlag = false;

	const float lineWidth = 250.0;
	const int lineCount = 1;
	bool loadedAnims = false;

	std::string caption = "Airbrake";

	//draw_menu_header_line(caption,350.0f,50.0f,15.0f,0.0f,15.0f,false);

	DWORD waitTime = 150;

	Ped playerPed = PLAYER::PLAYER_PED_ID();
	bool inVehicle = PED::IS_PED_IN_ANY_VEHICLE(playerPed, 0) ? true : false;

	frozen_time = false;
	if (!inVehicle)
	{
		STREAMING::REQUEST_ANIM_DICT(AIRBRAKE_ANIM_A);
		while (!STREAMING::HAS_ANIM_DICT_LOADED(AIRBRAKE_ANIM_A))
		{
			WAIT(0);
		}
		loadedAnims = true;
	}

	while (true && !exitFlag)
	{
		in_airbrake_mode = true;

		// timed menu draw, used for pause after active line switch
		DWORD maxTickCount = GetTickCount() + waitTime;
		do
		{
			// draw menu
			draw_menu_header_line(caption, 350.0f, 50.0f, 15.0f, 0.0f, 15.0f, false);
			//draw_menu_line(caption, lineWidth, 15.0, 18.0, 0.0, 5.0, false, true);

			make_periodic_feature_call();
			WAIT(0);
		} while (GetTickCount() < maxTickCount);
		waitTime = 0;

		airbrake(inVehicle);

		//// process buttons
		//bool bSelect, bBack, bUp, bDown;
		//get_button_state(&bSelect, &bBack, &bUp, &bDown, NULL, NULL);
		if (airbrake_switch_pressed())
		{
			menu_beep();
			break;
		}
	}

	if (!inVehicle)
	{
		AI::CLEAR_PED_TASKS_IMMEDIATELY(PLAYER::PLAYER_PED_ID());
	}

	exitFlag = false;
	in_airbrake_mode = false;
}

void update_airbrake_text()
{
	if (GetTickCount() < airbrakeStatusTextDrawTicksMax)
	{
		int numLines = sizeof(airbrakeStatusLines) / sizeof(airbrakeStatusLines[0]);

		float textY = 0.1f;

		for (int i = 0; i < numLines; i++)
		{
			UI::SET_TEXT_FONT(0);
			UI::SET_TEXT_SCALE(0.3, 0.3);
			UI::SET_TEXT_COLOUR(255, 255, 255, 255);
			UI::SET_TEXT_WRAP(0.0, 1.0);
			UI::SET_TEXT_DROPSHADOW(1, 1, 1, 1, 1);
			UI::SET_TEXT_EDGE(1, 0, 0, 0, 305);
			if (airbrakeStatusTextGxtEntry)
			{
				UI::_SET_TEXT_ENTRY((char *)airbrakeStatusLines[i].c_str());
			}
			else
			{
				UI::_SET_TEXT_ENTRY("STRING");
				UI::_ADD_TEXT_COMPONENT_STRING((char *)airbrakeStatusLines[i].c_str());
			}
			UI::_DRAW_TEXT(0.01, textY);

			textY += 0.025f;
		}

		int screen_w, screen_h;
		GRAPHICS::GET_SCREEN_RESOLUTION(&screen_w, &screen_h);
		float rectWidthScaled = 350 / (float)screen_w;
		float rectHeightScaled = (20 + (numLines*18)) / (float)screen_h;
		float rectXScaled = 0 / (float)screen_h;
		float rectYScaled = 65 / (float)screen_h;

		int rect_col[4] = { 128, 128, 128, 75.0f };

		// rect
		draw_rect(rectXScaled, rectYScaled,
			rectWidthScaled, rectHeightScaled,
			rect_col[0], rect_col[1], rect_col[2], rect_col[3]);
	}
}

void create_airbrake_help_text()
{
	//Debug
	std::stringstream ss;

	/*ss << "Heading: " << curHeading << " Rotation: " << curRotation.z
	<< "\n xVect: " << xVect << "yVect: " << yVect;*/

	std::string travelSpeedStr;
	switch (travelSpeed)
	{
	case 0:
		travelSpeedStr = "Slow";
		break;
	case 1:
		travelSpeedStr = "Fast";
		break;
	case 2:
		travelSpeedStr = "Very Fast";
		break;
	}

	ss << "Current Travel Speed: " << travelSpeedStr;

	airbrakeStatusLines[0] = "Default Airbrake Keys (may be changed in config):";
	airbrakeStatusLines[1] = "Q/Z - Move Up/Down";
	airbrakeStatusLines[2] = "A/D - Rotate Left/Right";
	airbrakeStatusLines[3] = "W/S - Move Forward/Back";
	airbrakeStatusLines[4] = "Shift - Toggle Move Speed";
	airbrakeStatusLines[5] = "T - Toggle Frozen Time";
	airbrakeStatusLines[6] = ss.str();

	airbrakeStatusTextDrawTicksMax = GetTickCount() + 2500;
	airbrakeStatusTextGxtEntry = false;
}

void moveThroughDoor()
{
	// common variables
	Ped playerPed = PLAYER::PLAYER_PED_ID();

	if (PED::IS_PED_IN_ANY_VEHICLE(playerPed, 0))
	{
		return;
	}

	Vector3 curLocation = ENTITY::GET_ENTITY_COORDS(playerPed, 0);
	float curHeading = ENTITY::GET_ENTITY_HEADING(playerPed);

	float forwardPush = 0.6;

	float xVect = forwardPush * sin(degToRad(curHeading)) * -1.0f;
	float yVect = forwardPush * cos(degToRad(curHeading));

	ENTITY::SET_ENTITY_COORDS_NO_OFFSET(playerPed, curLocation.x + xVect, curLocation.y + yVect, curLocation.z, 1, 1, 1);
}

bool lshiftWasDown = false;

void airbrake(bool inVehicle)
{
	// common variables
	Ped playerPed = PLAYER::PLAYER_PED_ID();
	BOOL bPlayerExists = ENTITY::DOES_ENTITY_EXIST(playerPed);

	Vector3 curLocation = ENTITY::GET_ENTITY_COORDS(playerPed, 0);
	Vector3 curRotation = ENTITY::GET_ENTITY_ROTATION(playerPed, 0);
	float curHeading = ENTITY::GET_ENTITY_HEADING(playerPed);
	//float tmpHeading = curHeading += ;

	float rotationSpeed = 2.5;
	float forwardPush;

	switch (travelSpeed)
	{
	case 0:
		forwardPush = 0.2f;
		break;
	case 1:
		forwardPush = 1.8f;
		break;
	case 2:
		forwardPush = 3.6f;
		break;
	}

	float xVect = forwardPush * sin(degToRad(curHeading)) * -1.0f;
	float yVect = forwardPush * cos(degToRad(curHeading));

	KeyInputConfig* keyConfig = get_config()->get_key_config();

	bool moveUpKey = IsKeyDown(KeyConfig::KEY_AIRBRAKE_UP);
	bool moveDownKey = IsKeyDown(KeyConfig::KEY_AIRBRAKE_DOWN);
	bool moveForwardKey = IsKeyDown(KeyConfig::KEY_AIRBRAKE_FORWARD);
	bool moveBackKey = IsKeyDown(KeyConfig::KEY_AIRBRAKE_BACK);
	bool rotateLeftKey = IsKeyDown(KeyConfig::KEY_AIRBRAKE_ROTATE_LEFT);
	bool rotateRightKey = IsKeyDown(KeyConfig::KEY_AIRBRAKE_ROTATE_RIGHT);

	//Airbrake controls vehicle if occupied
	if (PED::IS_PED_IN_ANY_VEHICLE(playerPed, 0))
	{
		playerPed = PED::GET_VEHICLE_PED_IS_USING(playerPed);
	}

	BOOL xBoolParam = 1;
	BOOL yBoolParam = 1;
	BOOL zBoolParam = 1;

	ENTITY::SET_ENTITY_VELOCITY(playerPed, 0, 0, 0);
	ENTITY::SET_ENTITY_ROTATION(playerPed, 0, 0, 0, 0, false);
	ENTITY::SET_ENTITY_COORDS_NO_OFFSET(playerPed, curLocation.x, curLocation.y, curLocation.z, xBoolParam, yBoolParam, zBoolParam);
	ENTITY::SET_ENTITY_HEADING(playerPed, curHeading);

	if (!inVehicle)
	{
		AI::TASK_PLAY_ANIM(PLAYER::PLAYER_PED_ID(), AIRBRAKE_ANIM_A, AIRBRAKE_ANIM_B, 8.0f, 0.0f, -1, 9, 0, 0, 0, 0);
	}

	if (IsKeyJustUp(KeyConfig::KEY_AIRBRAKE_SPEED))
	{
		travelSpeed++;
		if (travelSpeed > 2)
		{
			travelSpeed = 0;
		}
	}

	if (IsKeyJustUp(KeyConfig::KEY_AIRBRAKE_FREEZE_TIME))
	{
		frozen_time = !frozen_time;
	}

	create_airbrake_help_text();
	update_airbrake_text();

	if (moveUpKey){ ENTITY::SET_ENTITY_COORDS_NO_OFFSET(playerPed, curLocation.x, curLocation.y, curLocation.z + forwardPush / 2, xBoolParam, yBoolParam, zBoolParam); }
	if (moveDownKey){ ENTITY::SET_ENTITY_COORDS_NO_OFFSET(playerPed, curLocation.x, curLocation.y, curLocation.z - forwardPush / 2, xBoolParam, yBoolParam, zBoolParam); }
	if (rotateLeftKey)
	{
		ENTITY::SET_ENTITY_HEADING(playerPed, curHeading + rotationSpeed);
	}
	else if (rotateRightKey)
	{
		ENTITY::SET_ENTITY_HEADING(playerPed, curHeading - rotationSpeed);
	}

	/*
	std::stringstream ss2;
	ss2 << "Angle: " << curHeading << "\nXV: " << xVect << "\nYV: " << yVect << "\nCRZ: " << curRotation.z;
	set_status_text(ss2.str());
	*/

	if (moveForwardKey)
	{
		if (moveUpKey){ ENTITY::SET_ENTITY_COORDS_NO_OFFSET(playerPed, curLocation.x + xVect, curLocation.y + yVect, curLocation.z + forwardPush / 2, xBoolParam, yBoolParam, zBoolParam); }
		else if (moveDownKey){ ENTITY::SET_ENTITY_COORDS_NO_OFFSET(playerPed, curLocation.x + xVect, curLocation.y + yVect, curLocation.z - forwardPush / 2, xBoolParam, yBoolParam, zBoolParam); }
		else{ ENTITY::SET_ENTITY_COORDS_NO_OFFSET(playerPed, curLocation.x + xVect, curLocation.y + yVect, curLocation.z, xBoolParam, yBoolParam, zBoolParam); }
	}
	else if (moveBackKey)
	{
		if (moveUpKey){ ENTITY::SET_ENTITY_COORDS_NO_OFFSET(playerPed, curLocation.x - xVect, curLocation.y - yVect, curLocation.z + forwardPush / 2, xBoolParam, yBoolParam, zBoolParam); }
		else if (moveDownKey){ ENTITY::SET_ENTITY_COORDS_NO_OFFSET(playerPed, curLocation.x - xVect, curLocation.y - yVect, curLocation.z - forwardPush / 2, xBoolParam, yBoolParam, zBoolParam); }
		else{ ENTITY::SET_ENTITY_COORDS_NO_OFFSET(playerPed, curLocation.x - xVect, curLocation.y - yVect, curLocation.z, xBoolParam, yBoolParam, zBoolParam); }
	}
}

bool is_in_airbrake_mode()
{
	return in_airbrake_mode;
}

bool is_airbrake_frozen_time()
{
	return frozen_time;
}