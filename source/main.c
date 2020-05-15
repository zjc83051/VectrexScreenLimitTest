/*
 * This application is intended to determine two properties of a Vetrex:
 * a) the scale factor required to reach the vertical extents of the unit's display from
 *    its center using the maximum Y strength magnitude (labeled SCALE); and
 * b) the X strength magnitude using that scale factor required to reach the horizontal
 *    extents of the unit's display from its center (labeled WIDTH).
 * As the Vectrex has an aspect ratio of ~0.8, one would expect WIDTH to be approximately
 * equal to 102.
 *
 * The Y axis of the player 1 analog stick adjusts SCALE while the X axis adjusts WIDTH.
 * Quickest results are achieved by tilting the stick up until the box displayed is as
 * tall as the screen and then tilting the stick right until the box is as wide as the
 * screen.
 *
 * The application is not very efficient, but it doesn't need to be!
*/

#include <vectrex.h>

typedef unsigned int UINT8;    // Unsigned 8-bit integer.
typedef signed int SINT8;      // Signed 8-bit integer.
typedef unsigned long UINT16;  // Unsigned 16-bit integer.
typedef signed long SINT16;    // Signed 16-bit integer.

// Converts the 8-bit parameter to a 16-bit number with the parameter as the high byte and 0 as the low byte.
#define toHighOrderByte(x)((((UINT16)(x)) << 1 << 1 << 1 << 1 << 1 << 1 << 1 << 1))

// Retrieves the high order byte of the 16-bit parameter.
#define getHighOrderByte(x)((UINT8)((x) >> 1 >> 1 >> 1 >> 1 >> 1 >> 1 >> 1 >> 1))

// Returns 'value' clamped to the range of (min, max).
static inline UINT16 clampToRange(const UINT16 value, const UINT16 min, const UINT16 max) {
	if (value < min) {
		return min;
	} else if (value > max) {
		return max;
	} else {
		return value;
	}
}

// Adjusts the specified variable (scale or width) as appropriate for a single iteration of the
// main game loop given the specified value of the joystick axis used to manipulate said variable.
// The value will be clamped between the specified minimum and maximum values.
void adjustVariable(
	UINT16*const variablePtr,
	SINT8 joystickAxisValue,
	const UINT16 minValue,
	const UINT16 maxValue
) {
	// While we could add the axis value directly to the variable, this makes it a little slow to
	// change, so we double the axis value first (after adjusting to start at 0 from the dead zone end).

	// The dead zone is fairly large to ensure users can easily manipulate one axis at a time.
	static const SINT8 JOY_DEAD_ZONE = 30;

	if (joystickAxisValue > JOY_DEAD_ZONE) {
		(*variablePtr) += (( ((UINT16)joystickAxisValue) - JOY_DEAD_ZONE) << 1);
	} else if (joystickAxisValue < -JOY_DEAD_ZONE) {
		(*variablePtr) -= (( (joystickAxisValue == -128 ? 127 : (UINT16)-joystickAxisValue) - JOY_DEAD_ZONE) << 1);
	}
	*variablePtr = clampToRange(*variablePtr, minValue, maxValue);
}

// Converts 'num' to a 3-digit (padded with leading 0s), properly-terminated
// string stored in str[].  Assumes str[] is at least size 4.
void numToString(UINT8 str[], UINT8 num) {
	static const UINT8 numDigits = 3;

	for (SINT8 strPos = (numDigits - 1); (strPos >= 0); strPos--) {
		const UINT8 currentDigit = (num % 10);
		num /= 10;
		str[strPos] = (currentDigit + '0');
	}
	str[numDigits] = 0x80;  // terminate string
}

extern int main(void) {

	static const UINT8 DEFAULT_WIDTH = 80;
	static const UINT16 MIN_SCALE_16 = toHighOrderByte(10);
	static const UINT16 MAX_SCALE_16 = toHighOrderByte(200);
	static const UINT16 MIN_WIDTH_16 = toHighOrderByte(10);
	static const UINT16 MAX_WIDTH_16 = toHighOrderByte(127);

	Vec_Joy_Resltn = 0;   // set to a power of 2, with 128 = least accurate, 0 = most accurate

	// enable player 1 stick x/y
	Vec_Joy_Mux_1_X = 1;
	Vec_Joy_Mux_1_Y = 3;

	// disable player 2 stick
	Vec_Joy_Mux_2_X = 0;
	Vec_Joy_Mux_2_Y = 0;

	// Scale and width are represented as 16-bit, Q8.8 numbers to allow greater joystick precision
	// in controlling the numbers.  The fractional parts of the numbers are truncated for use in
	// drawing routines and for display.
	UINT16 scale16 = toHighOrderByte(100);
	UINT16 width16 = toHighOrderByte(DEFAULT_WIDTH);
	UINT8 scaleString[4];
	UINT8 widthString[4];

	// The box visually indicating the screen limits.
	SINT8 vectorList[] = {
		8, // vector count
		0, -(SINT8)DEFAULT_WIDTH,
		-127, 0,
		0, (SINT8)DEFAULT_WIDTH,
		0, (SINT8)DEFAULT_WIDTH,
		127, 0,
		127, 0,
		0, -(SINT8)DEFAULT_WIDTH,
		0, -(SINT8)DEFAULT_WIDTH,
		-127, 0
	};

	while (1) {
		Wait_Recal();
		Joy_Analog();

		// Adjust the variables based on stick position.
		adjustVariable(&scale16, Vec_Joy_1_Y, MIN_SCALE_16, MAX_SCALE_16);
		adjustVariable(&width16, Vec_Joy_1_X, MIN_WIDTH_16, MAX_WIDTH_16);

		// Truncate them for drawing/display.
		const UINT8 scale = getHighOrderByte(scale16);
		const UINT8 width = getHighOrderByte(width16);

		// Update the width in the box vector list.
		vectorList[2] = -(SINT8)width;
		vectorList[6] = (SINT8)width;
		vectorList[8] = (SINT8)width;
		vectorList[14] = -(SINT8)width;
		vectorList[16] = -(SINT8)width;

		// Draw the box.
		Reset0Ref();
		Intensity_5F();
		VIA_t1_cnt_lo = scale;
		Moveto_d(0, 0);
		Mov_Draw_VLc_a(vectorList);
		Reset0Ref();

		// Convert the variables to strings and display them.
		numToString(scaleString, scale);
		numToString(widthString, width);

		Print_Str_d(14, -63, "SCALE:\x80:");
		Print_Str_d(14, 20, scaleString);

		Print_Str_d(-6, -63, "WIDTH:\x80:");
		Print_Str_d(-6, 20, widthString);		
	};

	return 0;
}