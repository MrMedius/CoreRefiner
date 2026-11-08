#include "IconDrawer.h"

#include <Windows.h>
#include <windowsx.h>

#include <algorithm>
#include <cmath>

////////////////////////////////////////////////////////////////////////////////
// 状态
// 窗口尺寸、当前图、当前工具、命令、顶栏数字框和这一笔的进度都放在这里。
////////////////////////////////////////////////////////////////////////////////

// 窗口类名。注册和创建窗口时用同一个名字。
constexpr const wchar_t* kWindowClass = L"CoreRefinerIconDrawer";
// 左工具栏宽度。
constexpr int kToolWidth = 200;
// 顶栏高度。
constexpr int kTopHeight = 50;
// 右文本栏宽度。
constexpr int kSideWidth = 200;
// 一格最少和最多占多少像素。
constexpr int kZoomMin = 1;
constexpr int kZoomMax = 64;

// 客户区四块的位置。坐标是窗口客户区像素。
struct Layout
{
	RECT tool{};
	RECT top{};
	RECT board{};
	RECT side{};
};

// 落笔方式。画笔写亮，橡皮写灭，开关翻转。
enum class Tool
{
	Pen,
	Eraser,
	Toggle
};

// 左栏下半的命令。撤销回到旧图，重做走回去，清空把格子全部写成灭。
enum class Command
{
	Undo,
	Redo,
	Clear
};

// 三个工具按钮上的字。顺序与 Tool 相同。
constexpr const wchar_t* kToolLabels[] = { L"画笔", L"橡皮", L"开关" };
// 三个命令按钮上的字。顺序与 Command 相同。
constexpr const wchar_t* kCommandLabels[] = { L"撤销", L"重做", L"清空" };
// 顶栏四个方向的字。顺序是左、右、上、下。
constexpr const wchar_t* kExpandLabels[] = { L"左", L"右", L"上", L"下" };

// 左栏按钮位置。tools 在上，commands 在下。
struct ToolBar
{
	// 画笔、橡皮、开关。
	RECT tools[3]{};
	// 撤销、重做、清空。
	RECT commands[3]{};
};

// 顶栏扩像素的位置。labels 是方向文字，edits 是数字框，apply 是应用按钮。
struct ExpandBar
{
	RECT labels[4]{};
	RECT edits[4]{};
	RECT apply{};
};

// 顶栏右侧的放大控件。out 是缩小，percent 是百分数，in 是放大。
struct ZoomBar
{
	RECT out{};
	RECT percent{};
	RECT in{};
};

// 当前图和视图。zoom 是一格的像素边长。
// originX、originY 是缓冲区第一格在棋盘里的位置。原点 (0,0) 在它右方 left 格、下方 up 格。
struct App
{
	IconSheet sheet{};
	int zoom{ 16 };
	// 缓冲区第一格的横坐标。往左加格时它会左移，好让原点停住。
	int originX{ 0 };
	// 缓冲区第一格的纵坐标。往上加格时它会上移，好让原点停住。
	int originY{ 0 };
	// 已经按棋盘大小摆过一次。之后缩放和拖动不再被窗口尺寸改掉。
	bool placed{ false };
	// 中键正在拖动。
	bool dragging{ false };
	// 上一帧拖动时的客户区坐标。
	POINT dragLast{};
	// 当前选中的工具。
	Tool tool{ Tool::Pen };
	// 左键正按着，这一笔还没松开。
	bool painting{ false };
	// 这一笔已经把改之前的图压进撤销栈。
	bool strokePushed{ false };
	// 这一笔已经处理过的格子。1 表示本笔不再改它。
	std::vector<std::uint8_t> strokeSeen;
	// 上一格已经记下，用来把快速拖动中间漏掉的格子补上。
	bool hasStrokeCell{ false };
	// 上一格的横坐标。
	int strokeX{ 0 };
	// 上一格的纵坐标。
	int strokeY{ 0 };
	// 顶栏四个数字框。顺序是左、右、上、下，里面是从原点往该方向的格数。
	HWND expandEdits[4]{};
	// 数字框原来的消息函数。四个框共用。
	WNDPROC expandEditProc{};
	// 数字框和方向文字的字体。窗口销毁时删掉。
	HFONT fieldFont{};
	// 数字框的底色。窗口销毁时删掉。
	HBRUSH editBrush{};
};

// 整个程序只有这一份图和视图。
App gApp{};

////////////////////////////////////////////////////////////////////////////////
// 绘制基础
// 只负责铺色、画边、判断点和画一颗按钮，不改图。
////////////////////////////////////////////////////////////////////////////////

// 用纯色铺满矩形。
void Fill(HDC dc, const RECT& rect, COLORREF color)
{
	HBRUSH brush = CreateSolidBrush(color);
	FillRect(dc, &rect, brush);
	DeleteObject(brush);
}

// 给矩形画一圈边。
void Frame(HDC dc, const RECT& rect, COLORREF color)
{
	HBRUSH brush = CreateSolidBrush(color);
	FrameRect(dc, &rect, brush);
	DeleteObject(brush);
}

// 点是否落在矩形里。pt 是客户区坐标。
bool Contains(const RECT& rect, POINT pt)
{
	return pt.x >= rect.left && pt.x < rect.right && pt.y >= rect.top && pt.y < rect.bottom;
}

// 画一颗按钮。selected 为真时用亮边，表示当前工具。enabled 为假时字变暗，点了也不该改画布。
void DrawButton(HDC dc, const RECT& rect, const wchar_t* label, bool selected, bool enabled)
{
	if (rect.right <= rect.left || rect.bottom <= rect.top)
	{
		return;
	}
	COLORREF fill = RGB(62, 62, 68);
	COLORREF edge = RGB(120, 120, 120);
	COLORREF text = RGB(230, 230, 230);
	if (!enabled)
	{
		fill = RGB(52, 52, 56);
		edge = RGB(90, 90, 90);
		text = RGB(120, 120, 120);
	}
	else if (selected)
	{
		fill = RGB(110, 110, 118);
		edge = RGB(230, 230, 230);
	}
	Fill(dc, rect, fill);
	Frame(dc, rect, edge);
	SetBkMode(dc, TRANSPARENT);
	SetTextColor(dc, text);
	int fontPx = (rect.bottom - rect.top) / 2;
	if (fontPx < 12)
	{
		fontPx = 12;
	}
	if (fontPx > 18)
	{
		fontPx = 18;
	}
	// font 是按钮文字。用完就删。
	HFONT font = CreateFontW(
		-fontPx, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");
	HGDIOBJ old = nullptr;
	if (font != nullptr)
	{
		old = SelectObject(dc, font);
	}
	RECT textRect = rect;
	DrawTextW(dc, label, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	if (font != nullptr)
	{
		SelectObject(dc, old);
		DeleteObject(font);
	}
}

////////////////////////////////////////////////////////////////////////////////
// 布局
// 按客户区算出四块区域，再在左栏和顶栏里排按钮、数字框和放大控件。
////////////////////////////////////////////////////////////////////////////////

// 按客户区算出四块。窗口太窄时工具栏和文本栏一起让出位置，棋盘至少留一块。
Layout MakeLayout(const RECT& client)
{
	const int width = client.right - client.left;
	const int height = client.bottom - client.top;
	int tool = kToolWidth;
	int side = kSideWidth;
	int top = kTopHeight;
	if (tool + side + 32 > width)
	{
		const int spare = (std::max)(0, width - 32);
		tool = spare / 3;
		side = spare - tool;
	}
	if (top + 32 > height)
	{
		top = (std::max)(0, height / 5);
	}

	Layout layout{};
	layout.tool = RECT{ 0, top, tool, height };
	layout.top = RECT{ 0, 0, width, top };
	layout.side = RECT{ width - side, top, width, height };
	layout.board = RECT{ tool, top, width - side, height };
	if (layout.board.right < layout.board.left)
	{
		layout.board.right = layout.board.left;
	}
	if (layout.board.bottom < layout.board.top)
	{
		layout.board.bottom = layout.board.top;
	}
	return layout;
}

// 在左栏里排两排按钮。tool 是左栏。上面三个是工具，隔开一段后是撤销、重做、清空。
ToolBar MakeToolBar(const RECT& tool)
{
	const int margin = 16;
	const int gap = 10;
	const int groupGap = 24;
	const int chrome = margin * 2 + gap * 4 + groupGap;
	int room = (tool.bottom - tool.top) - chrome;
	int buttonH = 40;
	if (room < buttonH * 6)
	{
		buttonH = room / 6;
		if (buttonH < 1)
		{
			buttonH = 1;
		}
	}

	int y = tool.top + margin;
	const int left = tool.left + margin;
	int right = tool.right - margin;
	if (right < left)
	{
		right = left;
	}

	ToolBar bar{};
	for (int i = 0; i < 3; ++i)
	{
		bar.tools[i] = RECT{ left, y, right, y + buttonH };
		y += buttonH + gap;
	}
	y += groupGap - gap;
	for (int i = 0; i < 3; ++i)
	{
		bar.commands[i] = RECT{ left, y, right, y + buttonH };
		y += buttonH + gap;
	}
	return bar;
}

// 在顶栏左侧排四个数字框和应用按钮。top 是顶栏。顺序是左、右、上、下。
ExpandBar MakeExpandBar(const RECT& top)
{
	const int margin = 16;
	const int labelW = 24;
	const int gap = 10;
	const int applyW = 72;
	const int topH = top.bottom - top.top;
	int rowH = 26;
	if (rowH > topH)
	{
		rowH = topH;
	}
	if (rowH < 1)
	{
		rowH = 1;
	}
	const int y = top.top + (topH - rowH) / 2;
	int inner = (top.right - top.left) - margin * 2 - labelW * 4 - gap * 4 - applyW;
	int editW = 56;
	if (inner < editW * 4)
	{
		editW = inner / 4;
		if (editW < 24)
		{
			editW = 24;
		}
	}

	int x = top.left + margin;
	ExpandBar bar{};
	for (int i = 0; i < 4; ++i)
	{
		bar.labels[i] = RECT{ x, y, x + labelW, y + rowH };
		x += labelW;
		bar.edits[i] = RECT{ x, y, x + editW, y + rowH };
		x += editW + gap;
	}
	bar.apply = RECT{ x, y, x + applyW, y + rowH };
	return bar;
}

// 在顶栏右侧排缩小、百分数和放大。top 是顶栏。
ZoomBar MakeZoomBar(const RECT& top)
{
	const int margin = 16;
	const int gap = 8;
	const int buttonW = 28;
	const int percentW = 84;
	const int topH = top.bottom - top.top;
	int rowH = 26;
	if (rowH > topH)
	{
		rowH = topH;
	}
	if (rowH < 1)
	{
		rowH = 1;
	}
	const int y = top.top + (topH - rowH) / 2;
	int x = top.right - margin - buttonW - gap - percentW - gap - buttonW;
	ZoomBar bar{};
	bar.out = RECT{ x, y, x + buttonW, y + rowH };
	x += buttonW + gap;
	bar.percent = RECT{ x, y, x + percentW, y + rowH };
	x += percentW + gap;
	bar.in = RECT{ x, y, x + buttonW, y + rowH };
	return bar;
}

// 读出当前客户区的四块。client 写出客户区矩形。
Layout CurrentLayout(HWND hwnd, RECT& client)
{
	GetClientRect(hwnd, &client);
	return MakeLayout(client);
}

////////////////////////////////////////////////////////////////////////////////
// 视图
// 只管缩放、拖动，以及让原点停在屏幕原处。不改格子里的 0 和 1。
////////////////////////////////////////////////////////////////////////////////

// 第一次有足够大的棋盘时，把整张图放进中央。
void PlaceView(const RECT& board)
{
	if (gApp.placed)
	{
		return;
	}
	const int boardW = board.right - board.left;
	const int boardH = board.bottom - board.top;
	if (boardW < 16 || boardH < 16)
	{
		return;
	}
	int zoom = boardW / gApp.sheet.Width();
	const int zoomY = boardH / gApp.sheet.Height();
	if (zoomY < zoom)
	{
		zoom = zoomY;
	}
	if (zoom < 4)
	{
		zoom = 4;
	}
	if (zoom > 32)
	{
		zoom = 32;
	}
	gApp.zoom = zoom;
	gApp.originX = (boardW - gApp.sheet.Width() * zoom) / 2;
	gApp.originY = (boardH - gApp.sheet.Height() * zoom) / 2;
	gApp.placed = true;
}

// 以光标所在的位置为中心改缩放。notches 为正是放大。
void ZoomAt(POINT cursor, const RECT& board, int notches)
{
	if (notches == 0 || !Contains(board, cursor))
	{
		return;
	}
	PlaceView(board);
	const int oldZoom = gApp.zoom;
	int zoom = oldZoom + notches;
	if (zoom < kZoomMin)
	{
		zoom = kZoomMin;
	}
	if (zoom > kZoomMax)
	{
		zoom = kZoomMax;
	}
	if (zoom == oldZoom)
	{
		return;
	}
	const int localX = cursor.x - board.left;
	const int localY = cursor.y - board.top;
	const double cellX = static_cast<double>(localX - gApp.originX) / static_cast<double>(oldZoom);
	const double cellY = static_cast<double>(localY - gApp.originY) / static_cast<double>(oldZoom);
	gApp.zoom = zoom;
	gApp.originX = static_cast<int>(std::lround(static_cast<double>(localX) - cellX * zoom));
	gApp.originY = static_cast<int>(std::lround(static_cast<double>(localY) - cellY * zoom));
}

// 以棋盘中心为基准改一档缩放。notches 为正是放大，和滚轮一格相同。
void ZoomStep(HWND hwnd, int notches)
{
	RECT client{};
	const Layout layout = CurrentLayout(hwnd, client);
	const POINT center{
		(layout.board.left + layout.board.right) / 2,
		(layout.board.top + layout.board.bottom) / 2
	};
	ZoomAt(center, layout.board, notches);
	InvalidateRect(hwnd, nullptr, FALSE);
}

// 当前放大百分比。一格一个屏幕像素是 100%。
int ZoomPercent()
{
	return gApp.zoom * 100;
}

// 左或上的格数变了之后，把视图反着挪，让原点 (0,0) 仍画在原来的屏幕位置。
// beforeLeft、beforeUp 是改图前往左、往上的格数。
void HoldOrigin(int beforeLeft, int beforeUp)
{
	gApp.originX += (beforeLeft - gApp.sheet.Left()) * gApp.zoom;
	gApp.originY += (beforeUp - gApp.sheet.Up()) * gApp.zoom;
}

////////////////////////////////////////////////////////////////////////////////
// 命中
// 把客户区坐标换成格子坐标。
////////////////////////////////////////////////////////////////////////////////

// 向负无穷取整的除法。n 是被除数，d 是正的除数。
int FloorDiv(int n, int d)
{
	int q = n / d;
	const int r = n % d;
	if (r != 0 && n < 0)
	{
		--q;
	}
	return q;
}

// 客户区坐标换成格子。点不在棋盘里，或格子不在图内，则返回 false。
// cursor 是客户区坐标。board 是棋盘矩形。x、y 写出格子坐标。
bool CellAt(POINT cursor, const RECT& board, int& x, int& y)
{
	if (gApp.zoom <= 0 || !Contains(board, cursor))
	{
		return false;
	}
	const int localX = cursor.x - board.left - gApp.originX;
	const int localY = cursor.y - board.top - gApp.originY;
	const int cellX = FloorDiv(localX, gApp.zoom);
	const int cellY = FloorDiv(localY, gApp.zoom);
	if (cellX < 0 || cellY < 0 || cellX >= gApp.sheet.Width() || cellY >= gApp.sheet.Height())
	{
		return false;
	}
	x = cellX;
	y = cellY;
	return true;
}

////////////////////////////////////////////////////////////////////////////////
// 落笔
// 左键从按下到松开是一笔。真正改过格子才压一次撤销。
////////////////////////////////////////////////////////////////////////////////

// 开始一笔。松开之前，真正改过图才压一次撤销。
void BeginStroke()
{
	gApp.painting = true;
	gApp.strokePushed = false;
	gApp.hasStrokeCell = false;
	const std::size_t count = static_cast<std::size_t>(gApp.sheet.Width()) * static_cast<std::size_t>(gApp.sheet.Height());
	gApp.strokeSeen.assign(count, 0);
}

// 结束一笔。重复调用也安全。
void EndStroke()
{
	gApp.painting = false;
	gApp.hasStrokeCell = false;
	gApp.strokeSeen.clear();
}

// 按当前工具改一格。这一笔里同一格只处理一次。没改到图就不压撤销。
void ApplyToolAt(int x, int y)
{
	if (x < 0 || y < 0 || x >= gApp.sheet.Width() || y >= gApp.sheet.Height())
	{
		return;
	}
	const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(gApp.sheet.Width()) + static_cast<std::size_t>(x);
	if (index >= gApp.strokeSeen.size() || gApp.strokeSeen[index] != 0)
	{
		return;
	}
	gApp.strokeSeen[index] = 1;
	const bool current = gApp.sheet.Get(x, y);
	bool next = current;
	switch (gApp.tool)
	{
	case Tool::Pen:
		next = true;
		break;
	case Tool::Eraser:
		next = false;
		break;
	case Tool::Toggle:
		next = !current;
		break;
	}
	if (next == current)
	{
		return;
	}
	if (!gApp.strokePushed)
	{
		gApp.sheet.PushUndo();
		gApp.strokePushed = true;
	}
	gApp.sheet.Set(x, y, next);
}

// 从上一格连到这一格，把中间跳过的格子也改掉。x、y 是当前格子。
void StrokeTo(int x, int y)
{
	if (!gApp.hasStrokeCell)
	{
		ApplyToolAt(x, y);
		gApp.strokeX = x;
		gApp.strokeY = y;
		gApp.hasStrokeCell = true;
		return;
	}
	int x0 = gApp.strokeX;
	int y0 = gApp.strokeY;
	const int x1 = x;
	const int y1 = y;
	const int dx = x1 > x0 ? x1 - x0 : x0 - x1;
	const int sx = x0 < x1 ? 1 : -1;
	const int dy = y1 > y0 ? y0 - y1 : y1 - y0;
	const int sy = y0 < y1 ? 1 : -1;
	int err = dx + dy;
	while (true)
	{
		ApplyToolAt(x0, y0);
		if (x0 == x1 && y0 == y1)
		{
			break;
		}
		const int e2 = err * 2;
		if (e2 >= dy)
		{
			err += dy;
			x0 += sx;
		}
		if (e2 <= dx)
		{
			err += dx;
			y0 += sy;
		}
	}
	gApp.strokeX = x;
	gApp.strokeY = y;
}

////////////////////////////////////////////////////////////////////////////////
// 撤销、重做、清空
// 改图前压一份旧图。撤销和重做在两座栈之间移动。栈空，或图已经全灭，都不改画布。
////////////////////////////////////////////////////////////////////////////////

// 图上是否还有亮着的格子。
bool AnyOn()
{
	for (int y = 0; y < gApp.sheet.Height(); ++y)
	{
		for (int x = 0; x < gApp.sheet.Width(); ++x)
		{
			if (gApp.sheet.Get(x, y))
			{
				return true;
			}
		}
	}
	return false;
}

// 这颗命令现在点下去会不会改画布。command 是撤销、重做或清空。
bool CommandEnabled(Command command)
{
	switch (command)
	{
	case Command::Undo:
		return gApp.sheet.CanUndo();
	case Command::Redo:
		return gApp.sheet.CanRedo();
	case Command::Clear:
		return AnyOn();
	}
	return false;
}

// 正在落笔或拖动时，不插入另一次改图。
bool ViewBusy()
{
	return gApp.painting || gApp.dragging;
}

// 把当前四个方向的格数写回顶栏。定义在扩像素一节。撤销改了格数之后要刷新。
void SyncExpandFields();

// 撤销一步。栈空或视图正忙时画布不动。成功后四个数字框写回这一步各方向的格数。
void DoUndo(HWND hwnd)
{
	if (ViewBusy())
	{
		return;
	}
	const int beforeLeft = gApp.sheet.Left();
	const int beforeUp = gApp.sheet.Up();
	if (!gApp.sheet.Undo())
	{
		return;
	}
	HoldOrigin(beforeLeft, beforeUp);
	SyncExpandFields();
	InvalidateRect(hwnd, nullptr, FALSE);
}

// 重做一步。栈空或视图正忙时画布不动。成功后四个数字框写回这一步各方向的格数。
void DoRedo(HWND hwnd)
{
	if (ViewBusy())
	{
		return;
	}
	const int beforeLeft = gApp.sheet.Left();
	const int beforeUp = gApp.sheet.Up();
	if (!gApp.sheet.Redo())
	{
		return;
	}
	HoldOrigin(beforeLeft, beforeUp);
	SyncExpandFields();
	InvalidateRect(hwnd, nullptr, FALSE);
}

// 全部写成灭，并单独占一步撤销。已经全灭，或视图正忙，则不动。
void DoClear(HWND hwnd)
{
	if (ViewBusy() || !AnyOn())
	{
		return;
	}
	gApp.sheet.PushUndo();
	gApp.sheet.Clear();
	InvalidateRect(hwnd, nullptr, FALSE);
}

// 执行左栏下半的一颗命令。
void RunCommand(HWND hwnd, Command command)
{
	switch (command)
	{
	case Command::Undo:
		DoUndo(hwnd);
		break;
	case Command::Redo:
		DoRedo(hwnd);
		break;
	case Command::Clear:
		DoClear(hwnd);
		break;
	}
}

////////////////////////////////////////////////////////////////////////////////
// 四向扩像素
// 原点固定在 (0,0)。顶栏四个数是从原点往左、右、上、下各有多少格。
// 左、上是负方向，右、下是正方向。宽是左加右，高是上加下。
// 数变大就在那一侧加空格，变小就裁掉那一侧。最小是 0。和现在一样，或宽高超过 1024，则不动。
////////////////////////////////////////////////////////////////////////////////

// 读一个数字框。edit 是框。value 写出非负整数。空着或不是整数则返回 false。
bool ReadSide(HWND edit, int& value)
{
	if (edit == nullptr)
	{
		return false;
	}
	wchar_t text[16]{};
	GetWindowTextW(edit, text, 16);
	if (text[0] == 0)
	{
		return false;
	}
	long long number = 0;
	for (const wchar_t* p = text; *p != 0; ++p)
	{
		if (*p < L'0' || *p > L'9')
		{
			return false;
		}
		number = number * 10 + (*p - L'0');
		if (number > 1000000)
		{
			return false;
		}
	}
	value = static_cast<int>(number);
	return true;
}

// 读出左、右、上、下。有一框读失败则整次作废。
bool ReadExpandSides(int& left, int& right, int& up, int& down)
{
	return ReadSide(gApp.expandEdits[0], left)
		&& ReadSide(gApp.expandEdits[1], right)
		&& ReadSide(gApp.expandEdits[2], up)
		&& ReadSide(gApp.expandEdits[3], down);
}

// 应用按钮现在点下去会不会改画布。
bool ExpandEnabled()
{
	if (ViewBusy())
	{
		return false;
	}
	int left = 0;
	int right = 0;
	int up = 0;
	int down = 0;
	if (!ReadExpandSides(left, right, up, down))
	{
		return false;
	}
	return gApp.sheet.CanSetSides(left, right, up, down);
}

// 把当前四个方向的格数写进数字框。内容和现在一样的框不动，免得光标乱跳。
void SyncExpandFields()
{
	const int sides[4] = {
		gApp.sheet.Left(),
		gApp.sheet.Right(),
		gApp.sheet.Up(),
		gApp.sheet.Down()
	};
	for (int i = 0; i < 4; ++i)
	{
		if (gApp.expandEdits[i] == nullptr)
		{
			continue;
		}
		wchar_t next[16]{};
		wsprintfW(next, L"%d", sides[i]);
		wchar_t current[16]{};
		GetWindowTextW(gApp.expandEdits[i], current, 16);
		if (lstrcmpW(current, next) != 0)
		{
			SetWindowTextW(gApp.expandEdits[i], next);
		}
	}
}

// 按四个方向的格数重排。成功占一步撤销，原点停在原来的屏幕位置。写回框里，去掉多余的前导零。
void DoExpand(HWND hwnd)
{
	if (ViewBusy())
	{
		return;
	}
	int left = 0;
	int right = 0;
	int up = 0;
	int down = 0;
	if (!ReadExpandSides(left, right, up, down) || !gApp.sheet.CanSetSides(left, right, up, down))
	{
		return;
	}
	const int beforeLeft = gApp.sheet.Left();
	const int beforeUp = gApp.sheet.Up();
	gApp.sheet.PushUndo();
	if (!gApp.sheet.SetSides(left, right, up, down))
	{
		return;
	}
	HoldOrigin(beforeLeft, beforeUp);
	SyncExpandFields();
	InvalidateRect(hwnd, nullptr, FALSE);
}

// 数字框的消息。回车执行应用。Ctrl+Z、Ctrl+Y 仍交给主窗口撤销和重做。
LRESULT CALLBACK ExpandEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_KEYDOWN)
	{
		const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
		if (wParam == VK_RETURN)
		{
			DoExpand(GetParent(hwnd));
			return 0;
		}
		if (ctrl && (wParam == 'Z' || wParam == 'Y'))
		{
			SendMessageW(GetParent(hwnd), WM_KEYDOWN, wParam, lParam);
			return 0;
		}
	}
	return CallWindowProcW(gApp.expandEditProc, hwnd, msg, wParam, lParam);
}

// 把四个数字框挪到顶栏当前的位置。
void PlaceExpandEdits(HWND hwnd)
{
	RECT client{};
	const Layout layout = CurrentLayout(hwnd, client);
	const ExpandBar bar = MakeExpandBar(layout.top);
	for (int i = 0; i < 4; ++i)
	{
		if (gApp.expandEdits[i] == nullptr)
		{
			continue;
		}
		const RECT& rect = bar.edits[i];
		MoveWindow(gApp.expandEdits[i], rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, TRUE);
	}
}

// 创建四个数字框。instance 用来挂到主窗口上。
void CreateExpandEdits(HWND hwnd, HINSTANCE instance)
{
	gApp.fieldFont = CreateFontW(
		-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");
	gApp.editBrush = CreateSolidBrush(RGB(32, 32, 36));
	for (int i = 0; i < 4; ++i)
	{
		gApp.expandEdits[i] = CreateWindowExW(
			0,
			L"EDIT",
			L"",
			WS_CHILD | WS_VISIBLE | ES_NUMBER | ES_CENTER | WS_BORDER,
			0,
			0,
			0,
			0,
			hwnd,
			nullptr,
			instance,
			nullptr);
		if (gApp.expandEdits[i] == nullptr)
		{
			continue;
		}
		SendMessageW(gApp.expandEdits[i], EM_LIMITTEXT, 8, 0);
		if (gApp.fieldFont != nullptr)
		{
			SendMessageW(gApp.expandEdits[i], WM_SETFONT, reinterpret_cast<WPARAM>(gApp.fieldFont), FALSE);
		}
		const auto previous = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
			gApp.expandEdits[i], GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(ExpandEditProc)));
		if (gApp.expandEditProc == nullptr)
		{
			gApp.expandEditProc = previous;
		}
	}
	PlaceExpandEdits(hwnd);
	SyncExpandFields();
}

// 关掉窗口时释放顶栏数字框用过的字体和底色。
void ReleaseExpandUi()
{
	if (gApp.fieldFont != nullptr)
	{
		DeleteObject(gApp.fieldFont);
		gApp.fieldFont = nullptr;
	}
	if (gApp.editBrush != nullptr)
	{
		DeleteObject(gApp.editBrush);
		gApp.editBrush = nullptr;
	}
}

////////////////////////////////////////////////////////////////////////////////
// 画面
// 把四块区域、棋盘、左栏按钮、顶栏扩像素和放大百分比画出来。
////////////////////////////////////////////////////////////////////////////////

// 画中央棋盘。board 是棋盘在客户区里的矩形。
void PaintBoard(HDC dc, const RECT& board)
{
	const int boardW = board.right - board.left;
	const int boardH = board.bottom - board.top;
	if (boardW <= 0 || boardH <= 0 || gApp.zoom <= 0)
	{
		return;
	}
	const int saved = SaveDC(dc);
	IntersectClipRect(dc, board.left, board.top, board.right, board.bottom);

	int x0 = (-gApp.originX) / gApp.zoom;
	int y0 = (-gApp.originY) / gApp.zoom;
	int x1 = (boardW - gApp.originX + gApp.zoom - 1) / gApp.zoom;
	int y1 = (boardH - gApp.originY + gApp.zoom - 1) / gApp.zoom;
	if (x0 < 0) { x0 = 0; }
	if (y0 < 0) { y0 = 0; }
	if (x1 > gApp.sheet.Width()) { x1 = gApp.sheet.Width(); }
	if (y1 > gApp.sheet.Height()) { y1 = gApp.sheet.Height(); }

	for (int y = y0; y < y1; ++y)
	{
		for (int x = x0; x < x1; ++x)
		{
			RECT cell{
				board.left + gApp.originX + x * gApp.zoom,
				board.top + gApp.originY + y * gApp.zoom,
				board.left + gApp.originX + (x + 1) * gApp.zoom,
				board.top + gApp.originY + (y + 1) * gApp.zoom
			};
			const COLORREF color = gApp.sheet.Get(x, y) ? RGB(230, 230, 230) : RGB(46, 46, 46);
			Fill(dc, cell, color);
		}
	}

	if (gApp.zoom >= 3)
	{
		HPEN pen = CreatePen(PS_SOLID, 1, RGB(96, 96, 96));
		HGDIOBJ old = SelectObject(dc, pen);
		const int left = board.left + gApp.originX;
		const int top = board.top + gApp.originY;
		const int right = left + gApp.sheet.Width() * gApp.zoom;
		const int bottom = top + gApp.sheet.Height() * gApp.zoom;
		for (int x = 0; x <= gApp.sheet.Width(); ++x)
		{
			const int px = left + x * gApp.zoom;
			MoveToEx(dc, px, top, nullptr);
			LineTo(dc, px, bottom);
		}
		for (int y = 0; y <= gApp.sheet.Height(); ++y)
		{
			const int py = top + y * gApp.zoom;
			MoveToEx(dc, left, py, nullptr);
			LineTo(dc, right, py);
		}
		SelectObject(dc, old);
		DeleteObject(pen);
	}
	RestoreDC(dc, saved);
}

// 画左栏两排按钮。tool 是左栏矩形。
void PaintToolBar(HDC dc, const RECT& tool)
{
	const ToolBar bar = MakeToolBar(tool);
	const int clip = SaveDC(dc);
	IntersectClipRect(dc, tool.left, tool.top, tool.right, tool.bottom);
	for (int i = 0; i < 3; ++i)
	{
		DrawButton(dc, bar.tools[i], kToolLabels[i], static_cast<int>(gApp.tool) == i, true);
	}
	for (int i = 0; i < 3; ++i)
	{
		const Command command = static_cast<Command>(i);
		DrawButton(dc, bar.commands[i], kCommandLabels[i], false, CommandEnabled(command));
	}
	RestoreDC(dc, clip);
}

// 画顶栏的方向文字和应用按钮。数字框是子窗口，自己画字。top 是顶栏矩形。
void PaintExpandBar(HDC dc, const RECT& top)
{
	const ExpandBar bar = MakeExpandBar(top);
	const int clip = SaveDC(dc);
	IntersectClipRect(dc, top.left, top.top, top.right, top.bottom);
	SetBkMode(dc, TRANSPARENT);
	SetTextColor(dc, RGB(230, 230, 230));
	HGDIOBJ old = nullptr;
	if (gApp.fieldFont != nullptr)
	{
		old = SelectObject(dc, gApp.fieldFont);
	}
	for (int i = 0; i < 4; ++i)
	{
		RECT text = bar.labels[i];
		DrawTextW(dc, kExpandLabels[i], -1, &text, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	}
	if (gApp.fieldFont != nullptr)
	{
		SelectObject(dc, old);
	}
	DrawButton(dc, bar.apply, L"应用", false, ExpandEnabled());
	RestoreDC(dc, clip);
}

// 画顶栏右侧的缩小、百分数和放大。top 是顶栏矩形。
void PaintZoomBar(HDC dc, const RECT& top)
{
	const ZoomBar bar = MakeZoomBar(top);
	const int clip = SaveDC(dc);
	IntersectClipRect(dc, top.left, top.top, top.right, top.bottom);
	DrawButton(dc, bar.out, L"-", false, gApp.zoom > kZoomMin);
	DrawButton(dc, bar.in, L"+", false, gApp.zoom < kZoomMax);
	SetBkMode(dc, TRANSPARENT);
	SetTextColor(dc, RGB(230, 230, 230));
	HGDIOBJ old = nullptr;
	if (gApp.fieldFont != nullptr)
	{
		old = SelectObject(dc, gApp.fieldFont);
	}
	wchar_t text[16]{};
	wsprintfW(text, L"%d%%", ZoomPercent());
	RECT percent = bar.percent;
	DrawTextW(dc, text, -1, &percent, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	if (gApp.fieldFont != nullptr)
	{
		SelectObject(dc, old);
	}
	RestoreDC(dc, clip);
}

// 给四块区域画边。
void PaintFrames(HDC dc, const Layout& layout)
{
	const COLORREF edge = RGB(180, 180, 180);
	Frame(dc, layout.tool, edge);
	Frame(dc, layout.top, edge);
	Frame(dc, layout.board, edge);
	Frame(dc, layout.side, edge);
}

// 画整块客户区。dc 是客户区的画布。
void Paint(HDC dc, const RECT& client)
{
	const Layout layout = MakeLayout(client);
	PlaceView(layout.board);

	Fill(dc, client, RGB(18, 18, 18));
	Fill(dc, layout.tool, RGB(48, 48, 52));
	Fill(dc, layout.top, RGB(48, 48, 52));
	Fill(dc, layout.side, RGB(42, 42, 46));
	PaintBoard(dc, layout.board);
	PaintToolBar(dc, layout.tool);
	PaintExpandBar(dc, layout.top);
	PaintZoomBar(dc, layout.top);
	PaintFrames(dc, layout);
}

// 画到一张内存位图上再贴到窗口，避免闪烁。
void PaintWindow(HWND hwnd)
{
	PAINTSTRUCT paint{};
	HDC windowDc = BeginPaint(hwnd, &paint);
	RECT client{};
	GetClientRect(hwnd, &client);
	// bitmapW、bitmapH 是客户区宽高，至少 1，用来建内存位图。
	const int bitmapW = client.right > 1 ? client.right : 1;
	const int bitmapH = client.bottom > 1 ? client.bottom : 1;
	HDC memory = CreateCompatibleDC(windowDc);
	HBITMAP bitmap = CreateCompatibleBitmap(windowDc, bitmapW, bitmapH);
	HGDIOBJ old = SelectObject(memory, bitmap);
	Paint(memory, client);
	BitBlt(windowDc, 0, 0, client.right, client.bottom, memory, 0, 0, SRCCOPY);
	SelectObject(memory, old);
	DeleteObject(bitmap);
	DeleteDC(memory);
	EndPaint(hwnd, &paint);
}

////////////////////////////////////////////////////////////////////////////////
// 窗口消息
// 把输入分给视图、落笔、命令和扩像素。这里不直接改格子。
////////////////////////////////////////////////////////////////////////////////

// 滚轮缩放。光标要在棋盘上。
bool OnMouseWheel(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	POINT cursor{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	ScreenToClient(hwnd, &cursor);
	RECT client{};
	const Layout layout = CurrentLayout(hwnd, client);
	const int notches = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
	ZoomAt(cursor, layout.board, notches);
	InvalidateRect(hwnd, nullptr, FALSE);
	return true;
}

// 客户区尺寸变了，数字框跟着顶栏挪。
void OnSize(HWND hwnd)
{
	PlaceExpandEdits(hwnd);
	InvalidateRect(hwnd, nullptr, FALSE);
}

// 左键按下。点到放大、缩小、应用、命令或工具就执行；点到棋盘就开始一笔。
bool OnLeftDown(HWND hwnd, LPARAM lParam)
{
	const POINT cursor{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	RECT client{};
	const Layout layout = CurrentLayout(hwnd, client);
	const ZoomBar zoom = MakeZoomBar(layout.top);
	if (Contains(zoom.out, cursor))
	{
		ZoomStep(hwnd, -1);
		return true;
	}
	if (Contains(zoom.in, cursor))
	{
		ZoomStep(hwnd, 1);
		return true;
	}
	const ExpandBar expand = MakeExpandBar(layout.top);
	if (Contains(expand.apply, cursor))
	{
		DoExpand(hwnd);
		return true;
	}
	const ToolBar bar = MakeToolBar(layout.tool);
	for (int i = 0; i < 3; ++i)
	{
		if (!Contains(bar.commands[i], cursor))
		{
			continue;
		}
		RunCommand(hwnd, static_cast<Command>(i));
		return true;
	}
	for (int i = 0; i < 3; ++i)
	{
		if (!Contains(bar.tools[i], cursor))
		{
			continue;
		}
		gApp.tool = static_cast<Tool>(i);
		InvalidateRect(hwnd, nullptr, FALSE);
		return true;
	}
	if (gApp.dragging || !Contains(layout.board, cursor))
	{
		return false;
	}
	PlaceView(layout.board);
	BeginStroke();
	SetCapture(hwnd);
	int x = 0;
	int y = 0;
	if (CellAt(cursor, layout.board, x, y))
	{
		StrokeTo(x, y);
	}
	InvalidateRect(hwnd, nullptr, FALSE);
	return true;
}

// 左键松开，结束这一笔。
bool OnLeftUp(HWND hwnd)
{
	if (gApp.painting)
	{
		EndStroke();
		if (GetCapture() == hwnd)
		{
			ReleaseCapture();
		}
	}
	return true;
}

// 中键按下，开始拖动画布。正在落笔时不抢。
bool OnMiddleDown(HWND hwnd, LPARAM lParam)
{
	const POINT cursor{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	RECT client{};
	const Layout layout = CurrentLayout(hwnd, client);
	if (gApp.painting || !Contains(layout.board, cursor))
	{
		return false;
	}
	PlaceView(layout.board);
	gApp.dragging = true;
	gApp.dragLast = cursor;
	SetCapture(hwnd);
	return true;
}

// 鼠标移动。拖动时改原点，落笔时改格子。
bool OnMouseMove(HWND hwnd, LPARAM lParam)
{
	const POINT cursor{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	if (gApp.dragging)
	{
		gApp.originX += cursor.x - gApp.dragLast.x;
		gApp.originY += cursor.y - gApp.dragLast.y;
		gApp.dragLast = cursor;
		InvalidateRect(hwnd, nullptr, FALSE);
	}
	if (gApp.painting)
	{
		RECT client{};
		const Layout layout = CurrentLayout(hwnd, client);
		int x = 0;
		int y = 0;
		if (CellAt(cursor, layout.board, x, y))
		{
			StrokeTo(x, y);
		}
		else
		{
			gApp.hasStrokeCell = false;
		}
		InvalidateRect(hwnd, nullptr, FALSE);
	}
	return true;
}

// 中键松开，结束拖动。
bool OnMiddleUp(HWND hwnd)
{
	if (gApp.dragging)
	{
		gApp.dragging = false;
		ReleaseCapture();
	}
	return true;
}

// 捕获被拿走时，拖动和落笔都停下。
bool OnCaptureChanged()
{
	gApp.dragging = false;
	EndStroke();
	return true;
}

// Ctrl+Z 撤销，Ctrl+Y 重做。
bool OnKeyDown(HWND hwnd, WPARAM wParam)
{
	const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
	if (!ctrl)
	{
		return false;
	}
	if (wParam == 'Z')
	{
		DoUndo(hwnd);
		return true;
	}
	if (wParam == 'Y')
	{
		DoRedo(hwnd);
		return true;
	}
	return false;
}

// 处理窗口消息。关掉窗口时退出程序。
// hwnd 是窗口。msg 是消息编号。wParam、lParam 是这条消息自带的参数。
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_ERASEBKGND:
		return 1;
	case WM_PAINT:
		PaintWindow(hwnd);
		return 0;
	case WM_SIZE:
		OnSize(hwnd);
		return 0;
	case WM_COMMAND:
		if (HIWORD(wParam) == EN_CHANGE)
		{
			InvalidateRect(hwnd, nullptr, FALSE);
			return 0;
		}
		break;
	case WM_CTLCOLOREDIT:
	{
		if (gApp.editBrush == nullptr)
		{
			break;
		}
		const HDC editDc = reinterpret_cast<HDC>(wParam);
		SetTextColor(editDc, RGB(230, 230, 230));
		SetBkColor(editDc, RGB(32, 32, 36));
		return reinterpret_cast<LRESULT>(gApp.editBrush);
	}
	case WM_NCDESTROY:
		ReleaseExpandUi();
		break;
	case WM_MOUSEWHEEL:
		if (OnMouseWheel(hwnd, wParam, lParam))
		{
			return 0;
		}
		break;
	case WM_LBUTTONDOWN:
		if (OnLeftDown(hwnd, lParam))
		{
			return 0;
		}
		break;
	case WM_LBUTTONUP:
		if (OnLeftUp(hwnd))
		{
			return 0;
		}
		break;
	case WM_MBUTTONDOWN:
		if (OnMiddleDown(hwnd, lParam))
		{
			return 0;
		}
		break;
	case WM_MOUSEMOVE:
		if (OnMouseMove(hwnd, lParam))
		{
			return 0;
		}
		break;
	case WM_MBUTTONUP:
		if (OnMiddleUp(hwnd))
		{
			return 0;
		}
		break;
	case WM_CAPTURECHANGED:
		if (OnCaptureChanged())
		{
			return 0;
		}
		break;
	case WM_KEYDOWN:
		if (OnKeyDown(hwnd, wParam))
		{
			return 0;
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		break;
	}
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

////////////////////////////////////////////////////////////////////////////////
// 入口
// 先核对画布规则，再打开窗口并进入消息循环。
////////////////////////////////////////////////////////////////////////////////

// 程序入口。弹出窗口，直到用户关掉它。
// instance 是本程序实例，用来注册窗口类。show 是启动时窗口怎么显示。
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show)
{
	// 画布规则不对就不出窗口。
	if (!IconSheet::SelfCheck())
	{
		return -1;
	}

	// wc 描述窗口类：消息函数、实例、类名、光标和背景。
	WNDCLASSW wc{};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = instance;
	wc.lpszClassName = kWindowClass;
	wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	if (RegisterClassW(&wc) == 0)
	{
		return -1;
	}

	// hwnd 是主窗口。标题固定为 IconDrawer。
	HWND hwnd = CreateWindowExW(
		0,
		kWindowClass,
		L"IconDrawer",
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		960,
		640,
		nullptr,
		nullptr,
		instance,
		nullptr);
	if (hwnd == nullptr)
	{
		return -1;
	}

	CreateExpandEdits(hwnd, instance);
	ShowWindow(hwnd, show);
	UpdateWindow(hwnd);

	// message 是当前取出的一条窗口消息。
	MSG message{};
	while (GetMessageW(&message, nullptr, 0, 0) > 0)
	{
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}
	return static_cast<int>(message.wParam);
}
