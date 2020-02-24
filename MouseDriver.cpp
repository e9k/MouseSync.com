// AntiRawInput.cpp : Defines the entry point for the console application.
//
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#include "stdafx.h"

#include "interception.h"
#include "utils.h"
#include <exception>
#include "json.hpp"
#include <iostream>
#include <windows.h>
#include <chrono>
using json = nlohmann::json;

using namespace std;
using namespace nlohmann;

static NTSTATUS(__stdcall *NtDelayExecution)(BOOL Alertable, PLARGE_INTEGER DelayInterval) = (NTSTATUS(__stdcall*)(BOOL, PLARGE_INTEGER)) GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtDelayExecution");

static NTSTATUS(__stdcall *ZwSetTimerResolution)(IN ULONG RequestedResolution, IN BOOLEAN Set, OUT PULONG ActualResolution) = (NTSTATUS(__stdcall*)(ULONG, BOOLEAN, PULONG)) GetProcAddress(GetModuleHandle(L"ntdll.dll"), "ZwSetTimerResolution");

void waiter(int micro) {
	auto start = std::chrono::high_resolution_clock::now();
	auto end = std::chrono::high_resolution_clock::now();
	auto elapsed = end - start;
	long long microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();


	while (microseconds <= (long long)micro) {
		end = std::chrono::high_resolution_clock::now();
		elapsed = end - start;
		microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
	}
}

int main(int argc, char* argv[])
{
	if (argc != 2) {
		printf("Invalid Arguments Supplied");
	}
	json config;
	try {
		std::cout << argv[1] << std::endl;
		config = json::parse(argv[1]);
	}
	catch (exception& e)
	{
		printf("Invalid Data Supplied");
	}
	InterceptionContext context;
	InterceptionDevice device, mouse = 0;
	InterceptionStroke stroke;
	
	//SetProcessAffinityMask(
	//	GetCurrentProcess(),
	//	0xC
	//);
	
	raise_process_priority();

	context = interception_create_context();
	interception_set_filter(context, interception_is_mouse,
		INTERCEPTION_FILTER_MOUSE_MOVE);

	double dx;
	double dy;
	double carryX = 0;
	double carryY = 0;
	double scale__x = config.count("scale__x") ? (double)config["scale__x"] : 1;
	double scale__y = config.count("scale__y") ? (double)config["scale__y"] : 1;


	double scale_x = config.count("scale_x") ? (double)config["scale_x"] : 1;
	double scale_y = config.count("scale_y") ? (double)config["scale_y"] : 1;
	double int_x, int_y;

	printf("Starting Driver...");
	ULONG actualResolution;
	ZwSetTimerResolution(1, true, &actualResolution);
	auto delta_start = std::chrono::high_resolution_clock::now();
	while (true)
	{
		device = interception_wait(context);
		auto start = std::chrono::high_resolution_clock::now();
		if (interception_is_mouse(device))
		{
				interception_receive(context, device, &stroke, 1);
				InterceptionMouseStroke &mstroke = *(InterceptionMouseStroke *)&stroke;
				mstroke.flags =  INTERCEPTION_MOUSE_MOVE_NOCOALESCE;

				dx = (double)mstroke.x;
				dy = (double)mstroke.y;

				dx *= scale__x;
				dx *= scale_x;

				dy *= scale__y;
				dy *= scale_y;

				if ((dx > 0.0 && carryX < 0.0)) {
					carryX = 0.0;
				}
				if ((dx < 0.0 && carryX > 0.0)) {
					carryX = 0.0;
				}

				if ((dy > 0.0 && carryY < 0.0)) {
					carryY = 0.0;
				}
				if ((dy < 0.0 && carryY > 0.0)) {
					carryY = 0.0;
				}

				dx += carryX;
				carryX = modf(dx, &int_x);
				mstroke.x = (int)int_x;

		
				dy += carryY;
				carryY = modf(dy, &int_y);
				mstroke.y = (int)int_y;


				auto end_time = std::chrono::high_resolution_clock::now();
				auto elapsed = end_time - start;

				long long microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
				delta_start = std::chrono::high_resolution_clock::now();

				waiter(50 - microseconds);
				
				interception_send(context, device, &stroke, 1);
		}
	}
	interception_destroy_context(context);

    return 0;
}

