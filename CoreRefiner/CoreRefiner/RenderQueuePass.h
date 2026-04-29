#pragma once
#include "BindingPass.h"
#include "Job.h"
#include <vector>
#include "Sink.h"
#include "Source.h"

namespace Rgph
{
	class RenderQueuePass : public BindingPass
	{
	public:
		using BindingPass::BindingPass;
		void Accept(Job job) noexcept;
		void Execute(Graphics& gfx) const noxnd override;
		void Reset() noxnd override;
	protected:
		std::vector<Job> jobs;
	};
}