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
// This file contains the definitions of the GenericFSM class methods/functions
// 
// V1.1.8	2024-12-18	Added Generic Finite State Machine class
//						output rules special entries,
//							<space>		a space character is output
//							<no>		no output symbol (empty)
//
// Application standardized error numbers for functions:
//		See AppErrors.h
//
//
//  This contains the Finite State Machine definition and procedures
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
//
#include "framework.h"
#include <shtypes.h>
#include <string.h>
#include <stdio.h>
#include "AppErrors.h"
#include "GenericFSM.h"

//*******************************************************************************
//
//  GenericFSM()
//  class constructor
// 
//*******************************************************************************
GenericFSM::GenericFSM()
{
	return;
}

//*******************************************************************************
//
//  ~GenericFSM()
//  class destructor
// 
//*******************************************************************************
GenericFSM::~GenericFSM()
{
	DeleteFSM();
	return;
}

//*******************************************************************************
//
//  LoadFSM(WCHAR* Filename, BOOL WarningsFlag)
//  
//	Read the FSM in from the configuration file
// 
//  parmameters
// 
//	WCHAR *Filename		INI file containing the FSM definition
//  int*  WarningsFlag	returns bit flags indicating warning:
//						WARNING_FSM_NEXT_STATES_RULES	0x001
//						WARNING_FSM_OUTPUT_RULES		0x002
//						WARNING_FSM_STATES_UNREACHABLE	0x004
//						WARNING_FSM_OUTPUTS_UNUSED		0x008
//						ERROR_FSM_NO_START_STATE		0x010
//						ERROR_FSM_NO_INPUTS				0x020
//						ERROR_FSM_NO_OUTPUTS			0x040
//						ERROR_FSM_NO_STATES				0x080
//						ERROR_FSM_NO_STATE_RULES		0x100
//						ERROR_FSM_NO_OUTPUT_RULES		0X200
//
// return
//		see AppErrors.h file for standard erro list
// 
//*******************************************************************************
int GenericFSM::LoadFSM(WCHAR* Filename, int* WarningFlags)
{
	(*WarningFlags) = 0;
	int tmpNumInputs = 0;
	int tmpNumOutputs = 0;
	int tmpNumStates = 0;
	int tmpStartState = -1;

	// The input containing the FSM definition is an INI conifiguration file
	// You can have comment lines starting with ; or // in an ini conifiguration
	// The order of entries under a section does not matter or line spacing

	// A list of delimiters to be used by external functions
	// that are appropriate for extracting input token form a string for this FSM
	// [delmiters]
	// 0=<space>
	// 1=<tab>
	// 2=-
	// 3=,
	// 
	if (ReadDelimiters(Filename) != APP_SUCCESS) {
		return APPERR_PARAMETER;
	}

	// 
	// The starting state must be defined in the [states] section using the 'start' key
	// example:
	// [states]
	//		start=0
	tmpStartState = GetPrivateProfileInt(L"states", L"start", -1, (LPCTSTR)Filename);
	if (tmpStartState == -1) {
		(*WarningFlags) = ERROR_FSM_NO_START_STATE;
		return APPERR_PARAMETER;
	}

	// Determine the size of the Input Alphabet
	//	The input alphabet in under the INI section [input]
	//	entries are of the form of (0 to n-1), n is the number of symbols
	//	example for 3 symbols, symbols can be any string (please do not use comma in symbol)
	//  The index number must start with 0 and end with n-1. Do not skip index numbers
	//	[input]
	//		0=a
	//		1=b
	//		2=1
	//
	for (int i = 0;; i++) {
		WCHAR IndexStr[MAX_SYMBOL_SIZE];
		WCHAR Symbol[MAX_SYMBOL_SIZE];
		swprintf_s(IndexStr, MAX_SYMBOL_SIZE, L"%d",i);
		GetPrivateProfileString(L"input", IndexStr,L"", Symbol, MAX_SYMBOL_SIZE, (LPCTSTR)Filename);
		if (wcslen(Symbol) == 0) {
			if (i == 0) {
				// no memory allocations to delete []
				return APPERR_PARAMETER;
			}
			tmpNumInputs = i;
			break;
		}
	}
	if (tmpNumInputs == 0) {
		(*WarningFlags) = ERROR_FSM_NO_INPUTS;
	}

	// Determine the size of the Output Alphabet
	//	The Output alphabet in under the INI section [output]
	//	entries are of the form of (0 to n-1), n is the number of symbols
	//	example for 3 symbols, symbols can be any string (please do not use comma in symbol)
	//  The index number must start with 0 and end with n-1. Do not skip index numbers
	//	[output]
	//		0=N
	//		1=A
	//		2=?
	//
	for (int i = 0;; i++) {
		WCHAR IndexStr[MAX_SYMBOL_SIZE];
		WCHAR Symbol[MAX_SYMBOL_SIZE];
		swprintf_s(IndexStr, MAX_SYMBOL_SIZE, L"%d", i);
		GetPrivateProfileString(L"output", IndexStr, L"", Symbol, MAX_SYMBOL_SIZE, (LPCTSTR)Filename);
		if (wcslen(Symbol) == 0) {
			if (i == 0) {
				// no memory allocations to delete []
				return APPERR_PARAMETER;
			}
			tmpNumOutputs = i;
			break;
		}
	}
	if (tmpNumOutputs == 0) {
		(*WarningFlags) |= ERROR_FSM_NO_OUTPUTS;
	}

	// Determine the number of the states
	//	The state labels are under the INI section [states]
	//	entries are of the form of (0 to n-1), n is the number of states
	//	example for 3 states, labels can be any string
	//  The index number must start with 0 and end with n-1. Do not skip index numbers
	//	[output]
	//		0=unknown
	//		1=number
	//		2=letter
	//
	for (int i = 0;; i++) {
		WCHAR IndexStr[MAX_SYMBOL_SIZE];
		WCHAR Label[MAX_SYMBOL_SIZE];
		swprintf_s(IndexStr, MAX_SYMBOL_SIZE, L"%d", i);
		GetPrivateProfileString(L"states", IndexStr, L"", Label, MAX_SYMBOL_SIZE, (LPCTSTR)Filename);
		if (wcslen(Label) == 0) {
			if (i == 0) {
				// no memory allocations to delete []
				return APPERR_PARAMETER;
			}
			tmpNumStates = i;
			break;
		}
	}
	if (tmpNumStates == 0) {
		(*WarningFlags) |= ERROR_FSM_NO_STATES;
	}

	if ((*WarningFlags) != 0) {
		// can't have FSM without input, output or states
		return APPERR_PARAMETER;
	}

	// allocate next state rules
	// use tmp array, when all good then delete old FSM and set to new FSM

	int* tmpNextStateRules = new int[tmpNumInputs * tmpNumStates];
	if (tmpNextStateRules == nullptr) {
		return APPERR_MEMALLOC;
	}
	for (int i = 0; i < tmpNumInputs * tmpNumStates; i++) {
		tmpNextStateRules[i] = -1;
	}

	// Read Next State rules
	for (int i = 0, index=0; i < tmpNumStates; i++) {
		for (int j = 0; j < tmpNumInputs; j++, index++) {
			WCHAR IndexStr[MAX_SYMBOL_SIZE];
			swprintf_s(IndexStr, MAX_SYMBOL_SIZE, L"%d.%d", i, j);
			tmpNextStateRules[index] = GetPrivateProfileInt(L"next_state_rules", IndexStr, -1, (LPCTSTR)Filename);
		}
	}

	// Verify Next State rules are complete (NumInputs*NumStates), no missing rules
	for (int i = 0, index = 0; i < tmpNumStates; i++) {
		for (int j = 0; j < tmpNumInputs; j++, index++) {
			if (tmpNextStateRules[index] == -1) {
				(*WarningFlags) = (*WarningFlags) | WARNING_FSM_NEXT_STATES_RULES; // invalid state
			}
			if (tmpNextStateRules[index] >= tmpNumStates) {
				(*WarningFlags) = (*WarningFlags) | WARNING_FSM_NEXT_STATES_RULES; // invalid state
			}
		}
	}

	// allocate next output rules
	// use tmp array, when all good delete old FSM and copy new one
	int* tmpOutputRules = new int[tmpNumInputs * tmpNumStates];
	if (tmpOutputRules == nullptr) {
		delete [] tmpNextStateRules;
		return APPERR_MEMALLOC;
	}
	for (int i = 0l; i < tmpNumInputs * tmpNumStates; i++) {
		tmpOutputRules[i] = -1;
	}

	// read output rules
	for (int i = 0, index = 0; i < tmpNumStates; i++) {
		for (int j = 0; j < tmpNumInputs; j++, index++) {
			WCHAR IndexStr[MAX_SYMBOL_SIZE];
			swprintf_s(IndexStr, MAX_SYMBOL_SIZE, L"%d.%d", i, j);
			tmpOutputRules[index] = GetPrivateProfileInt(L"output_rules", IndexStr, -1, (LPCTSTR)Filename);
		}
	}

	// Verify Output rules are complete (NumInputs*NumStates), no missing rules
	for (int i = 0, index = 0; i < tmpNumStates; i++) {
		for (int j = 0; j < tmpNumInputs; j++, index++) {
			if (tmpOutputRules[index] == -1) {
				(*WarningFlags) = (*WarningFlags) | WARNING_FSM_OUTPUT_RULES;
			}
			if (tmpOutputRules[index] >= tmpNumOutputs) {
				(*WarningFlags) = (*WarningFlags) | WARNING_FSM_OUTPUT_RULES;
			}
		}
	}

	// Verify all states are reachable
	int* tmpCheckStates = new int[tmpNumStates];
	if (tmpCheckStates == nullptr) {
		delete[] tmpOutputRules;
		delete[] tmpNextStateRules;
		return APPERR_MEMALLOC;
	}

	for (int i = 0; i < tmpNumStates; i++) {
		tmpCheckStates[i] = 0;
	}
	for (int i = 0; i < tmpNumInputs * tmpNumStates; i++) {
		if ((tmpNextStateRules[i] == -1) || (tmpNextStateRules[i] >= tmpNumStates)) {
			continue; // invalid state
		}
		tmpCheckStates[tmpNextStateRules[i]]++;
	}
	for (int i = 0; i < tmpNumStates; i++) {
		if (tmpCheckStates[i] == 0) {
			(*WarningFlags) = (*WarningFlags) | WARNING_FSM_STATES_UNREACHABLE;
		}
	}
	
	// Verify all Outputs are used
	int* tmpCheckOutputs = new int[tmpNumOutputs];
	if (tmpCheckOutputs == nullptr) {
		delete[] tmpOutputRules;
		delete[] tmpNextStateRules;
		delete[] tmpCheckStates;
		return APPERR_MEMALLOC;
	}
	for (int i = 0; i < tmpNumOutputs; i++) {
		tmpCheckOutputs[i] = 0;
	}
	for (int i = 0; i < tmpNumInputs * tmpNumStates; i++) {
		if ((tmpOutputRules[i] == -1) || (tmpOutputRules[i] >= tmpNumOutputs)) {
			continue;
		}
		tmpCheckOutputs[tmpOutputRules[i]]++;
	}
	for (int i = 0; i < tmpNumOutputs; i++) {
		if (tmpCheckOutputs[i] == 0) {
			(*WarningFlags) = (*WarningFlags) | WARNING_FSM_OUTPUTS_UNUSED;
		}
	}

	// Read in Input Alphabet
	WCHAR** tmpInputSymbols = new WCHAR * [tmpNumInputs];
	if (tmpInputSymbols == nullptr) {
		delete[] tmpCheckOutputs;
		delete[] tmpCheckStates;
		delete[] tmpOutputRules;
		delete[] tmpNextStateRules;
		return APPERR_MEMALLOC;
	}
	for (int i = 0; i < tmpNumInputs; i++) {
		WCHAR InputIndexStr[MAX_SYMBOL_SIZE];
		WCHAR* Symbol = new WCHAR[MAX_SYMBOL_SIZE];
		if (Symbol == nullptr) {
			for (int j = 0; j < i; j++) {
				delete[] tmpInputSymbols[j];
			}
			delete[] tmpInputSymbols;
			delete[] tmpCheckOutputs;
			delete[] tmpCheckStates;
			delete[] tmpOutputRules;
			delete[] tmpNextStateRules;
			return APPERR_MEMALLOC;
		}
		swprintf_s(InputIndexStr, MAX_SYMBOL_SIZE, L"%d", i);
		GetPrivateProfileString(L"input", InputIndexStr, L"", Symbol, MAX_SYMBOL_SIZE, (LPCTSTR)Filename);
		tmpInputSymbols[i] = Symbol;
	}

	// Read in Output Alphabet
	WCHAR** tmpOutputSymbols = new WCHAR * [tmpNumOutputs];
	if (tmpOutputSymbols == nullptr) {
		for (int j = 0; j < tmpNumInputs; j++) {
			delete[] tmpInputSymbols[j];
		}
		delete[] tmpInputSymbols;
		delete[] tmpCheckOutputs;
		delete[] tmpCheckStates;
		delete[] tmpOutputRules;
		delete[] tmpNextStateRules;
		return APPERR_MEMALLOC;
	}

	for (int i = 0; i < tmpNumOutputs; i++) {
		WCHAR IndexStr[MAX_SYMBOL_SIZE];
		WCHAR* Symbol = new WCHAR[MAX_SYMBOL_SIZE];
		if (Symbol == nullptr) {
			for (int j = 0; j < i; j++) {
				delete[] tmpOutputSymbols[j];
			}
			for (int j = 0; j < tmpNumInputs; j++) {
				delete[] tmpInputSymbols[j];
			}
			delete[] tmpOutputSymbols;
			delete[] tmpInputSymbols;
			delete[] tmpCheckOutputs;
			delete[] tmpCheckStates;
			delete[] tmpOutputRules;
			delete[] tmpNextStateRules;
			return APPERR_MEMALLOC;
		}
		swprintf_s(IndexStr, MAX_SYMBOL_SIZE, L"%d", i);
		GetPrivateProfileString(L"output", IndexStr, L"", Symbol, MAX_SYMBOL_SIZE, (LPCTSTR)Filename);
		tmpOutputSymbols[i] = Symbol;
	}

	// Read in State labels
	WCHAR** tmpStateLabels = new WCHAR * [tmpNumStates];
	if (tmpStateLabels == nullptr) {
		for (int j = 0; j < tmpNumOutputs; j++) {
			delete[] tmpOutputSymbols[j];
		}
		for (int j = 0; j < tmpNumInputs; j++) {
			delete[] tmpInputSymbols[j];
		}
		delete[] tmpOutputSymbols;
		delete[] tmpInputSymbols;
		delete[] tmpCheckOutputs;
		delete[] tmpCheckStates;
		delete[] tmpOutputRules;
		delete[] tmpNextStateRules;
		return APPERR_MEMALLOC;
	}
	for (int i = 0; i < tmpNumStates; i++) {
		WCHAR IndexStr[MAX_SYMBOL_SIZE];
		WCHAR* Symbol = new WCHAR[MAX_SYMBOL_SIZE];
		if (Symbol == nullptr) {
			for (int j = 0; j < i; j++) {
				delete[] tmpStateLabels[j];
			}
			for (int j = 0; j < tmpNumOutputs; j++) {
				delete[] tmpOutputSymbols[j];
			}
			for (int j = 0; j < tmpNumInputs; j++) {
				delete[] tmpInputSymbols[j];
			}
			delete[] tmpStateLabels;
			delete[] tmpOutputSymbols;
			delete[] tmpInputSymbols;
			delete[] tmpCheckOutputs;
			delete[] tmpCheckStates;
			delete[] tmpOutputRules;
			delete[] tmpNextStateRules;
			return APPERR_MEMALLOC;
		}
		swprintf_s(IndexStr, MAX_SYMBOL_SIZE, L"%d", i);
		GetPrivateProfileString(L"states", IndexStr, L"", Symbol, MAX_SYMBOL_SIZE, (LPCTSTR)Filename);
		tmpStateLabels[i] = Symbol;
	}

	// if old FSM loaded delete it
	DeleteFSM();

	// set to new FSM
	NumInputs = tmpNumInputs;
	InputSymbols = tmpInputSymbols;

	NumOutputs = tmpNumOutputs;
	OutputSymbols = tmpOutputSymbols;

	NumStates = tmpNumStates;
	StateLabels = tmpStateLabels;

	CheckStates = tmpCheckStates;

	CheckOutputs = tmpCheckOutputs;

	NextStateRules = tmpNextStateRules;

	OutputRules = tmpOutputRules;

	CurrentState = tmpStartState;

	CurrentOutput = -1;  // output can not be dfined until there is an input

	wcscpy_s(ConfigurationFile,MAX_PATH,Filename);
	
	return APP_SUCCESS;
}

//*******************************************************************************
//
//  ResetFSM()
//  
//	Resets current state to 0, output to <invalid>
// 
//  parmameters
// 
// return
//		see AppErrors.h file for standard erro list
// 
//*******************************************************************************
int GenericFSM::ResetFSM()
{
	CurrentState = 0;
	CurrentOutput = -1;
	return APP_SUCCESS;
}

//*******************************************************************************
//
//  StepFSM(int index)
//  
//	sets current state according to Next state rule
// 
//  parmameters
//
//	int index		index of input symbol
//
//	return
//		APP_SCUCCESS		FSM state advanced
//		APPERR_PARAMETER	index is outside the range of states
// 
//*******************************************************************************
int GenericFSM::StepFSM(int index) // sets current state according to Next state rule
{
	if (index < 0 || index >= NumInputs) {
		// input is not found in input alphabet
		return APPERR_PARAMETER;
	}

	// lookup current state, input index to set the Current Output
	int NextStateIndex = (index + CurrentState * NumInputs);
	CurrentOutput = OutputRules[NextStateIndex];
	// Set Current state to new state
	CurrentState = NextStateRules[NextStateIndex];

	return APPERR_NYI;
}

//*******************************************************************************
//
//  StepFSM(WCHAR* Token)
//  
//	sets current state according to Next state rule
// 
//  parmameters
//
//	WCHAR* Token		input string should match alphabet input
// 
//	return
//		APP_SCUCCESS		FSM state advanced
//		APPERR_PARAMETER	Symbol not found in input alphabet
// 
//*******************************************************************************
int GenericFSM::StepFSM(WCHAR* Token) // sets current state according to Next state rule
{
	int index;

	//Convert Token to index number of input alphabet
	for (index = 0; index < NumInputs; index++) {
		if (wcscmp(Token, InputSymbols[index]) == 0) {
			break;
		}
	}

	if (index >= NumInputs) {
		// input is not found in input alphabet
		return APPERR_PARAMETER;
	}

	// lookup current state, input index to set the Current Output
	int NextStateIndex = (index + CurrentState * NumInputs);
	CurrentOutput = OutputRules[NextStateIndex];
	// Set Current state to new state
	CurrentState = NextStateRules[NextStateIndex];

	return APP_SUCCESS;
}

//*******************************************************************************
//
//  GetCurrentOutput(WCHAR* Output, int MaxLength)
//  
//	returns current output symbol
// 
//  parmameters
//		WCHAR* Output		Output symbol is return in this string
//		size_t MaxLength	Maximum string lengh of 'Output'
// 
// return
//		see AppErrors.h file for standard erro list
// 
//*******************************************************************************
int GenericFSM::GetCurrentOutput(WCHAR* Output, int MaxLength) // returns current output symbol
{
	if (CurrentOutput < 0) {
		wcscpy_s(Output, MaxLength, L"<invalid>");
		return APPERR_PARAMETER;
	}

	wcscpy_s(Output, MaxLength, OutputSymbols[CurrentOutput]);
	return APP_SUCCESS;


}

//*******************************************************************************
//
//  GetCurrentStateIndex()
//  
//	returns current output symbol
// 
//  parmameters
//		None
// return
//		Current State index
// 
//*******************************************************************************
int GenericFSM::GetCurrentStateIndex()
{
	return CurrentState;
}

//*******************************************************************************
//
//  GetNumInputAlphabet()
// 
//  parmameters
//		none
// return
//		number of symbols in the input alphabet
// 
//*******************************************************************************
int GenericFSM::GetNumInputAlphabet()
{
	return NumInputs;
}

//*******************************************************************************
//
//  GetNumOutputAlpabet()
// 
//  parmameters
//		none
// return
//		number of symbols in the output alphabet
// 
//*******************************************************************************
int GenericFSM::GetNumOutputAlpabet()
{
	return NumOutputs;
}

//*******************************************************************************
//
//  GetNumSates()
// 
//  parmameters
//		none
// return
//		number of symbols in the output alphabet
// 
//*******************************************************************************
int GenericFSM::GetNumStates()
{
	return NumStates;
}

//*******************************************************************************
//
//  GetStateLabel(int index, WCHAR* label, size_t MaxLength)
// 
//	Returns label for a State
// 
//  parmameters
//		int				state index
//		
//		WCHAR* label	state label(string) is return in this string
//		size_t MaxLength Maximum string lengh of 'label'
//
// return
//		
//		see AppErrors.h file for standard erro list
// 
//*******************************************************************************
int GenericFSM::GetStateLabel(int index, WCHAR* label, size_t MaxLength)
{
	if (index < 0 || index >= NumStates) {
		return APPERR_PARAMETER;
	}
	wcscpy_s(label, MaxLength, StateLabels[index]);

	return APP_SUCCESS;
}

//*******************************************************************************
//
//  GetInputAlphabetLabel(int index, WCHAR* label, size_t MaxLength)
// 
//	Returns the input symbol for index
// 
//  parmameters
//		int				index of the input alphabet symbol to be returned
//		
//		WCHAR* label	input symbol is return in this string
//		size_t MaxLength Maximum string lengh of 'label'
// 
//  return
//		
//		see AppErrors.h file for standard error list
// 
//*******************************************************************************
int GenericFSM::GetInputAlphabetLabel(int index, WCHAR* label, size_t MaxLength)
{
	if (index < 0 || index >= NumInputs) {
		return APPERR_PARAMETER;
	}
	wcscpy_s(label, MaxLength, InputSymbols[index]);

	return APP_SUCCESS;
}

//*******************************************************************************
//
//  GetOutputAlphabetLabel(int index, WCHAR* label, size_t MaxLength)
// 
//	Returns the Output symbol for index
// 
//  parmameters
//		int				index of the output alphabet symbol to be returned
//		
//		WCHAR* label	Output symbol is return in this string
//		size_t MaxLength Maximum string lengh of 'label'
// 
//  return
//		
//		see AppErrors.h file for standard error list
// 
//*******************************************************************************
int GenericFSM::GetOutputAlphabetLabel(int index, WCHAR* label, size_t MaxLength)
{
	if (index < 0 || index >= NumOutputs) {
		return APPERR_PARAMETER;
	}
	wcscpy_s(label, MaxLength, OutputSymbols[index]);

	return APP_SUCCESS;
}

//*******************************************************************************
//
//  GetNextStateRule(int StateIndex, int InputIndex)
// 
//	Returns the next state for State, Input
// 
//  parmameters
//		int StateIndex
//		int InputIndex	
//		
//  return
//		int		Next Stat	
// 
//*******************************************************************************
int GenericFSM::GetNextStateRule(int StateIndex, int InputIndex)
{
	if (StateIndex < 0 || StateIndex >= NumStates) {
		return -1; // Out of range parameter
	}
	if (InputIndex < 0 || InputIndex >= NumInputs) {
		return -1; // Out of range parameter
	}
	int NextState = NextStateRules[InputIndex + (StateIndex * NumInputs)];
	return NextState;
}

//*******************************************************************************
//
//  GetOuputRule(int StateIndex, int InputIndex)
// 
//  parmameters
//		int StateIndex
//		int InputIndex	
// 
//  return
//		int			index of the output symbol for this State,Input
// 
//*******************************************************************************
int GenericFSM::GetOuputRule(int StateIndex, int InputIndex)
{
	if (StateIndex < 0 || StateIndex >= NumStates) {
		return -1; // Out of range parameter
	}
	if (InputIndex < 0 || InputIndex >= NumInputs) {
		return -1; // Out of range parameter
	}
	int Output = OutputRules[InputIndex + (StateIndex*NumInputs)];
	return Output;
}

//*******************************************************************************
//
//  IsStateUsed(int StateIndex)
// 
//	Returns the Output symbol for index
// 
//  parmameters
//		
//  return
//		TRUE	This state is reachable in the FSM
//		FALSE	This state is not reachable in the FSM
// 
//*******************************************************************************
BOOL GenericFSM::IsStateUsed(int StateIndex)
{
	if (StateIndex < 0 || StateIndex >= NumStates) {
		return FALSE;
	}
	if (CheckStates[StateIndex] == 0) {
		return FALSE;
	}
	return TRUE;
}

//*******************************************************************************
//
//  IsOutputUsed(int OutputIndex)
// 
//  parmameters
//		int OutputIndex		Index number of an output alphabet
// 
//  return
//		TRUE	This output symbol is used in the FSM
//		FALSE	This output symbol is NOT used in the FSM
// 
//*******************************************************************************
BOOL GenericFSM::IsOutputUsed(int OutputIndex)
{
	if (OutputIndex < 0 || OutputIndex >= NumOutputs) {
		return FALSE;
	}
	if (CheckOutputs[OutputIndex] == 0) {
		return FALSE;
	}
	return TRUE;
}

//*******************************************************************************
//
//  GetDelimiters(WCHAR* Delimiter, int MaxLength)
// 
//  parmameters
// 
//	int MaxLength		maximum length of Delimiter string
// 
//	WCHAR* Delimiter	The list of delimiters is copied as a string
//						to this paremeter
//		
//  return
//		
//		see AppErrors.h file for standard error list
// 
//*******************************************************************************
int GenericFSM::GetDelimiters(WCHAR* Delimiter, int MaxLength)
{
	if (wcslen(Delimiters) == 0) {
		return APPERR_PARAMETER;
	}

	wcscpy_s(Delimiter, MaxLength, Delimiters);
	return APP_SUCCESS;
}

//*******************************************************************************
//
//  ReadDelimiters(WCHAR* INIfilename)
// 
//	Read the delimiters for use with the FSM to separate
//	the input alphabet symbols in an input sequence
// 
//	If nothing is defined in the INI file
//		then the delimiters are <space><tab>,-
//
//	example:
//	[delimiters]
//	0=<sapce>
//	1=<tab>
//	2=,
//	3=-
// 
//  parmameters
//	WCHAR* INIfilename		INI file for FSM
//		
//  return
//		APP_SUCCESS
// 
//		see AppErrors.h file for standard error list
// 
//*******************************************************************************
int GenericFSM::ReadDelimiters(WCHAR* INIfilename)
{
	WCHAR Symbol[MAX_SYMBOL_SIZE];
	WCHAR Results[MAX_SYMBOL_SIZE];
	WCHAR IndexStr[MAX_SYMBOL_SIZE];

	wcscpy_s(Results, MAX_SYMBOL_SIZE, L"");
	for (int i = 0; i < MAX_SYMBOL_SIZE; i++) {
		swprintf_s(IndexStr, MAX_SYMBOL_SIZE, L"%d", i);
		GetPrivateProfileString(L"Delimiters", IndexStr, L"", Symbol, MAX_SYMBOL_SIZE, (LPCTSTR)INIfilename);
		if (wcslen(Symbol) == 0) {
			if (i != 0) {
				break;
			}
			// default delimiters if none specified in the FSM
			wcscpy_s(Results, MAX_SYMBOL_SIZE, L" \t,-");
			return APP_SUCCESS;
		}

		if (wcscmp(Symbol, L"<space>") == 0) {
			wcscat_s(Results, MAX_SYMBOL_SIZE, L" ");
		}
		else if (wcscmp(Symbol, L"<tab>") == 0) {
			wcscat_s(Results, MAX_SYMBOL_SIZE, L"\t");
		}
		else {
			wcscat_s(Results, MAX_SYMBOL_SIZE, Symbol);
		}
	}

	wcscpy_s(Delimiters, MAX_SYMBOL_SIZE, Results);
	return APP_SUCCESS;
}

//*******************************************************************************
//
//  DeleteFSM()
// 
//	Delete any loaded FSM
// 
//  parmameters
// 
//		none
//		
//  return
// 
//		none
//		
//		see AppErrors.h file for standard error list
// 
//*******************************************************************************
void GenericFSM::DeleteFSM()
{
	if (InputSymbols != nullptr) {
		for (int i = 0; i < NumInputs; i++) {
			if (InputSymbols[i] != nullptr) {
				delete[] InputSymbols[i];
				InputSymbols[i] = nullptr;
			}
		}
		delete[] InputSymbols;
		InputSymbols = nullptr;
	}

	if (OutputSymbols != nullptr) {
		for (int i = 0; i < NumOutputs; i++) {
			if (OutputSymbols[i] != nullptr) {
				delete[] OutputSymbols[i];
				OutputSymbols[i] = nullptr;
			}
		}
		delete[] OutputSymbols;
		OutputSymbols = nullptr;
	}

	if (StateLabels != nullptr) {
		for (int i = 0; i < NumStates; i++) {
			if (StateLabels[i] != nullptr) {
				delete[] StateLabels[i];
				StateLabels[i] = nullptr;
			}
		}
		delete[] StateLabels;
		StateLabels = nullptr;
	}

	NumInputs = 0;
	NumOutputs = 0;
	NumStates = 0;

	if (CheckStates != nullptr) {
		delete[] CheckStates;
		CheckStates = nullptr;
	}

	if (CheckOutputs != nullptr) {
		delete[] CheckOutputs;
		CheckOutputs = nullptr;
	}

	if (NextStateRules != nullptr) {
		delete[] NextStateRules;
		NextStateRules = nullptr;
	}
	
	if (OutputRules != nullptr) {
		delete[] OutputRules;
		OutputRules = nullptr;
	}
	
	CurrentState = -1;
	CurrentOutput = -1;
	return;

}