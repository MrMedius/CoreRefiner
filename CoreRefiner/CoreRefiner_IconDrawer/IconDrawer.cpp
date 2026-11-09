#include "IconDrawer.h"

#include <Windows.h>
#include <commdlg.h>
#include <windowsx.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

// 打开和保存用系统文件框。库写在这里，工程文件不用再加。
#pragma comment(lib, "comdlg32.lib")

////////////////////////////////////////////////////////////////////////////////
// 状态
// 窗口尺寸、当前图、当前工具、命令、顶栏数字框和这一笔的进度都放在这里。
////////////////////////////////////////////////////////////////////////////////

// 窗口类名。注册和创建窗口时用同一个名字。
constexpr const wchar_t* kWindowClass = L"CoreRefinerIconDrawer";
// 左工具栏宽度。只放两个字的按钮，快捷键不写在按钮上。
constexpr int kToolWidth = 100;
// 顶栏高度。
constexpr int kTopHeight = 50;
// 右文本栏宽度。留给 01 和行号，16 列能一次看完。
constexpr int kSideWidth = 280;
// 一格最少和最多占多少像素。
constexpr int kZoomMin = 1;
constexpr int kZoomMax = 64;
// 打开时那张图的边长。归位始终按这张 16×16 摆，不跟着后来的宽高改。
constexpr int kHomeSide = 16;
// 棋盘左边行号的宽度。只放下四个数字，例如 1024。
constexpr int kRowRuler = 28;
// 棋盘上边列号的高度。
constexpr int kColRuler = 20;
// 右侧文字离边的空隙。
constexpr int kSidePad = 4;
// 右侧行号的宽度。和棋盘左边同一套宽窄。
constexpr int kSideRowRuler = 28;
// 右侧列号的高度。
constexpr int kSideColRuler = 18;

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

// 左栏下半的命令。撤销回到旧图，恢复走回去，清空把格子全部写成灭。
enum class Command
{
	Undo,
	Redo,
	Clear
};

// 三个工具按钮上的字。顺序与 Tool 相同。快捷键不写在按钮上。
constexpr const wchar_t* kToolLabels[] = { L"画笔", L"橡皮", L"开关" };
// 三个命令按钮上的字。顺序与 Command 相同。快捷键不写在按钮上。
constexpr const wchar_t* kCommandLabels[] = { L"撤销", L"恢复", L"清空" };
// 顶栏四个方向的字。顺序是左、右、上、下。
constexpr const wchar_t* kExpandLabels[] = { L"左", L"右", L"上", L"下" };

// 左栏按钮位置。tools 在上，commands 在中，views 在下。组与组之间空得比按钮间距大。
struct ToolBar
{
	// 画笔、橡皮、开关。
	RECT tools[3]{};
	// 撤销、恢复、清空。
	RECT commands[3]{};
	// 标尺开关。打开时按钮亮着。
	RECT ruler{};
	// 归位。百分数回到 100%，原点按 16×16 回到中间。
	RECT home{};
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
// originX、originY 是缓冲区第一格在格子区域里的位置。格子区域不含标尺。
// 原点 (0,0) 在它右方 left 格、下方 up 格。
struct App
{
	IconSheet sheet{};
	int zoom{ 16 };
	// 100% 时一格占多少屏幕像素。显示百分数 = 格子像素 * 100 / zoomBase。
	int zoomBase{ 32 };
	// 顶栏上显示的百分数。加减按它取整十，不从格子像素反推。
	int shownPercent{ 100 };
	// 这一下按键已经按整十缩放过。紧跟着的字符不要再缩一次。
	bool zoomKeyUsed{ false };
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
	// 右侧 01 和标尺用的等宽字体。窗口销毁时删掉。
	HFONT monoFont{};
	// 等宽字体里一个数字的宽度。用来排 01 和滚动。
	int monoW{ 8 };
	// 等宽字体里一行的高度。
	int monoH{ 16 };
	// 右侧文字已经向右滚过的像素。
	int sideX{ 0 };
	// 右侧文字已经向下滚过的像素。
	int sideY{ 0 };
	// 鼠标下有没有一格。没有时标尺不额外标出那一格。
	bool hover{ false };
	// 鼠标下那一格的缓冲区列号。
	int hoverX{ 0 };
	// 鼠标下那一格的缓冲区行号。
	int hoverY{ 0 };
	// 棋盘和右侧的标尺是否打开。一开始关掉，避免挡住格子。
	bool rulers{ false };
	// 右侧文字的横滑条。文字比窗口窄时藏起来。
	HWND sideHScroll{};
	// 右侧文字的竖滑条。文字比窗口矮时藏起来。
	HWND sideVScroll{};
	// 上次打开或保存的 01 文本完整路径。下一次文件框从这里接着选。
	wchar_t textPath[MAX_PATH]{};
	// 上次保存的黑白 BMP 完整路径。下一次文件框从这里接着选。
	wchar_t bmpPath[MAX_PATH]{};
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

// 标尺打开时，格子区域往右让出的像素。关掉时是 0，图按整块中间区域排。
int RulerShiftX()
{
	return gApp.rulers ? kRowRuler : 0;
}

// 标尺打开时，格子区域往下让出的像素。关掉时是 0。
int RulerShiftY()
{
	return gApp.rulers ? kColRuler : 0;
}

// 棋盘里真正画格子的区域。标尺打开时，左边和上边留给行号、列号。
RECT CellArea(const RECT& board)
{
	RECT cells{
		board.left + RulerShiftX(),
		board.top + RulerShiftY(),
		board.right,
		board.bottom
	};
	if (cells.right < cells.left)
	{
		cells.right = cells.left;
	}
	if (cells.bottom < cells.top)
	{
		cells.bottom = cells.top;
	}
	return cells;
}

// 右侧文字的分区。openText、saveText、saveBmp 是打开和两种保存。colRuler、rowRuler 是列号和行号。
// text 是 01。hScroll、vScroll 是滑条。showH、showV 表示这两条现在要不要露出来。
struct SideView
{
	// 打开一份 01 文本，换成当前图。
	RECT openText{};
	// 把当前图写成 01 文本。
	RECT saveText{};
	// 把当前图写成黑白 BMP。一格一个像素。
	RECT saveBmp{};
	RECT colRuler{};
	RECT rowRuler{};
	RECT text{};
	RECT hScroll{};
	RECT vScroll{};
	bool showH{ false };
	bool showV{ false };
};

// 滑条的厚度。系统给不出时用 16。
int ScrollBarThickness()
{
	const int thickness = GetSystemMetrics(SM_CXVSCROLL);
	return thickness > 0 ? thickness : 16;
}

// 在右栏里排打开、保存文本、保存图片、行号、列号、01 和滑条。side 是右栏。
// 文字放得下时不留滑条。行号在文字左边，不跟着左右滚动。
SideView MakeSideView(const RECT& side)
{
	const int left = side.left + kSidePad;
	const int top = side.top + kSidePad;
	int right = side.right - kSidePad;
	int bottom = side.bottom - kSidePad;
	if (right < left)
	{
		right = left;
	}
	if (bottom < top)
	{
		bottom = top;
	}

	SideView view{};
	// fileH 是顶部那排按钮的高度。三颗按钮平分右栏宽度。
	int fileH = 26;
	const int roomH = bottom - top;
	if (fileH > roomH)
	{
		fileH = roomH;
	}
	if (fileH < 1)
	{
		fileH = 1;
	}
	const int fileGap = 6;
	const int fileCount = 3;
	int span = right - left - fileGap * (fileCount - 1);
	if (span < fileCount)
	{
		span = fileCount;
	}
	const int buttonW = span / fileCount;
	view.openText = RECT{ left, top, left + buttonW, top + fileH };
	view.saveText = RECT{ left + buttonW + fileGap, top, left + buttonW * 2 + fileGap, top + fileH };
	view.saveBmp = RECT{ left + buttonW * 2 + fileGap * 2, top, right, top + fileH };
	if (view.saveBmp.left > right)
	{
		view.saveBmp.left = right;
	}
	const int contentLeft = left;
	int contentTop = top + fileH + kSidePad;
	if (contentTop > bottom)
	{
		contentTop = bottom;
	}
	const int contentRight = right;
	const int contentBottom = bottom;
	const int rowW = gApp.rulers ? kSideRowRuler : 0;
	const int colH = gApp.rulers ? kSideColRuler : 0;
	const int bar = ScrollBarThickness();
	const int contentW = gApp.sheet.Width() * gApp.monoW;
	const int contentH = gApp.sheet.Height() * gApp.monoH;

	// 先按没有滑条的窗口算。装不下再把对应的边让给滑条，两条会互相挤窄。
	int textW = contentRight - (contentLeft + rowW);
	int textH = contentBottom - (contentTop + colH);
	if (textW < 0)
	{
		textW = 0;
	}
	if (textH < 0)
	{
		textH = 0;
	}
	if (contentW > textW)
	{
		view.showH = true;
		textH = contentBottom - bar - (contentTop + colH);
		if (textH < 0)
		{
			textH = 0;
		}
	}
	if (contentH > textH)
	{
		view.showV = true;
		textW = contentRight - bar - (contentLeft + rowW);
		if (textW < 0)
		{
			textW = 0;
		}
	}
	if (!view.showH && contentW > textW)
	{
		view.showH = true;
	}

	const int textLeft = contentLeft + rowW;
	const int textTop = contentTop + colH;
	int textRight = contentRight - (view.showV ? bar : 0);
	int textBottom = contentBottom - (view.showH ? bar : 0);
	if (textRight < textLeft)
	{
		textRight = textLeft;
	}
	if (textBottom < textTop)
	{
		textBottom = textTop;
	}
	view.text = RECT{ textLeft, textTop, textRight, textBottom };
	if (gApp.rulers)
	{
		view.colRuler = RECT{ textLeft, contentTop, textRight, textTop };
		view.rowRuler = RECT{ contentLeft, textTop, textLeft, textBottom };
	}
	if (view.showH)
	{
		view.hScroll = RECT{ contentLeft, textBottom, textRight, contentBottom > textBottom ? contentBottom : textBottom };
	}
	if (view.showV)
	{
		view.vScroll = RECT{ textRight, contentTop, contentRight > textRight ? contentRight : textRight, textBottom };
	}
	return view;
}

// 把右侧滚动收进文字范围。side 是右栏。文字比窗口小的时候停在开头。
void ClampSideScroll(const RECT& side)
{
	const SideView view = MakeSideView(side);
	const int viewW = view.text.right - view.text.left;
	const int viewH = view.text.bottom - view.text.top;
	const int contentW = gApp.sheet.Width() * gApp.monoW;
	const int contentH = gApp.sheet.Height() * gApp.monoH;
	int maxX = contentW - viewW;
	int maxY = contentH - viewH;
	if (maxX < 0)
	{
		maxX = 0;
	}
	if (maxY < 0)
	{
		maxY = 0;
	}
	if (gApp.sideX < 0)
	{
		gApp.sideX = 0;
	}
	if (gApp.sideY < 0)
	{
		gApp.sideY = 0;
	}
	if (gApp.sideX > maxX)
	{
		gApp.sideX = maxX;
	}
	if (gApp.sideY > maxY)
	{
		gApp.sideY = maxY;
	}
}

// 把一条滑条挪到 rect。show 为假时藏起来。位置没变就不动，避免反复重画。
void PlaceScrollBar(HWND bar, const RECT& rect, bool show)
{
	if (bar == nullptr)
	{
		return;
	}
	if (!show || rect.right <= rect.left || rect.bottom <= rect.top)
	{
		if (IsWindowVisible(bar))
		{
			ShowWindow(bar, SW_HIDE);
		}
		return;
	}
	RECT now{};
	GetWindowRect(bar, &now);
	MapWindowPoints(HWND_DESKTOP, GetParent(bar), reinterpret_cast<POINT*>(&now), 2);
	const int width = rect.right - rect.left;
	const int height = rect.bottom - rect.top;
	const bool same = now.left == rect.left && now.top == rect.top
		&& now.right - now.left == width && now.bottom - now.top == height;
	if (!same)
	{
		MoveWindow(bar, rect.left, rect.top, width, height, TRUE);
	}
	if (!IsWindowVisible(bar))
	{
		ShowWindow(bar, SW_SHOW);
	}
}

// 把滑条的范围和滑块对准当前滚过的像素。bar 是滑条。pos 是已滚像素。
// content 是文字总像素。page 是窗口里看得到的像素。
void SyncScrollBar(HWND bar, int pos, int content, int page)
{
	if (bar == nullptr || !IsWindowVisible(bar))
	{
		return;
	}
	if (page < 1)
	{
		page = 1;
	}
	if (content < page)
	{
		content = page;
	}
	if (pos < 0)
	{
		pos = 0;
	}
	const int maxPos = content - page;
	if (pos > maxPos)
	{
		pos = maxPos;
	}
	SCROLLINFO info{};
	info.cbSize = sizeof(info);
	info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
	info.nMin = 0;
	info.nMax = content - 1;
	info.nPage = static_cast<UINT>(page);
	info.nPos = pos;
	SetScrollInfo(bar, SB_CTL, &info, TRUE);
}

// 读出当前客户区的四块。定义在后面。client 写出客户区矩形。
Layout CurrentLayout(HWND hwnd, RECT& client);

// 按当前右栏摆好两条滑条，并让滑块对准已滚过的位置。
void PlaceSideScroll(HWND hwnd)
{
	RECT client{};
	const Layout layout = CurrentLayout(hwnd, client);
	ClampSideScroll(layout.side);
	const SideView view = MakeSideView(layout.side);
	PlaceScrollBar(gApp.sideHScroll, view.hScroll, view.showH);
	PlaceScrollBar(gApp.sideVScroll, view.vScroll, view.showV);
	const int pageW = view.text.right - view.text.left;
	const int pageH = view.text.bottom - view.text.top;
	SyncScrollBar(gApp.sideHScroll, gApp.sideX, gApp.sheet.Width() * gApp.monoW, pageW);
	SyncScrollBar(gApp.sideVScroll, gApp.sideY, gApp.sheet.Height() * gApp.monoH, pageH);
}

// 在左栏里排三组按钮。tool 是左栏。上面是工具，中间是撤销、恢复、清空，下面是标尺和归位。
ToolBar MakeToolBar(const RECT& tool)
{
	// 左栏只剩约 100 像素，四周少留一点，两个字才放得下。
	const int margin = 8;
	const int gap = 10;
	// 组和组之间的空隙。和按钮之间的 10 像素不是同一个数。
	const int groupGap = 24;
	const int buttonCount = 8;
	const int chrome = margin * 2 + gap * 5 + groupGap * 2;
	int room = (tool.bottom - tool.top) - chrome;
	int buttonH = 40;
	if (room < buttonH * buttonCount)
	{
		buttonH = room / buttonCount;
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
	y += groupGap - gap;
	bar.ruler = RECT{ left, y, right, y + buttonH };
	y += buttonH + gap;
	bar.home = RECT{ left, y, right, y + buttonH };
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
// 只管缩放、拖动、归位，以及让原点停在屏幕原处。不改格子里的 0 和 1。
////////////////////////////////////////////////////////////////////////////////

// 格子区域还原成含标尺的整块中间区域。cells 是 CellArea 算出来的矩形。
// 标尺关掉时，格子区域就是整块中间区域。
RECT BoardOfCells(const RECT& cells)
{
	RECT board = cells;
	board.left -= RulerShiftX();
	board.top -= RulerShiftY();
	return board;
}

// 把整张图摆进整块中间区域的正中。board 含标尺。zoom 保持调用前的值。
// origin 从格子区域的左上角量起，所以要减掉标尺占掉的宽高。
void CenterSheet(const RECT& board)
{
	const int boardW = board.right - board.left;
	const int boardH = board.bottom - board.top;
	const int shiftX = RulerShiftX();
	const int shiftY = RulerShiftY();
	gApp.originX = (boardW - gApp.sheet.Width() * gApp.zoom) / 2 - shiftX;
	gApp.originY = (boardH - gApp.sheet.Height() * gApp.zoom) / 2 - shiftY;
}

// 按打开时的 16×16 把原点摆回整块中间区域的正中。board 含标尺。zoom 保持调用前的值。
// 后来加宽加高的格子挂在这张 16×16 外面，不把整张新图重新居中。
void PlaceHome(const RECT& board)
{
	const int boardW = board.right - board.left;
	const int boardH = board.bottom - board.top;
	const int homeX = (boardW - kHomeSide * gApp.zoom) / 2 - RulerShiftX();
	const int homeY = (boardH - kHomeSide * gApp.zoom) / 2 - RulerShiftY();
	gApp.originX = homeX - gApp.sheet.Left() * gApp.zoom;
	gApp.originY = homeY - gApp.sheet.Up() * gApp.zoom;
}

// 第一次有足够大的棋盘时，把整张图放进整块中间区域的正中。board 含标尺。
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
	gApp.shownPercent = gApp.zoomBase > 0 ? zoom * 100 / gApp.zoomBase : 100;
	CenterSheet(board);
	gApp.placed = true;
}

// 把缩放改到 zoom，并让光标下的格子留在原地。zoom 是一格的像素边长。
void SetZoomAt(POINT cursor, const RECT& board, int zoom)
{
	if (!Contains(board, cursor))
	{
		return;
	}
	PlaceView(BoardOfCells(board));
	if (zoom < kZoomMin)
	{
		zoom = kZoomMin;
	}
	if (zoom > kZoomMax)
	{
		zoom = kZoomMax;
	}
	const int oldZoom = gApp.zoom;
	if (zoom == oldZoom || oldZoom <= 0)
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

// 以光标所在的位置为中心改缩放。notches 为正是放大，一格像素一档。改完把百分数按像素算回显示。
void ZoomAt(POINT cursor, const RECT& board, int notches)
{
	if (notches == 0 || !Contains(board, cursor))
	{
		return;
	}
	PlaceView(BoardOfCells(board));
	int zoom = gApp.zoom + notches;
	if (zoom < kZoomMin)
	{
		zoom = kZoomMin;
	}
	if (zoom > kZoomMax)
	{
		zoom = kZoomMax;
	}
	if (zoom == gApp.zoom)
	{
		return;
	}
	SetZoomAt(cursor, board, zoom);
	if (gApp.zoomBase > 0)
	{
		gApp.shownPercent = gApp.zoom * 100 / gApp.zoomBase;
	}
}

// 把百分数收到整十。dir 为 1 是放大，为 -1 是缩小。已经是整十就再走 10。
int SnapTen(int percent, int dir)
{
	if (percent % 10 == 0)
	{
		return percent + dir * 10;
	}
	if (dir > 0)
	{
		return (percent / 10 + 1) * 10;
	}
	return (percent / 10) * 10;
}

// 百分数换成格子像素，四舍五入，并限制在 1 到 64。
int ZoomFromPercent(int percent)
{
	if (gApp.zoomBase <= 0)
	{
		return gApp.zoom;
	}
	int zoom = (percent * gApp.zoomBase + 50) / 100;
	if (zoom < kZoomMin)
	{
		zoom = kZoomMin;
	}
	if (zoom > kZoomMax)
	{
		zoom = kZoomMax;
	}
	return zoom;
}

// 这一档整十还能不能再走。dir 为 1 是放大，为 -1 是缩小。
bool CanZoomStep(int dir)
{
	const int sign = dir > 0 ? 1 : -1;
	const int next = SnapTen(gApp.shownPercent, sign);
	if (next <= 0)
	{
		return false;
	}
	const int zoom = ZoomFromPercent(next);
	if (sign > 0 && gApp.zoom >= kZoomMax && zoom >= kZoomMax)
	{
		return false;
	}
	if (sign < 0 && gApp.zoom <= kZoomMin && zoom <= kZoomMin)
	{
		return false;
	}
	return true;
}

// 以棋盘中心为基准，把显示百分数收到下一档整十。dir 为 1 是放大，为 -1 是缩小。
void ZoomStep(HWND hwnd, int dir)
{
	if (!CanZoomStep(dir))
	{
		return;
	}
	const int sign = dir > 0 ? 1 : -1;
	const int next = SnapTen(gApp.shownPercent, sign);
	RECT client{};
	const Layout layout = CurrentLayout(hwnd, client);
	const RECT cells = CellArea(layout.board);
	POINT center{
		(layout.board.left + layout.board.right) / 2,
		(layout.board.top + layout.board.bottom) / 2
	};
	if (center.x < cells.left)
	{
		center.x = cells.left;
	}
	if (center.y < cells.top)
	{
		center.y = cells.top;
	}
	if (center.x >= cells.right && cells.right > cells.left)
	{
		center.x = cells.right - 1;
	}
	if (center.y >= cells.bottom && cells.bottom > cells.top)
	{
		center.y = cells.bottom - 1;
	}
	SetZoomAt(center, cells, ZoomFromPercent(next));
	gApp.shownPercent = next;
	InvalidateRect(hwnd, nullptr, FALSE);
}

// 顶栏要画的百分数。这是显示值，不是从格子像素反推的值。
int ZoomPercent()
{
	return gApp.shownPercent;
}

// 打开或关掉棋盘和右侧的标尺。画面上的格子留在原来的屏幕位置。
void ToggleRulers(HWND hwnd)
{
	gApp.rulers = !gApp.rulers;
	if (gApp.rulers)
	{
		gApp.originX -= kRowRuler;
		gApp.originY -= kColRuler;
	}
	else
	{
		gApp.originX += kRowRuler;
		gApp.originY += kColRuler;
	}
	PlaceSideScroll(hwnd);
	InvalidateRect(hwnd, nullptr, FALSE);
}

// 正在落笔或拖动。定义在撤销一节。归位时用来避开半笔。
bool ViewBusy();

// 归位。百分数回到 100%，原点按打开时的 16×16 回到棋盘正中。正在落笔或拖动时不动。
void ResetView(HWND hwnd)
{
	if (ViewBusy())
	{
		return;
	}
	RECT client{};
	const Layout layout = CurrentLayout(hwnd, client);
	const int boardW = layout.board.right - layout.board.left;
	const int boardH = layout.board.bottom - layout.board.top;
	if (boardW < 16 || boardH < 16)
	{
		return;
	}
	gApp.zoom = ZoomFromPercent(100);
	gApp.shownPercent = 100;
	gApp.placed = true;
	PlaceHome(layout.board);
	InvalidateRect(hwnd, nullptr, FALSE);
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

// 客户区坐标换成格子。点不在格子区域里，或格子不在图内，则返回 false。
// cursor 是客户区坐标。cells 是格子区域，不含标尺。x、y 写出缓冲区下标。
bool CellAt(POINT cursor, const RECT& cells, int& x, int& y)
{
	if (gApp.zoom <= 0 || !Contains(cells, cursor))
	{
		return false;
	}
	const int localX = cursor.x - cells.left - gApp.originX;
	const int localY = cursor.y - cells.top - gApp.originY;
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

// 缓冲区列号换成原点坐标。bufferX 从画面左上角数起，0 是最左边那一列。原点那列是 0。
int ColOf(int bufferX)
{
	return bufferX - gApp.sheet.Left();
}

// 缓冲区行号换成原点坐标。bufferY 从画面左上角数起，0 是最上边那一行。原点那行是 0。
int RowOf(int bufferY)
{
	return bufferY - gApp.sheet.Up();
}

// 标尺上要写的列号。原点那一列显示 1。格子数据仍从 0 数。
int MarkCol(int bufferX)
{
	return ColOf(bufferX) + 1;
}

// 标尺上要写的行号。原点那一行显示 1。格子数据仍从 0 数。
int MarkRow(int bufferY)
{
	return RowOf(bufferY) + 1;
}

// 右侧 01 上的点换成格子。点不在文字里，或格子不在图内，则返回 false。
// cursor 是客户区坐标。side 是右栏。x、y 写出缓冲区下标。
bool SideCellAt(POINT cursor, const RECT& side, int& x, int& y)
{
	if (gApp.monoW <= 0 || gApp.monoH <= 0)
	{
		return false;
	}
	const SideView view = MakeSideView(side);
	if (!Contains(view.text, cursor))
	{
		return false;
	}
	const int cellX = (cursor.x - view.text.left + gApp.sideX) / gApp.monoW;
	const int cellY = (cursor.y - view.text.top + gApp.sideY) / gApp.monoH;
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
// 撤销、恢复、清空
// 改图前压一份旧图。撤销和恢复在两座栈之间移动。栈空，或图已经全灭，都不改画布。
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

// 这颗命令现在点下去会不会改画布。command 是撤销、恢复或清空。
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

// 恢复一步。栈空或视图正忙时画布不动。成功后四个数字框写回这一步各方向的格数。
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

// 数字框的消息。回车执行应用。Ctrl+Z、Ctrl+Y 仍交给主窗口撤销和恢复。
// Ctrl+Tab 交给主窗口开关标尺。1、2、3、-、+、= 留在框里填格数，不切工具，也不缩放。
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
		if (ctrl && (wParam == 'Z' || wParam == 'Y' || wParam == VK_TAB))
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
	gApp.monoFont = CreateFontW(
		-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
		FIXED_PITCH | FF_MODERN, L"Consolas");
	if (gApp.monoFont != nullptr)
	{
		const HDC dc = GetDC(hwnd);
		const HGDIOBJ old = SelectObject(dc, gApp.monoFont);
		SIZE size{};
		GetTextExtentPoint32W(dc, L"0", 1, &size);
		SelectObject(dc, old);
		ReleaseDC(hwnd, dc);
		if (size.cx > 0)
		{
			gApp.monoW = size.cx;
		}
		if (size.cy > 0)
		{
			gApp.monoH = size.cy;
		}
	}
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
	gApp.sideHScroll = CreateWindowExW(
		0, L"SCROLLBAR", L"", WS_CHILD | SBS_HORZ,
		0, 0, 0, 0, hwnd, nullptr, instance, nullptr);
	gApp.sideVScroll = CreateWindowExW(
		0, L"SCROLLBAR", L"", WS_CHILD | SBS_VERT,
		0, 0, 0, 0, hwnd, nullptr, instance, nullptr);
	PlaceSideScroll(hwnd);
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
	if (gApp.monoFont != nullptr)
	{
		DeleteObject(gApp.monoFont);
		gApp.monoFont = nullptr;
	}
}

////////////////////////////////////////////////////////////////////////////////
// 打开和保存
// 右栏三颗按钮。01 文本一行一排，只写 0 和 1。BMP 是无压缩 24 位，一格一个像素。
////////////////////////////////////////////////////////////////////////////////

// 从 01 文本收出来的网格。width、height 是格数。cells 按行排，0 是灭，1 是亮。
struct TextGrid
{
	int width{ 0 };
	int height{ 0 };
	std::vector<std::uint8_t> cells;
};

// 读出整个文件。path 是完整路径。bytes 写出字节。空文件、太大或读失败则返回 false。
bool ReadWholeFile(const wchar_t* path, std::vector<std::uint8_t>& bytes)
{
	bytes.clear();
	// file 是只读句柄。读完就关。
	const HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (file == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	LARGE_INTEGER size{};
	// 1024 行，每行 1024 个字再加回车换行。比这更大的文件不必再读。
	const LONGLONG limit = static_cast<LONGLONG>(IconSheet::kMaxSide) * (IconSheet::kMaxSide + 2);
	if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0 || size.QuadPart > limit)
	{
		CloseHandle(file);
		return false;
	}
	bytes.resize(static_cast<std::size_t>(size.QuadPart));
	DWORD read = 0;
	const BOOL ok = ReadFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &read, nullptr);
	CloseHandle(file);
	return ok && read == bytes.size();
}

// 把字节写进文件。path 是完整路径。已有的文件会被盖掉。
bool WriteWholeFile(const wchar_t* path, const void* data, std::size_t size)
{
	const HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (file == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	const auto* bytes = static_cast<const std::uint8_t*>(data);
	std::size_t left = size;
	while (left > 0)
	{
		const DWORD chunk = left > (1u << 20) ? (1u << 20) : static_cast<DWORD>(left);
		DWORD wrote = 0;
		if (!WriteFile(file, bytes, chunk, &wrote, nullptr) || wrote != chunk)
		{
			CloseHandle(file);
			return false;
		}
		bytes += chunk;
		left -= chunk;
	}
	CloseHandle(file);
	return true;
}

// 把文件字节收成网格。bytes 是整个文件。
// 行末可以是换行，也可以是回车加换行。文件末尾那一个换行不算空行。
// 每一行要一样长，只能是 0 和 1，宽高在 1 到 1024。不合规则时 grid 不动。
bool Parse01Text(const std::vector<std::uint8_t>& bytes, TextGrid& grid)
{
	std::vector<std::uint8_t> cells;
	cells.reserve(bytes.size());
	// width 是第一行的格数。后面每一行都要等于它。-1 表示还没遇到行尾。
	int width = -1;
	int row = 0;
	int col = 0;
	for (std::size_t i = 0; i < bytes.size(); ++i)
	{
		const unsigned char ch = bytes[i];
		if (ch == '\r')
		{
			if (i + 1 >= bytes.size() || bytes[i + 1] != '\n')
			{
				return false;
			}
			continue;
		}
		if (ch == '\n')
		{
			if (width < 0)
			{
				width = col;
			}
			if (col != width || width < 1)
			{
				return false;
			}
			++row;
			if (row > IconSheet::kMaxSide)
			{
				return false;
			}
			col = 0;
			continue;
		}
		if (ch != '0' && ch != '1')
		{
			return false;
		}
		if (col >= IconSheet::kMaxSide)
		{
			return false;
		}
		cells.push_back(ch == '1' ? 1 : 0);
		++col;
	}
	if (col > 0)
	{
		if (width < 0)
		{
			width = col;
		}
		if (col != width || width < 1)
		{
			return false;
		}
		++row;
	}
	if (row < 1 || width < 1 || width > IconSheet::kMaxSide || row > IconSheet::kMaxSide)
	{
		return false;
	}
	if (cells.size() != static_cast<std::size_t>(width) * static_cast<std::size_t>(row))
	{
		return false;
	}
	grid.width = width;
	grid.height = row;
	grid.cells = std::move(cells);
	return true;
}

// 核对 01 文本的行规。失败返回 false，程序不出窗口。
bool TextSelfCheck()
{
	TextGrid grid;
	const std::vector<std::uint8_t> rows{ '0', '1', '\r', '\n', '1', '0', '\n' };
	if (!Parse01Text(rows, grid) || grid.width != 2 || grid.height != 2 || grid.cells.size() != 4)
	{
		return false;
	}
	if (grid.cells[0] != 0 || grid.cells[1] != 1 || grid.cells[2] != 1 || grid.cells[3] != 0)
	{
		return false;
	}
	const std::vector<std::uint8_t> other{ '0', '2' };
	const std::vector<std::uint8_t> uneven{ '0', '1', '\n', '0' };
	const std::vector<std::uint8_t> blank{ '0', '\n', '\n' };
	if (Parse01Text(other, grid) || Parse01Text(uneven, grid) || Parse01Text(blank, grid))
	{
		return false;
	}
	return grid.width == 2 && grid.height == 2;
}

// 弹出系统文件框。filter 是类型列表，title 是标题，extension 是省略后缀时补上的扩展名。
// save 为真时是保存。path 里已有的路径当作起点，确定后改成选中的完整路径。取消则返回 false。
bool PickFile(HWND hwnd, bool save, const wchar_t* filter, const wchar_t* title, const wchar_t* extension, wchar_t* path, int pathCount)
{
	OPENFILENAMEW dialog{};
	dialog.lStructSize = sizeof(dialog);
	dialog.hwndOwner = hwnd;
	dialog.lpstrFilter = filter;
	dialog.lpstrFile = path;
	dialog.nMaxFile = static_cast<DWORD>(pathCount);
	dialog.lpstrTitle = title;
	dialog.lpstrDefExt = extension;
	dialog.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
	if (save)
	{
		dialog.Flags |= OFN_OVERWRITEPROMPT;
	}
	else
	{
		dialog.Flags |= OFN_FILEMUSTEXIST;
	}
	const BOOL ok = save ? GetSaveFileNameW(&dialog) : GetOpenFileNameW(&dialog);
	if (ok)
	{
		return true;
	}
	if (CommDlgExtendedError() != 0)
	{
		MessageBoxW(hwnd, L"文件框打不开。", L"IconDrawer", MB_OK | MB_ICONWARNING);
	}
	return false;
}

// 把当前图写成 01 文本。path 是完整路径。一行一排，行末是回车换行。
bool Write01Text(const wchar_t* path)
{
	const int width = gApp.sheet.Width();
	const int height = gApp.sheet.Height();
	// bytes 是要写出去的字符。每个格子一个 0 或 1。
	std::string bytes;
	bytes.reserve(static_cast<std::size_t>(height) * static_cast<std::size_t>(width + 2));
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			bytes.push_back(gApp.sheet.Get(x, y) ? '1' : '0');
		}
		bytes.push_back('\r');
		bytes.push_back('\n');
	}
	return WriteWholeFile(path, bytes.data(), bytes.size());
}

// 打开一份 01 文本并换成当前图。正在落笔或拖动时不动。不合规则时保持原图。
void OpenText(HWND hwnd)
{
	if (ViewBusy())
	{
		return;
	}
	// path 是文件框里的完整路径。先放上次用过的，确定后再记住。
	wchar_t path[MAX_PATH]{};
	lstrcpynW(path, gApp.textPath, MAX_PATH);
	if (!PickFile(hwnd, false, L"01 文本 (*.txt)\0*.txt\0", L"打开 01 文本", L"txt", path, MAX_PATH))
	{
		return;
	}
	std::vector<std::uint8_t> bytes;
	TextGrid grid;
	if (!ReadWholeFile(path, bytes) || !Parse01Text(bytes, grid))
	{
		MessageBoxW(
			hwnd,
			L"这份 01 文本不能打开。每一行要一样长，只能有 0 和 1，宽和高要在 1 到 1024。",
			L"IconDrawer",
			MB_OK | MB_ICONWARNING);
		return;
	}
	const int beforeLeft = gApp.sheet.Left();
	const int beforeUp = gApp.sheet.Up();
	if (!gApp.sheet.LoadGrid(grid.width, grid.height, std::move(grid.cells)))
	{
		MessageBoxW(hwnd, L"这份 01 文本不能打开。", L"IconDrawer", MB_OK | MB_ICONWARNING);
		return;
	}
	lstrcpynW(gApp.textPath, path, MAX_PATH);
	HoldOrigin(beforeLeft, beforeUp);
	SyncExpandFields();
	PlaceSideScroll(hwnd);
	InvalidateRect(hwnd, nullptr, FALSE);
}

// 把当前图保存成 01 文本。取消或写失败时图不动。
void SaveText(HWND hwnd)
{
	wchar_t path[MAX_PATH]{};
	lstrcpynW(path, gApp.textPath, MAX_PATH);
	if (!PickFile(hwnd, true, L"01 文本 (*.txt)\0*.txt\0", L"保存 01 文本", L"txt", path, MAX_PATH))
	{
		return;
	}
	if (!Write01Text(path))
	{
		MessageBoxW(hwnd, L"这份 01 文本写不出去。", L"IconDrawer", MB_OK | MB_ICONWARNING);
		return;
	}
	lstrcpynW(gApp.textPath, path, MAX_PATH);
}

// 把一张图收成无压缩 24 位 BMP。sheet 是要写的图。一格一个像素，0 是黑，1 是白。
// 不看屏幕上放大后的格子。宽或高不合法时返回空。
std::vector<std::uint8_t> BuildBmp(const IconSheet& sheet)
{
	const int width = sheet.Width();
	const int height = sheet.Height();
	if (width < 1 || height < 1)
	{
		return {};
	}
	// stride 是文件里一行的字节数。24 位一行要补到 4 的倍数，补出来的字节保持 0。
	const int stride = (width * 3 + 3) / 4 * 4;
	const std::size_t headerSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	const std::size_t imageSize = static_cast<std::size_t>(stride) * static_cast<std::size_t>(height);
	std::vector<std::uint8_t> bytes(headerSize + imageSize, 0);

	BITMAPFILEHEADER file{};
	file.bfType = 0x4D42;
	file.bfOffBits = static_cast<DWORD>(headerSize);
	file.bfSize = static_cast<DWORD>(bytes.size());
	BITMAPINFOHEADER info{};
	info.biSize = sizeof(info);
	info.biWidth = width;
	info.biHeight = height;
	info.biPlanes = 1;
	info.biBitCount = 24;
	info.biCompression = BI_RGB;
	info.biSizeImage = static_cast<DWORD>(imageSize);
	std::memcpy(bytes.data(), &file, sizeof(file));
	std::memcpy(bytes.data() + sizeof(file), &info, sizeof(info));

	// pixels 是像素区的开头。BMP 从画面最下边那一行开始存。
	std::uint8_t* pixels = bytes.data() + headerSize;
	for (int y = 0; y < height; ++y)
	{
		std::uint8_t* row = pixels + static_cast<std::size_t>(height - 1 - y) * static_cast<std::size_t>(stride);
		for (int x = 0; x < width; ++x)
		{
			const std::uint8_t value = sheet.Get(x, y) ? 255 : 0;
			row[static_cast<std::size_t>(x) * 3] = value;
			row[static_cast<std::size_t>(x) * 3 + 1] = value;
			row[static_cast<std::size_t>(x) * 3 + 2] = value;
		}
	}
	return bytes;
}

// 核对黑白 BMP 的尺寸、黑白和行尾补齐。失败返回 false，程序不出窗口。
bool BmpSelfCheck()
{
	IconSheet sheet;
	sheet.Set(0, 0, true);
	sheet.Set(1, 15, true);
	const std::vector<std::uint8_t> bytes = BuildBmp(sheet);
	const int stride = (16 * 3 + 3) / 4 * 4;
	const std::size_t headerSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	if (bytes.size() != headerSize + static_cast<std::size_t>(stride) * 16 || bytes[0] != 'B' || bytes[1] != 'M')
	{
		return false;
	}
	// top 是画面最上边那一行，在文件像素区的最后一行。
	const std::uint8_t* top = bytes.data() + headerSize + static_cast<std::size_t>(15) * static_cast<std::size_t>(stride);
	if (top[0] != 255 || top[1] != 255 || top[2] != 255 || top[3] != 0)
	{
		return false;
	}
	// bottom 是画面最下边那一行，在文件像素区的第一行。第 2 格是白。
	const std::uint8_t* bottom = bytes.data() + headerSize;
	if (bottom[0] != 0 || bottom[3] != 255 || bottom[4] != 255 || bottom[5] != 255)
	{
		return false;
	}

	IconSheet thin;
	if (!thin.LoadGrid(1, 1, std::vector<std::uint8_t>{ 1 }))
	{
		return false;
	}
	const std::vector<std::uint8_t> one = BuildBmp(thin);
	if (one.size() != headerSize + 4)
	{
		return false;
	}
	return one[headerSize] == 255 && one[headerSize + 1] == 255 && one[headerSize + 2] == 255 && one[headerSize + 3] == 0;
}

// 把当前图保存成黑白 BMP。取消或写失败时图不动。像素尺寸等于格子宽高。
void SaveBmp(HWND hwnd)
{
	// path 是文件框里的完整路径。先放上次用过的，确定后再记住。
	wchar_t path[MAX_PATH]{};
	lstrcpynW(path, gApp.bmpPath, MAX_PATH);
	if (!PickFile(hwnd, true, L"黑白 BMP (*.bmp)\0*.bmp\0", L"保存黑白 BMP", L"bmp", path, MAX_PATH))
	{
		return;
	}
	const std::vector<std::uint8_t> bytes = BuildBmp(gApp.sheet);
	if (bytes.empty() || !WriteWholeFile(path, bytes.data(), bytes.size()))
	{
		MessageBoxW(hwnd, L"这份图片写不出去。", L"IconDrawer", MB_OK | MB_ICONWARNING);
		return;
	}
	lstrcpynW(gApp.bmpPath, path, MAX_PATH);
}

////////////////////////////////////////////////////////////////////////////////
// 画面
// 把四块区域、棋盘、标尺、右侧 01、左栏按钮、顶栏扩像素和放大百分比画出来。
////////////////////////////////////////////////////////////////////////////////

// 这一格的标尺数字要不要写出来。shown 是给人看的数字，原点那格是 1。
// stride 是隔几格标一次。4 的倍数会标成 4、8、12、16。原点那格和鼠标下那一格总是标。
bool WantRulerLabel(int shown, int stride, bool hover)
{
	if (hover || stride <= 1 || shown == 1)
	{
		return true;
	}
	return shown % stride == 0;
}

// 隔多少格标一次，才不会叠在一起。cellPx 是一格占的像素。
int RulerStride(int cellPx)
{
	int stride = 1;
	while (stride < 1024 && stride * cellPx < 26)
	{
		stride *= 2;
	}
	return stride;
}

// 标尺上一个数的颜色。shown 是给人看的数字。原点那格是 1，用浅黄。hover 为真时用最亮的字。
COLORREF RulerColor(int shown, bool hover)
{
	if (hover)
	{
		return RGB(255, 255, 255);
	}
	if (shown == 1)
	{
		return RGB(230, 200, 110);
	}
	return RGB(150, 150, 150);
}

// 画棋盘左边和上边的标尺。board 是整块棋盘，含标尺。cells 是格子区域。
void PaintBoardRuler(HDC dc, const RECT& board, const RECT& cells)
{
	const RECT colRuler{ cells.left, board.top, board.right, cells.top };
	const RECT rowRuler{ board.left, cells.top, cells.left, board.bottom };
	Fill(dc, colRuler, RGB(32, 32, 36));
	Fill(dc, rowRuler, RGB(32, 32, 36));
	Fill(dc, RECT{ board.left, board.top, cells.left, cells.top }, RGB(32, 32, 36));
	if (gApp.zoom <= 0 || gApp.monoFont == nullptr)
	{
		return;
	}

	const int stride = RulerStride(gApp.zoom);
	const HGDIOBJ old = SelectObject(dc, gApp.monoFont);
	SetBkMode(dc, TRANSPARENT);
	const int boardW = cells.right - cells.left;
	const int boardH = cells.bottom - cells.top;
	int x0 = (-gApp.originX) / gApp.zoom;
	int y0 = (-gApp.originY) / gApp.zoom;
	int x1 = (boardW - gApp.originX + gApp.zoom - 1) / gApp.zoom;
	int y1 = (boardH - gApp.originY + gApp.zoom - 1) / gApp.zoom;
	if (x0 < 0) { x0 = 0; }
	if (y0 < 0) { y0 = 0; }
	if (x1 > gApp.sheet.Width()) { x1 = gApp.sheet.Width(); }
	if (y1 > gApp.sheet.Height()) { y1 = gApp.sheet.Height(); }

	const int colClip = SaveDC(dc);
	IntersectClipRect(dc, colRuler.left, colRuler.top, colRuler.right, colRuler.bottom);
	int lastRight = -100000;
	for (int pass = 0; pass < 2; ++pass)
	{
		for (int x = x0; x < x1; ++x)
		{
			const int coord = MarkCol(x);
			const bool hot = gApp.hover && x == gApp.hoverX;
			if (pass == 0 && hot)
			{
				continue;
			}
			if (pass == 1 && !hot)
			{
				continue;
			}
			if (!WantRulerLabel(coord, stride, hot))
			{
				continue;
			}
			wchar_t text[16]{};
			wsprintfW(text, L"%d", coord);
			const int half = lstrlenW(text) * ((gApp.monoW + 1) / 2);
			const int center = cells.left + gApp.originX + x * gApp.zoom + gApp.zoom / 2;
			RECT box{ center - half, colRuler.top, center + half, colRuler.bottom };
			if (pass == 0 && coord != 1 && box.left < lastRight)
			{
				continue;
			}
			SetTextColor(dc, RulerColor(coord, hot));
			DrawTextW(dc, text, -1, &box, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
			if (pass == 0 && box.right > lastRight)
			{
				lastRight = box.right;
			}
		}
	}
	RestoreDC(dc, colClip);

	const int rowClip = SaveDC(dc);
	IntersectClipRect(dc, rowRuler.left, rowRuler.top, rowRuler.right, rowRuler.bottom);
	int lastBottom = -100000;
	for (int pass = 0; pass < 2; ++pass)
	{
		for (int y = y0; y < y1; ++y)
		{
			const int coord = MarkRow(y);
			const bool hot = gApp.hover && y == gApp.hoverY;
			if (pass == 0 && hot)
			{
				continue;
			}
			if (pass == 1 && !hot)
			{
				continue;
			}
			if (!WantRulerLabel(coord, stride, hot))
			{
				continue;
			}
			wchar_t text[16]{};
			wsprintfW(text, L"%d", coord);
			const int center = cells.top + gApp.originY + y * gApp.zoom + gApp.zoom / 2;
			RECT box{ rowRuler.left, center - 10, rowRuler.right - 2, center + 10 };
			if (pass == 0 && coord != 1 && box.top < lastBottom)
			{
				continue;
			}
			SetTextColor(dc, RulerColor(coord, hot));
			DrawTextW(dc, text, -1, &box, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
			if (pass == 0 && box.bottom > lastBottom)
			{
				lastBottom = box.bottom;
			}
		}
	}
	RestoreDC(dc, rowClip);
	SelectObject(dc, old);
}

// 画中央棋盘和它的标尺。board 是棋盘在客户区里的矩形，含标尺。
void PaintBoard(HDC dc, const RECT& board)
{
	const RECT cells = CellArea(board);
	const int boardW = cells.right - cells.left;
	const int boardH = cells.bottom - cells.top;
	if (gApp.rulers)
	{
		PaintBoardRuler(dc, board, cells);
	}
	if (boardW <= 0 || boardH <= 0 || gApp.zoom <= 0)
	{
		return;
	}
	const int saved = SaveDC(dc);
	IntersectClipRect(dc, cells.left, cells.top, cells.right, cells.bottom);

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
				cells.left + gApp.originX + x * gApp.zoom,
				cells.top + gApp.originY + y * gApp.zoom,
				cells.left + gApp.originX + (x + 1) * gApp.zoom,
				cells.top + gApp.originY + (y + 1) * gApp.zoom
			};
			const COLORREF color = gApp.sheet.Get(x, y) ? RGB(230, 230, 230) : RGB(46, 46, 46);
			Fill(dc, cell, color);
		}
	}

	if (gApp.zoom >= 3)
	{
		HPEN pen = CreatePen(PS_SOLID, 1, RGB(96, 96, 96));
		HGDIOBJ old = SelectObject(dc, pen);
		const int left = cells.left + gApp.originX;
		const int top = cells.top + gApp.originY;
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
	if (gApp.hover)
	{
		RECT cell{
			cells.left + gApp.originX + gApp.hoverX * gApp.zoom,
			cells.top + gApp.originY + gApp.hoverY * gApp.zoom,
			cells.left + gApp.originX + (gApp.hoverX + 1) * gApp.zoom,
			cells.top + gApp.originY + (gApp.hoverY + 1) * gApp.zoom
		};
		Frame(dc, cell, RGB(230, 200, 110));
	}
	RestoreDC(dc, saved);
}

// 画右侧的打开、保存文本、保存图片、01 和同一套原点标尺。side 是右栏。文字只显示，不在这里改字。
void PaintSide(HDC dc, const RECT& side)
{
	ClampSideScroll(side);
	const SideView view = MakeSideView(side);
	DrawButton(dc, view.openText, L"打开", false, !ViewBusy());
	DrawButton(dc, view.saveText, L"保存文本", false, true);
	DrawButton(dc, view.saveBmp, L"保存图片", false, true);
	if (gApp.monoFont == nullptr || gApp.monoW <= 0 || gApp.monoH <= 0)
	{
		return;
	}
	if (gApp.rulers)
	{
		Fill(dc, view.colRuler, RGB(32, 32, 36));
		Fill(dc, view.rowRuler, RGB(32, 32, 36));
	}
	const HGDIOBJ old = SelectObject(dc, gApp.monoFont);
	SetBkMode(dc, TRANSPARENT);
	const int stride = RulerStride(gApp.monoW);
	const int viewW = view.text.right - view.text.left;
	const int viewH = view.text.bottom - view.text.top;
	int x0 = gApp.sideX / gApp.monoW;
	int y0 = gApp.sideY / gApp.monoH;
	int x1 = (gApp.sideX + viewW + gApp.monoW - 1) / gApp.monoW;
	int y1 = (gApp.sideY + viewH + gApp.monoH - 1) / gApp.monoH;
	if (x0 < 0) { x0 = 0; }
	if (y0 < 0) { y0 = 0; }
	if (x1 > gApp.sheet.Width()) { x1 = gApp.sheet.Width(); }
	if (y1 > gApp.sheet.Height()) { y1 = gApp.sheet.Height(); }

	const int textClip = SaveDC(dc);
	IntersectClipRect(dc, view.text.left, view.text.top, view.text.right, view.text.bottom);
	std::vector<wchar_t> line(static_cast<std::size_t>(x1 - x0) + 1, 0);
	for (int y = y0; y < y1; ++y)
	{
		const int count = x1 - x0;
		for (int i = 0; i < count; ++i)
		{
			line[static_cast<std::size_t>(i)] = gApp.sheet.Get(x0 + i, y) ? L'1' : L'0';
		}
		const int pixelX = view.text.left + x0 * gApp.monoW - gApp.sideX;
		const int pixelY = view.text.top + y * gApp.monoH - gApp.sideY;
		SetTextColor(dc, RGB(210, 210, 210));
		ExtTextOutW(dc, pixelX, pixelY, ETO_CLIPPED, &view.text, line.data(), count, nullptr);
		if (gApp.hover && y == gApp.hoverY && gApp.hoverX >= x0 && gApp.hoverX < x1)
		{
			RECT mark{
				view.text.left + gApp.hoverX * gApp.monoW - gApp.sideX,
				pixelY,
				view.text.left + (gApp.hoverX + 1) * gApp.monoW - gApp.sideX,
				pixelY + gApp.monoH
			};
			Frame(dc, mark, RGB(230, 200, 110));
		}
	}
	RestoreDC(dc, textClip);

	if (gApp.rulers)
	{
	const int colClip = SaveDC(dc);
	IntersectClipRect(dc, view.colRuler.left, view.colRuler.top, view.colRuler.right, view.colRuler.bottom);
	int lastRight = -100000;
	for (int pass = 0; pass < 2; ++pass)
	{
		for (int x = x0; x < x1; ++x)
		{
			const int coord = MarkCol(x);
			const bool hot = gApp.hover && x == gApp.hoverX;
			if ((pass == 0 && hot) || (pass == 1 && !hot) || !WantRulerLabel(coord, stride, hot))
			{
				continue;
			}
			wchar_t text[16]{};
			wsprintfW(text, L"%d", coord);
			const int half = lstrlenW(text) * ((gApp.monoW + 1) / 2);
			const int center = view.text.left + x * gApp.monoW - gApp.sideX + gApp.monoW / 2;
			RECT box{ center - half, view.colRuler.top, center + half, view.colRuler.bottom };
			if (pass == 0 && coord != 1 && box.left < lastRight)
			{
				continue;
			}
			SetTextColor(dc, RulerColor(coord, hot));
			DrawTextW(dc, text, -1, &box, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
			if (pass == 0 && box.right > lastRight)
			{
				lastRight = box.right;
			}
		}
	}
	RestoreDC(dc, colClip);

	const int rowClip = SaveDC(dc);
	IntersectClipRect(dc, view.rowRuler.left, view.rowRuler.top, view.rowRuler.right, view.rowRuler.bottom);
	int lastBottom = -100000;
	for (int pass = 0; pass < 2; ++pass)
	{
		for (int y = y0; y < y1; ++y)
		{
			const int coord = MarkRow(y);
			const bool hot = gApp.hover && y == gApp.hoverY;
			if ((pass == 0 && hot) || (pass == 1 && !hot) || !WantRulerLabel(coord, stride, hot))
			{
				continue;
			}
			wchar_t text[16]{};
			wsprintfW(text, L"%d", coord);
			const int center = view.text.top + y * gApp.monoH - gApp.sideY + gApp.monoH / 2;
			RECT box{ view.rowRuler.left, center - gApp.monoH / 2, view.rowRuler.right - 2, center + gApp.monoH / 2 };
			if (pass == 0 && coord != 1 && box.top < lastBottom)
			{
				continue;
			}
			SetTextColor(dc, RulerColor(coord, hot));
			DrawTextW(dc, text, -1, &box, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
			if (pass == 0 && box.bottom > lastBottom)
			{
				lastBottom = box.bottom;
			}
		}
	}
	RestoreDC(dc, rowClip);
	}
	SelectObject(dc, old);
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
	DrawButton(dc, bar.ruler, L"标尺", gApp.rulers, true);
	DrawButton(dc, bar.home, L"归位", false, true);
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
	DrawButton(dc, bar.out, L"-", false, CanZoomStep(-1));
	DrawButton(dc, bar.in, L"+", false, CanZoomStep(1));
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
	PaintSide(dc, layout.side);
	PaintToolBar(dc, layout.tool);
	PaintExpandBar(dc, layout.top);
	PaintZoomBar(dc, layout.top);
	PaintFrames(dc, layout);
}

// 画到一张内存位图上再贴到窗口，避免闪烁。
void PaintWindow(HWND hwnd)
{
	PlaceSideScroll(hwnd);
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

// 滑条被拖动或点了箭头。horizontal 为真时改左右滚动，否则改上下滚动。
void OnSideBar(HWND hwnd, HWND bar, bool horizontal, WPARAM wParam)
{
	if (bar == nullptr)
	{
		return;
	}
	RECT client{};
	const Layout layout = CurrentLayout(hwnd, client);
	const SideView view = MakeSideView(layout.side);
	const int page = horizontal ? view.text.right - view.text.left : view.text.bottom - view.text.top;
	const int line = horizontal ? gApp.monoW : gApp.monoH;
	int pos = horizontal ? gApp.sideX : gApp.sideY;
	switch (LOWORD(wParam))
	{
	case SB_LINELEFT:
		pos -= line;
		break;
	case SB_LINERIGHT:
		pos += line;
		break;
	case SB_PAGELEFT:
		pos -= page;
		break;
	case SB_PAGERIGHT:
		pos += page;
		break;
	case SB_THUMBTRACK:
	case SB_THUMBPOSITION:
	{
		SCROLLINFO info{};
		info.cbSize = sizeof(info);
		info.fMask = SIF_TRACKPOS;
		GetScrollInfo(bar, SB_CTL, &info);
		pos = info.nTrackPos;
		break;
	}
	default:
		return;
	}
	if (horizontal)
	{
		gApp.sideX = pos;
	}
	else
	{
		gApp.sideY = pos;
	}
	PlaceSideScroll(hwnd);
	InvalidateRect(hwnd, nullptr, FALSE);
}

// 滚轮。光标在右侧文字上就滚动文字；在棋盘上就缩放。
bool OnMouseWheel(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	POINT cursor{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	ScreenToClient(hwnd, &cursor);
	RECT client{};
	const Layout layout = CurrentLayout(hwnd, client);
	const int notches = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
	if (Contains(layout.side, cursor))
	{
		const bool horizontal = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
		const int step = horizontal ? gApp.monoW * 4 : gApp.monoH * 3;
		if (horizontal)
		{
			gApp.sideX -= notches * step;
		}
		else
		{
			gApp.sideY -= notches * step;
		}
		ClampSideScroll(layout.side);
		PlaceSideScroll(hwnd);
		InvalidateRect(hwnd, nullptr, FALSE);
		return true;
	}
	const RECT cells = CellArea(layout.board);
	POINT anchor = cursor;
	if (anchor.x < cells.left)
	{
		anchor.x = cells.left;
	}
	if (anchor.y < cells.top)
	{
		anchor.y = cells.top;
	}
	if (anchor.x >= cells.right)
	{
		anchor.x = cells.right > cells.left ? cells.right - 1 : cells.left;
	}
	if (anchor.y >= cells.bottom)
	{
		anchor.y = cells.bottom > cells.top ? cells.bottom - 1 : cells.top;
	}
	if (Contains(layout.board, cursor))
	{
		ZoomAt(anchor, cells, notches);
		InvalidateRect(hwnd, nullptr, FALSE);
	}
	return true;
}

// 客户区尺寸变了，数字框跟着顶栏挪。
void OnSize(HWND hwnd)
{
	PlaceExpandEdits(hwnd);
	PlaceSideScroll(hwnd);
	InvalidateRect(hwnd, nullptr, FALSE);
}

// 换成这一种落笔方式。tool 是画笔、橡皮或开关。点按钮和按 1、2、3 都走这里。
void SelectTool(HWND hwnd, Tool tool)
{
	gApp.tool = tool;
	InvalidateRect(hwnd, nullptr, FALSE);
}

// 左键按下。点到放大、缩小、应用、打开、保存、命令或工具就执行；点到棋盘就开始一笔。
bool OnLeftDown(HWND hwnd, LPARAM lParam)
{
	// 点在数字框外。焦点回到主窗口，1、2、3、-、+、= 才用来切工具和缩放。
	SetFocus(hwnd);
	const POINT cursor{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	RECT client{};
	const Layout layout = CurrentLayout(hwnd, client);
	const SideView sideView = MakeSideView(layout.side);
	if (Contains(sideView.openText, cursor))
	{
		OpenText(hwnd);
		return true;
	}
	if (Contains(sideView.saveText, cursor))
	{
		SaveText(hwnd);
		return true;
	}
	if (Contains(sideView.saveBmp, cursor))
	{
		SaveBmp(hwnd);
		return true;
	}
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
	if (Contains(bar.ruler, cursor))
	{
		ToggleRulers(hwnd);
		return true;
	}
	if (Contains(bar.home, cursor))
	{
		ResetView(hwnd);
		return true;
	}
	for (int i = 0; i < 3; ++i)
	{
		if (!Contains(bar.tools[i], cursor))
		{
			continue;
		}
		SelectTool(hwnd, static_cast<Tool>(i));
		return true;
	}
	const RECT cells = CellArea(layout.board);
	if (gApp.dragging || !Contains(cells, cursor))
	{
		return false;
	}
	PlaceView(layout.board);
	BeginStroke();
	SetCapture(hwnd);
	int x = 0;
	int y = 0;
	if (CellAt(cursor, cells, x, y))
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
	// 拖动时焦点也回到主窗口，随后的快捷键不再填数字框。
	SetFocus(hwnd);
	gApp.dragging = true;
	gApp.dragLast = cursor;
	SetCapture(hwnd);
	return true;
}

// 记下鼠标下的格子，标尺和 01 用它标出当前行列。不在图上则清掉。
void UpdateHover(HWND hwnd, POINT cursor)
{
	RECT client{};
	const Layout layout = CurrentLayout(hwnd, client);
	int x = 0;
	int y = 0;
	bool on = CellAt(cursor, CellArea(layout.board), x, y);
	if (!on)
	{
		on = SideCellAt(cursor, layout.side, x, y);
	}
	if (gApp.hover == on && (!on || (gApp.hoverX == x && gApp.hoverY == y)))
	{
		return;
	}
	gApp.hover = on;
	gApp.hoverX = x;
	gApp.hoverY = y;
	InvalidateRect(hwnd, nullptr, FALSE);
}

// 鼠标移动。拖动时改原点，落笔时改格子，并刷新标尺上的当前行列。
bool OnMouseMove(HWND hwnd, LPARAM lParam)
{
	const POINT cursor{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	TRACKMOUSEEVENT track{};
	track.cbSize = sizeof(track);
	track.dwFlags = TME_LEAVE;
	track.hwndTrack = hwnd;
	TrackMouseEvent(&track);
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
		const RECT cells = CellArea(layout.board);
		int x = 0;
		int y = 0;
		if (CellAt(cursor, cells, x, y))
		{
			StrokeTo(x, y);
		}
		else
		{
			gApp.hasStrokeCell = false;
		}
		InvalidateRect(hwnd, nullptr, FALSE);
	}
	UpdateHover(hwnd, cursor);
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

// 这个键会打出的字符。key 是虚拟键。用来认出不同键盘上的 - 和 +。
wchar_t PrintedKey(WPARAM key)
{
	BYTE state[256]{};
	if (!GetKeyboardState(state))
	{
		return 0;
	}
	wchar_t text[8]{};
	const UINT scan = MapVirtualKeyW(static_cast<UINT>(key), MAPVK_VK_TO_VSC);
	const int count = ToUnicode(static_cast<UINT>(key), scan, state, text, 8, 0);
	if (count <= 0)
	{
		return 0;
	}
	return text[0];
}

// 快捷键。Ctrl+Z 撤销，Ctrl+Y 恢复，Ctrl+Delete 清空，Ctrl+0 归位，Ctrl+Tab 开关标尺。
// 1、2、3 换工具。- 缩小。+ 和 = 放大。都按整十，以棋盘中心为基准。
bool OnKeyDown(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	gApp.zoomKeyUsed = false;
	const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
	if (ctrl && wParam == 'Z')
	{
		DoUndo(hwnd);
		return true;
	}
	if (ctrl && wParam == 'Y')
	{
		DoRedo(hwnd);
		return true;
	}
	if (ctrl && wParam == VK_DELETE)
	{
		DoClear(hwnd);
		return true;
	}
	if (ctrl && (wParam == '0' || wParam == VK_NUMPAD0))
	{
		ResetView(hwnd);
		return true;
	}
	if (ctrl && wParam == VK_TAB)
	{
		// 按住不放时系统会连发。只在第一次按下时开关，避免标尺来回闪。
		const bool repeated = (static_cast<ULONG>(lParam) & (1u << 30)) != 0;
		if (!repeated)
		{
			ToggleRulers(hwnd);
		}
		return true;
	}
	if (ctrl)
	{
		return false;
	}
	if (wParam == '1' || wParam == VK_NUMPAD1)
	{
		SelectTool(hwnd, Tool::Pen);
		return true;
	}
	if (wParam == '2' || wParam == VK_NUMPAD2)
	{
		SelectTool(hwnd, Tool::Eraser);
		return true;
	}
	if (wParam == '3' || wParam == VK_NUMPAD3)
	{
		SelectTool(hwnd, Tool::Toggle);
		return true;
	}
	const wchar_t printed = PrintedKey(wParam);
	if (wParam == VK_OEM_MINUS || wParam == VK_SUBTRACT || printed == L'-')
	{
		ZoomStep(hwnd, -1);
		gApp.zoomKeyUsed = true;
		return true;
	}
	if (wParam == VK_OEM_PLUS || wParam == VK_ADD || printed == L'+' || printed == L'=')
	{
		ZoomStep(hwnd, 1);
		gApp.zoomKeyUsed = true;
		return true;
	}
	return false;
}

// 字符键。+ 放大，- 缩小。按键消息已经缩放过的，这里不再缩一次。
bool OnChar(HWND hwnd, WPARAM wParam)
{
	if (gApp.zoomKeyUsed)
	{
		gApp.zoomKeyUsed = false;
		return true;
	}
	if (wParam == L'+' || wParam == L'=')
	{
		ZoomStep(hwnd, 1);
		return true;
	}
	if (wParam == L'-')
	{
		ZoomStep(hwnd, -1);
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
	case WM_HSCROLL:
		OnSideBar(hwnd, gApp.sideHScroll, true, wParam);
		return 0;
	case WM_VSCROLL:
		OnSideBar(hwnd, gApp.sideVScroll, false, wParam);
		return 0;
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
	case WM_MOUSELEAVE:
		if (gApp.hover)
		{
			gApp.hover = false;
			InvalidateRect(hwnd, nullptr, FALSE);
		}
		return 0;
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
		if (OnKeyDown(hwnd, wParam, lParam))
		{
			return 0;
		}
		break;
	case WM_CHAR:
		if (OnChar(hwnd, wParam))
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
	// 画布、01 文本或黑白 BMP 的规则不对就不出窗口。
	if (!IconSheet::SelfCheck() || !TextSelfCheck() || !BmpSelfCheck())
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
