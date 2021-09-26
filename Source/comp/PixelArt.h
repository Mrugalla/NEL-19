#pragma once

namespace pxl {
	struct ImgComp :
		public Comp
	{
		ImgComp(Nel19AudioProcessor& p, Utils& u, juce::String&& tooltp, const void* imgData, const int imgSize) :
			Comp(p, u, std::move(tooltp)),
			img(),
			data(imgData), size(imgSize)
		{
			setBufferedToImage(true);
		}
		juce::Image img;
	protected:
		const void* data;
		const int size;

		void paint(juce::Graphics& g) override {
			auto tmp = nelG::load(data, size).rescaled(getWidth(), getHeight(), juce::Graphics::lowResamplingQuality);
			img = juce::Image(juce::Image::ARGB, getWidth(), getHeight(), false);
			for (auto y = 0; y < getHeight(); ++y)
				for (auto x = 0; x < getWidth(); ++x) {
					auto pixel = tmp.getPixelAt(x, y);
					if (!pixelIsTransparent(x, y, pixel))
						if (!pixelIsPartOfColourSheme(x, y, pixel))
							img.setPixelAt(x, y, pixel);
				}
			g.drawImageAt(img, 0, 0, false);
		}

	private:
		bool pixelIsTransparent(int x, int y, juce::Colour pixel) {
			if (pixel.getAlpha() == 0x00) {
				img.setPixelAt(x, y, pixel);
				return true;
			}
			return false;
		}
		bool pixelIsPartOfColourSheme(int x, int y, juce::Colour pixel) {
			for (auto c = 0; c < Utils::ColourID::EnumSize; ++c)
				if (pixel == utils.colours.getDefault(c)) {
					img.setPixelAt(x, y, utils.colours[c]);
					return true;
				}
			return false;
		}
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImgComp)
	};
}