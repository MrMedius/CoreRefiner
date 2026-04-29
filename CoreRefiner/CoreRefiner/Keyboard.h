#pragma once
#include <queue>
#include <bitset>
#include <optional>

class Keyboard
{
    friend class InputCodex;
public:
	class Event
	{
	public:
		enum class Type
		{
			Press,
			Release,
			Invalid
		};
	private:
		Type type;
		unsigned char code;
	public:
		Event()
			:
			type(Type::Invalid),
			code(0u)
		{}
		Event( Type type,unsigned char code ) noexcept
			:
			type( type ),
			code( code )
		{}
		bool IsPress() const noexcept
		{
			return type == Type::Press;
		}
		bool IsRelease() const noexcept
		{
			return type == Type::Release;
		}
		bool IsValid() const noexcept
		{
			return type != Type::Invalid;
		}
		unsigned char GetCode() const noexcept
		{
			return code;
		}
	};
public:
	Keyboard() = default;
	Keyboard( const Keyboard& ) = delete;
	Keyboard& operator=( const Keyboard& ) = delete;
	void Update(void);
	bool KeyIsPressed( unsigned char keycode ) const noexcept;
	bool KeyIsTriggered(unsigned char keycode) const noexcept;
	bool KeyIsReleased(unsigned char keycode) const noexcept;
	std::optional<Event> ReadKey() noexcept;
	bool KeyIsEmpty() const noexcept;
	void FlushKey() noexcept;
	std::optional<char> ReadChar() noexcept;
	bool CharIsEmpty() const noexcept;
	void FlushChar() noexcept;
	void Flush() noexcept;
	void EnableAutorepeat() noexcept;
	void DisableAutorepeat() noexcept;
	bool AutorepeatIsEnabled() const noexcept;
	bool AnyKeyPressed() const noexcept { return keystates.any(); }
private:
	void OnKeyPressed( unsigned char keycode ) noexcept;
	void OnKeyReleased( unsigned char keycode ) noexcept;
	void OnChar( char character ) noexcept;
	void ClearState() noexcept;
	template<typename T>
	static void TrimBuffer( std::queue<T>& buffer ) noexcept;
private:
	static constexpr unsigned int nKeys = 256u;
	static constexpr unsigned int bufferSize = 4u;
	bool autorepeatEnabled = false;
	std::bitset<nKeys> keystates;
	std::bitset<nKeys> keyOldstates;
	std::queue<Event> keybuffer;
	std::queue<char> charbuffer;
};

typedef enum Keyboard_Keys_tag
{
    KK_NONE = 0x0,

    KK_BACK = 0x8,
    KK_TAB = 0x9,

    KK_ENTER = 0xd,

    KK_PAUSE = 0x13,
    KK_CAPSLOCK = 0x14,
    KK_KANA = 0x15,

    KK_KANJI = 0x19,

    KK_ESCAPE = 0x1b,
    KK_IMECONVERT = 0x1c,
    KK_IMENOCONVERT = 0x1d,

    KK_SPACE = 0x20,
    KK_PAGEUP = 0x21,
    KK_PAGEDOWN = 0x22,
    KK_END = 0x23,
    KK_HOME = 0x24,
    KK_LEFT = 0x25,
    KK_UP = 0x26,
    KK_RIGHT = 0x27,
    KK_DOWN = 0x28,
    KK_SELECT = 0x29,
    KK_PRINT = 0x2a,
    KK_EXECUTE = 0x2b,
    KK_PRINTSCREEN = 0x2c,
    KK_INSERT = 0x2d,
    KK_DELETE = 0x2e,
    KK_HELP = 0x2f,
    KK_D0 = 0x30,
    KK_D1 = 0x31,
    KK_D2 = 0x32,
    KK_D3 = 0x33,
    KK_D4 = 0x34,
    KK_D5 = 0x35,
    KK_D6 = 0x36,
    KK_D7 = 0x37,
    KK_D8 = 0x38,
    KK_D9 = 0x39,

    KK_A = 0x41,
    KK_B = 0x42,
    KK_C = 0x43,
    KK_D = 0x44,
    KK_E = 0x45,
    KK_F = 0x46,
    KK_G = 0x47,
    KK_H = 0x48,
    KK_I = 0x49,
    KK_J = 0x4a,
    KK_K = 0x4b,
    KK_L = 0x4c,
    KK_M = 0x4d,
    KK_N = 0x4e,
    KK_O = 0x4f,
    KK_P = 0x50,
    KK_Q = 0x51,
    KK_R = 0x52,
    KK_S = 0x53,
    KK_T = 0x54,
    KK_U = 0x55,
    KK_V = 0x56,
    KK_W = 0x57,
    KK_X = 0x58,
    KK_Y = 0x59,
    KK_Z = 0x5a,
    KK_LEFTWINDOWS = 0x5b,
    KK_RIGHTWINDOWS = 0x5c,
    KK_APPS = 0x5d,

    KK_SLEEP = 0x5f,
    KK_NUMPAD0 = 0x60,
    KK_NUMPAD1 = 0x61,
    KK_NUMPAD2 = 0x62,
    KK_NUMPAD3 = 0x63,
    KK_NUMPAD4 = 0x64,
    KK_NUMPAD5 = 0x65,
    KK_NUMPAD6 = 0x66,
    KK_NUMPAD7 = 0x67,
    KK_NUMPAD8 = 0x68,
    KK_NUMPAD9 = 0x69,
    KK_MULTIPLY = 0x6a,
    KK_ADD = 0x6b,
    KK_SEPARATOR = 0x6c,
    KK_SUBTRACT = 0x6d,

    KK_DECIMAL = 0x6e,
    KK_DIVIDE = 0x6f,
    KK_F1 = 0x70,
    KK_F2 = 0x71,
    KK_F3 = 0x72,
    KK_F4 = 0x73,
    KK_F5 = 0x74,
    KK_F6 = 0x75,
    KK_F7 = 0x76,
    KK_F8 = 0x77,
    KK_F9 = 0x78,
    KK_F10 = 0x79,
    KK_F11 = 0x7a,
    KK_F12 = 0x7b,
    KK_F13 = 0x7c,
    KK_F14 = 0x7d,
    KK_F15 = 0x7e,
    KK_F16 = 0x7f,
    KK_F17 = 0x80,
    KK_F18 = 0x81,
    KK_F19 = 0x82,
    KK_F20 = 0x83,
    KK_F21 = 0x84,
    KK_F22 = 0x85,
    KK_F23 = 0x86,
    KK_F24 = 0x87,

    KK_NUMLOCK = 0x90,
    KK_SCROLL = 0x91,

    KK_LEFTSHIFT = 0x10,
    KK_RIGHTSHIFT = 0x10,
    KK_LEFTCONTROL = 0xa2,
    KK_RIGHTCONTROL = 0xa3,
    KK_LEFTALT = 0xa4,
    KK_RIGHTALT = 0xa5,
    KK_BROWSERBACK = 0xa6,
    KK_BROWSERFORWARD = 0xa7,
    KK_BROWSERREFRESH = 0xa8,
    KK_BROWSERSTOP = 0xa9,
    KK_BROWSERSEARCH = 0xaa,
    KK_BROWSERFAVORITES = 0xab,
    KK_BROWSERHOME = 0xac,
    KK_VOLUMEMUTE = 0xad,
    KK_VOLUMEDOWN = 0xae,
    KK_VOLUMEUP = 0xaf,
    KK_MEDIANEXTTRACK = 0xb0,
    KK_MEDIAPREVIOUSTRACK = 0xb1,
    KK_MEDIASTOP = 0xb2,
    KK_MEDIAPLAYPAUSE = 0xb3,
    KK_LAUNCHMAIL = 0xb4,
    KK_SELECTMEDIA = 0xb5,
    KK_LAUNCHAPPLICATION1 = 0xb6,
    KK_LAUNCHAPPLICATION2 = 0xb7,

    KK_OEMSEMICOLON = 0xba,
    KK_OEMPLUS = 0xbb,
    KK_OEMCOMMA = 0xbc,
    KK_OEMMINUS = 0xbd,
    KK_OEMPERIOD = 0xbe,
    KK_OEMQUESTION = 0xbf,
    KK_OEMTILDE = 0xc0,

    KK_OEMOPENBRACKETS = 0xdb,
    KK_OEMPIPE = 0xdc,
    KK_OEMCLOSEBRACKETS = 0xdd,
    KK_OEMQUOTES = 0xde,
    KK_OEM8 = 0xdf,

    KK_OEMBACKSLASH = 0xe2,

    KK_PROCESSKEY = 0xe5,

    KK_OEMCOPY = 0xf2,
    KK_OEMAUTO = 0xf3,
    KK_OEMENLW = 0xf4,

    KK_ATTN = 0xf6,
    KK_CRSEL = 0xf7,
    KK_EXSEL = 0xf8,
    KK_ERASEEOF = 0xf9,
    KK_PLAY = 0xfa,
    KK_ZOOM = 0xfb,

    KK_PA1 = 0xfd,
    KK_OEMCLEAR = 0xfe,
} Keyboard_Keys;