#ifndef KEYBOARD_H
#define KEYBOARD_H


// Wrapper to buffer users from the internal glfw driver
// Converted to enumeration for type safety - yeah!

enum AZUL_KEY
{
	// Printable keys
	KEY_SPACE = 32,
	KEY_APOSTROPHE = 39,
	KEY_COMMA = 44,
	KEY_MINUS = 45,
	KEY_PERIOD = 46,
	KEY_SLASH = 47,

	KEY_0 = 48,
	KEY_1 = 49,
	KEY_2 = 50,
	KEY_3 = 51,
	KEY_4 = 52,
	KEY_5 = 53,
	KEY_6 = 54,
	KEY_7 = 55,
	KEY_8 = 56,
	KEY_9 = 57,

	KEY_SEMICOLON = 59,
	KEY_EQUAL = 61,

	KEY_A = 65,
	KEY_B = 66,
	KEY_C = 67,
	KEY_D = 68,
	KEY_E = 69,
	KEY_F = 70,
	KEY_G = 71,
	KEY_H = 72,
	KEY_I = 73,
	KEY_J = 74,
	KEY_K = 75,
	KEY_L = 76,
	KEY_M = 77,
	KEY_N = 78,
	KEY_O = 79,
	KEY_P = 80,
	KEY_Q = 81,
	KEY_R = 82,
	KEY_S = 83,
	KEY_T = 84,
	KEY_U = 85,
	KEY_V = 86,
	KEY_W = 87,
	KEY_X = 88,
	KEY_Y = 89,
	KEY_Z = 90,

	KEY_LEFT_BRACKET = 91,
	KEY_BACKSLASH = 92,
	KEY_RIGHT_BRACKET = 93,
	KEY_GRAVE_ACCENT = 96,
	KEY_WORLD_1 = 161,
	KEY_WORLD_2 = 162,

	KEY_ARROW_UP = (256 + 27),
	KEY_ARROW_DOWN = (256 + 28),
	KEY_ARROW_LEFT = (256 + 29),
	KEY_ARROW_RIGHT = (256 + 30),

	KEY_ESCAPE = (256 + 1),
	KEY_ENTER = (256 + 38),
	KEY_TAB = (256 + 37),
	KEY_BACKSPACE = (256 + 39),
	KEY_INSERT = (256 + 40),

	KEY_DELETE = (256 + 41),
	KEY_PAGE_UP = (256 + 42),
	KEY_PAGE_DOWN = (256 + 43),
	KEY_HOME = (256 + 44),
	KEY_END = (256 + 45),

	KEY_CAPS_LOCK = (256 + 64),
	KEY_SCROLL_LOCK = (256 + 65),
	KEY_NUM_LOCK = (256 + 63),


	KEY_PAUSE = (256 + 66),
	KEY_F1 = (256 + 2),
	KEY_F2 = (256 + 3),
	KEY_F3 = (256 + 4),
	KEY_F4 = (256 + 5),
	KEY_F5 = (256 + 6),
	KEY_F6 = (256 + 7),
	KEY_F7 = (256 + 8),
	KEY_F8 = (256 + 9),
	KEY_F9 = (256 + 10),
	KEY_F10 = (256 + 11),
	KEY_F11 = (256 + 12),
	KEY_F12 = (256 + 13),
	KEY_F13 = (256 + 14),
	KEY_F14 = (256 + 15),
	KEY_F15 = (256 + 16),
	KEY_F16 = (256 + 17),
	KEY_F17 = (256 + 18),
	KEY_F18 = (256 + 19),
	KEY_F19 = (256 + 20),
	KEY_F20 = (256 + 21),
	KEY_F21 = (256 + 22),
	KEY_F22 = (256 + 23),
	KEY_F23 = (256 + 24),
	KEY_F24 = (256 + 25),
	KEY_F25 = (256 + 26),

	KEY_KP_0 = (256 + 46),
	KEY_KP_1 = (256 + 47),
	KEY_KP_2 = (256 + 48),
	KEY_KP_3 = (256 + 49),
	KEY_KP_4 = (256 + 50),
	KEY_KP_5 = (256 + 51),
	KEY_KP_6 = (256 + 52),
	KEY_KP_7 = (256 + 53),
	KEY_KP_8 = (256 + 54),
	KEY_KP_9 = (256 + 55),
	KEY_KP_DECIMAL = (256 + 60),
	KEY_KP_DIVIDE = (256 + 56),
	KEY_KP_MULTIPLY = (256 + 57),
	KEY_KP_SUBTRACT = (256 + 58),
	KEY_KP_ADD = (256 + 59),
	KEY_KP_ENTER = (256 + 62),
	KEY_KP_EQUAL = (256 + 61),

	KEY_LEFT_SHIFT = (256 + 31),
	KEY_LEFT_CONTROL = (256 + 33),
	KEY_LEFT_ALT = (256 + 35),
	KEY_LEFT_SUPER = (256 + 67),

	KEY_RIGHT_SHIFT = (256 + 32),
	KEY_RIGHT_CONTROL = (256 + 34),
	KEY_RIGHT_ALT = (256 + 36),
	KEY_RIGHT_SUPER = (256 + 68),
	KEY_MENU = (256 + 69),

};


class Keyboard
{
public:	
	Keyboard();

	// Use this to read keyboard
	bool GetKeyState( AZUL_KEY key);

private:
	
	
};


#endif