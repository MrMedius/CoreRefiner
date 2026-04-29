#pragma once
#include <queue>
#include <optional>

class Mouse
{
	friend class InputCodex;
public:
	struct RawDelta
	{
		int x,y;
	};
	class Event
	{
	public:
		enum class Type
		{
			LPress,
			LRelease,
			RPress,
			RRelease,
			WheelUp,
			WheelDown,
			Move,
			Enter,
			Leave,
			Invalid
		};
	private:
		Type type;
		bool leftIsPressed;
		bool rightIsPressed;
		int x;
		int y;
	public:
		Event()
			:
			type(Type::Invalid),
			leftIsPressed(false),
			rightIsPressed(false),
			x(0),
			y(0)
		{
		}
		Event(Type type, const Mouse& parent)
			:
			type(type),
			leftIsPressed(parent.leftState),
			rightIsPressed(parent.rightState),
			x(parent.x),
			y(parent.y)
		{
		}
		bool IsValid() const
		{
			return type != Type::Invalid;
		}
		Type GetType() const
		{
			return type;
		}
		std::pair<int, int> GetPos() const
		{
			return{ x,y };
		}
		int GetPosX() const
		{
			return x;
		}
		int GetPosY() const
		{
			return y;
		}
		bool LeftIsPressed() const
		{
			return leftIsPressed;
		}
		bool RightIsPressed() const
		{
			return rightIsPressed;
		}
	};
public:
	Mouse() :x(0), y(0) {};
	Mouse(const Mouse&) = delete;
	Mouse& operator=(const Mouse&) = delete;
	void Update(void);
	std::pair<int, int> GetPos() const noexcept;
	int GetPosX() const noexcept;
	int GetPosY() const noexcept;
	bool IsMove() const noexcept;
	bool LeftIsPressed() const noexcept;
	bool LeftIsTriggered() const noexcept;
	bool LeftIsReleased() const noexcept;
	bool RightIsPressed() const noexcept;
	bool RightIsTriggered() const noexcept;
	bool RightIsReleased() const noexcept;
	bool IsInWindow() const noexcept;
	std::optional<Mouse::Event> Read() noexcept;
	bool IsEmpty() const
	{
		return buffer.empty();
	}
	void Flush() noexcept;
	void EnableRaw() noexcept;
	void DisableRaw() noexcept;
	bool RawEnabled() const noexcept;
	std::optional<RawDelta> ReadRawDelta() noexcept;
private:
	void OnMouseMove( int x,int y ) noexcept;
	void OnMouseLeave() noexcept;
	void OnMouseEnter() noexcept;
	void OnLeftPressed( int x,int y ) noexcept;
	void OnLeftReleased( int x,int y ) noexcept;
	void OnRightPressed( int x,int y ) noexcept;
	void OnRightReleased( int x,int y ) noexcept;
	void OnWheelUp( int x,int y ) noexcept;
	void OnWheelDown( int x,int y ) noexcept;
	void TrimBuffer() noexcept;
	void OnWheelDelta( int x,int y,int delta ) noexcept;
	void OnRawDelta(int dx, int dy) noexcept;
	void TrimRawInputBuffer() noexcept;
private:
	static constexpr unsigned int bufferSize = 4u;
	int x;
	int y;
	int oldX;
	int oldY;
	bool leftState = false;
	bool rightState = false;
	bool leftOldState = false;
	bool rightOldState = false;
	bool isInWindow = false;
	int wheelDeltaCarry = 0;
	bool rawEnabled = false;
	std::queue<Event> buffer;
	std::queue<RawDelta> rawDeltaBuffer;
};