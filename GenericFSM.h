#pragma once
//
// MySETIBCA, an application for decoding, encoding message images using 
// a block cellular automata like what was used in the 'A Sign inSpace' project message
// 
// GenericFSM.cpp
// (C) 2024, Mark Stegall
// Author: Mark Stegall
//
// This file is part of MySETIBCA.
//
// MySETIBCA is free software : you can redistribute it and /or modify it under
// the terms of the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// MySETIBCA is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
// without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
// You should have received a copy of the GNU General Public License along with MySETIBCA.
// If not, see < https://www.gnu.org/licenses/>. 
//
// Application standardized error numbers for functions that perform transform processes:
//      1 - success
//      0 - parameter or image header problem
//     -1 memory allocation failure
//     -2 open file failure
//     -3 file read failure
//     -4 incorect file type
//     -5 file sizes mismatch
//     -6 not yet implemented
//     -7 File write error
//
// Some function return TRUE/FALSE or int results
//
// V1.1.8	2024-12-18	Added GenericFSM class
//
//  This contains the Finite State Machine declarations and variables
// 
//	This implements an asynchronous Mealy FSM, event drive by input
// 
//	The FSM requires:
//
//		Input alphabet
//			The input alphabet can be any any string, it can not start with whitespace
//
//		Output alphabet
//			The output alphabet can be any any string, it can not start with whitespace
//			For single character space output use the string <space> as the definition
//			The output alphabet can include a no output entry, use <no> as the definition
// 
//		Next state rules
//			There are # Input Alphabet * # Output Alphabet rules
//			Rules are ordered by state,input and specifiy the next state
//
//		Output rules
//			There are # Input Alphabet * # Output Alphabet rules
//			Rules are ordered by state,input and specifiy the output
//
//		Intial State
//			This is the starting state for the FSM
// 
//	Operation is very simple
// 
//		Current State = Initial State
//		while (get input)
//			Output = Output rule (Current State, input)
//			Current state = Next state Rule (Current State, input)
//		endwhile
//
#include "framework.h"

#define	WARNING_FSM_NEXT_STATES_RULES	0x1
#define WARNING_FSM_OUTPUT_RULES		0x2
#define WARNING_FSM_STATES_UNREACHABLE	0x4
#define WARNING_FSM_OUTPUTS_UNUSED		0x8
#define ERROR_FSM_NO_START_STATE		0x10
#define ERROR_FSM_NO_INPUTS				0x20
#define ERROR_FSM_NO_OUTPUTS			0x40
#define ERROR_FSM_NO_STATES				0x80
#define ERROR_FSM_NO_STATE_RULES		0x100
#define ERROR_FSM_NO_OUTPUT_RULES		0x200

#define MAX_SYMBOL_SIZE	128

class GenericFSM {
private:
	// variables
	// InputAlphabet
	int NumInputs = 0;
	WCHAR** InputSymbols = nullptr;
	
	// OutputAlphabet
	int NumOutputs = 0;
	WCHAR** OutputSymbols = nullptr;

	// States labels
	int NumStates = 0;
	WCHAR** StateLabels = nullptr;

	// States check (number of ways this state can be reached)
	int* CheckStates = nullptr;

	// Outputs check (number of ways this output can be done)
	int* CheckOutputs = nullptr;

	// Next State rules
	// There are NumInputs * NumStates rules
	int* NextStateRules = { nullptr };
	
	// Output Rules
	// There are NumInputs * NumStates rules
	int* OutputRules = { nullptr };

	// Current State
	int CurrentState=-1;	// no FSM exists yet

	// Current Output
	int CurrentOutput = -1;  //can not have output until there is input

	// Starting state
	int StartState = -1;

	// forward method/function declarations
	//	method/functions definition are done in GenericFSM.cpp

	void DeleteFSM();
	int ReadDelimiters(WCHAR* INIfilename);

	// variables
	WCHAR ConfigurationFile[MAX_PATH] = L"";
	WCHAR Delimiters[MAX_SYMBOL_SIZE] = L", \t-";

public:

	// forward method/function declarations
	//	method/functions definition are done in GenericFSM.cpp

	// class constructor
	GenericFSM();
	// class destructor
	~GenericFSM();

	// load FSM from ini configuration file
	int LoadFSM(WCHAR* Filename, int* WarningFlags);

	// functions to run FSM
	int ResetFSM(); // resets current state to 0
	int StepFSM(int index); // sets current state according to Next state rule
	int StepFSM(WCHAR* Token);
	int GetCurrentOutput(WCHAR* Output, int MaxLength); // returns current output symbol
	int GetCurrentStateIndex();
	int GetDelimiters(WCHAR* Delimiter, int MaxLength);

	// information retrieval for the FSM
	int GetNumInputAlphabet(); // returns the number of symbols in the input alphabet
	int GetNumOutputAlpabet(); // returns the number of symbols in the output alphabet
	int GetNumStates(); // returns the number of states
	
	int GetStateLabel(int index, WCHAR* label, size_t MaxLabelLength); // returns label for State
	int GetInputAlphabetLabel(int index, WCHAR* label, size_t MaxLabelLength); // returns the input symbol for index
	int GetOutputAlphabetLabel(int index, WCHAR* label, size_t MaxLabelLength); // returns the input symbol for index

	int GetNextStateRule(int StateIndex, int InputIndex);
	int GetOuputRule(int StateIndex, int InputIndex);
	BOOL IsStateUsed(int StateIndex);
	BOOL IsOutputUsed(int OutputIndex);
};