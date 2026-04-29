#pragma once
#include "Sprite2D.h"
#include "Channels.h"

#include "SoundCodex.h"

class Button
{
public:
	Button(Graphics& gfx, Rgph::RenderGraph& rg, XMFLOAT2 pos, XMFLOAT2 size, XMFLOAT2 check, int numU, int numV, int idleNum, int chooseNum, int clickNum, std::string path)
		:
		checkField(check),
		NumU{ numU },
		NumV{ numV },
		IdleNum{ idleNum },
		ChooseNum{ chooseNum },
		ClickNum{ clickNum }
	{
		visualPre = std::make_unique<Sprite2D>(gfx, std::vector<std::string>{ path });
		visualPre->SetPosition(pos.x, pos.y);
		visualPre->SetScale(size.x, size.y);
		visualPre->SetFrame(NumU, NumV, IdleNum, 1, 1, 0, false);
		visualPre->LinkTechniques(rg);
	}
	~Button() = default;
	void Submit()
	{
		if (OnChoose)
			if (OnClick)
				visualPre->SetFrame(NumU, NumV, ClickNum, 1, 1, 0);
			else
				visualPre->SetFrame(NumU, NumV, ChooseNum, 1, 1, 0);
		else
			visualPre->SetFrame(NumU, NumV, IdleNum, 1, 1, 0);

		visualPre->Submit(Chan::ui);
	}
	bool MouseEnterCheck(XMFLOAT2 mousePos)
	{
		auto pos = visualPre->GetPosition();
		auto size = XMFLOAT2{ checkField.x / 2.0f, checkField.y / 2.0f };

		float right = pos.x + size.x; // âE
		float left =  pos.x - size.x; // ç∂
		float down =  pos.y + size.y; // â∫
		float up =	  pos.y - size.y; // è„

		bool onEnter = false;
		if (mousePos.x < right && mousePos.x > left && mousePos.y > up && mousePos.y < down)
			onEnter = true;

		return onEnter;
	}
	void SetOnChoose(bool choose) { OnChoose = choose; }
	bool GetOnChoose(void) const { return OnChoose; }
	void SetOnClick(bool click) { OnClick = click; }
	bool GetOnClick(void) const { return OnClick; }
	void SetOnExecute(bool execute) 
	{ 
		if (execute && !OnExecute)
		{
			SoundCodex::Get().PlaySE(SndPath::SE_Button_Click);
		}
		OnExecute = execute;
	}
	bool GetOnExecute(void) const { return OnExecute; }
	void Reset(void)
	{
		OnChoose = false;
		OnClick = false;
		OnExecute = false;
	}
	XMFLOAT2 GetPosition()
	{
		auto pos = visualPre->GetPosition();
		return { pos.x ,pos.y };
	}
private:
	std::unique_ptr<Sprite2D> visualPre;
	XMFLOAT2 checkField{ 0.0f,0.0f };
	int NumU{ 0 };
	int NumV{ 0 };
	int IdleNum{ 0 };
	int ChooseNum{ 0 };
	int ClickNum{ 0 };
	bool OnChoose{ false };
	bool OnClick{ false };
	bool OnExecute{ false };
};