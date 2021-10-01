#pragma once

namespace pxl {
	static std::function<void(const juce::Image& src, juce::Image& dest)> colourize(Utils& _utils, Utils::ColourID cIDFrom, Utils::ColourID cIDTo) {
		return [&utils = _utils, from = cIDFrom, to = cIDTo](const juce::Image& src, juce::Image& dest) {
			for (auto y = 0; y < dest.getHeight(); ++y)
				for (auto x = 0; x < dest.getWidth(); ++x) {
					const auto pxl = src.getPixelAt(x, y);
					const auto luminance = pxl.getBrightness();
					const auto col = utils.colours[from].interpolatedWith(utils.colours[to], luminance);
					const auto a = pxl.getAlpha();
					dest.setPixelAt(x, y, col.withAlpha(a));
				}
		};
	}

	struct ImgComp :
		public Comp
	{
		ImgComp(Nel19AudioProcessor& p, Utils& u, juce::String&& tooltp,
			const void* imgData, const int imgSize,
			std::function<void(const juce::Image& src, juce::Image& dest)> _onPaint = nullptr) :
			Comp(p, u, std::move(tooltp)),
			img(),
			fx(_onPaint),
			data(imgData), size(imgSize)
		{
			setBufferedToImage(true);
		}
		juce::Image img;
		std::function<void(const juce::Image& src, juce::Image& dest)> fx;
	protected:
		const void* data;
		const int size;

		void paint(juce::Graphics& g) override {
			auto tmp = nelG::load(data, size).rescaled(getWidth(), getHeight(), juce::Graphics::lowResamplingQuality);
			img = juce::Image(juce::Image::ARGB, getWidth(), getHeight(), false);
			if (fx != nullptr)
				fx(tmp, img);
			else
				img = tmp.createCopy();
			g.drawImageAt(img, 0, 0, false);
		}
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImgComp)
	};
}