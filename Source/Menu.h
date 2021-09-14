#pragma once

namespace menu {
	/* generic button with custom functionality onClick */
	struct Button :
		public Component
	{
		Button(Nel19AudioProcessor& p, Utils& u, juce::String&& tooltp, std::function<void()> _onClick = nullptr, std::function<void(juce::Graphics&, const Button&)> _onPaint = nullptr) :
			Component(p, u, tooltp, Utils::Cursor::Hover),
			onClick(_onClick),
			onPaint(_onPaint)
		{}
	protected:
		std::function<void()> onClick;
		std::function<void(juce::Graphics&, const Button&)> onPaint;

		void mouseUp(const juce::MouseEvent& evt) override {
			if (evt.mouseWasDraggedSinceMouseDown()) return;
			if (onClick != nullptr)
				onClick();
		}

		void paint(juce::Graphics& g) override {
			if (onPaint != nullptr)
				onPaint(g, *this);
			else
				Component::paint(g);
		}

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Button)
	};

	/* generic menu with name, entries and an optional sub-menu */
	struct Menu :
		public Component
	{
		Menu(Nel19AudioProcessor& p, Utils& u, const juce::String& _name) :
			Component(p, u),
			closeButton(p, u, "closes this menu.", [this]() { setVisible(false); }, [this](juce::Graphics& g, const Button& b) { paintCloseButton(g, b); }),
			nameLabel(_name, _name),
			entries(),
			menu(nullptr)
		{
			addAndMakeVisible(nameLabel);
			getTopLevelComponent()->addAndMakeVisible(closeButton);
			nameLabel.setJustificationType(juce::Justification::centred);
			nameLabel.setFont(utils.font);
		}
		void setQBounds(juce::Rectangle<int> menuBounds, juce::Rectangle<int> closeButtonBounds) {
			closeButton.setBounds(closeButtonBounds);
			setBounds(menuBounds);
			const auto nameX = closeButton.getRight();
			const auto nameY = closeButton.getY();
			const auto nameWidth = getWidth() - nameX;
			const auto nameHeight = closeButton.getHeight();
			nameLabel.setBounds(nameX, nameY, nameWidth, nameHeight);
		}
	protected:
		Button closeButton;
		juce::Label nameLabel;
		std::vector<std::unique_ptr<Component>> entries;
		std::unique_ptr<Menu> menu;

		void paint(juce::Graphics& g) override { g.fillAll(utils.colours[Utils::Background]); }
	private:
		void paintCloseButton(juce::Graphics& g, const Button& button) {
			const auto width = static_cast<float>(button.getWidth());
			const auto height = static_cast<float>(button.getHeight());
			const juce::Point<float> centre(width, height);
			auto minDimen = std::min(width, height);
			juce::Rectangle<float> bounds(
				(width - minDimen) * .5f,
				(height - minDimen) * .5f,
				minDimen,
				minDimen
			);
			bounds.reduce(nelG::Thicc, nelG::Thicc);
			g.setColour(utils.colours[Utils::Background]);
			g.fillRoundedRectangle(bounds, nelG::Thicc);

			g.setColour(utils.colours[Utils::Abort]);
			g.drawRoundedRectangle(bounds, nelG::Thicc, nelG::Thicc);
			g.drawFittedText("X", bounds.toNearestInt(), juce::Justification::centred, 1, 0);

		}

		void resized() override {
			const auto height = static_cast<float>(getHeight());
			auto xEntries = static_cast<float>(nameLabel.getX());
			auto yEntries = static_cast<float>(nameLabel.getBottom());
			auto widthEntries = static_cast<float>(nameLabel.getWidth());
			auto heightEntries = height - yEntries;
			juce::Rectangle<float> entryBounds(xEntries, yEntries, widthEntries, heightEntries);
			const auto numEntries = entries.size();
			if (numEntries < 1.f) return;
			const auto heightEntry = heightEntries / static_cast<float>(numEntries);
			auto yEntry = yEntries;
			for (auto i = 0; i < numEntries; ++i, yEntry += heightEntry)
				entries[i]->setBounds(
					juce::Rectangle<float>(xEntries, yEntry, widthEntries, heightEntry).toNearestInt()
				);
		}

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Menu)
	};

	/* generic plane for a menu with sub-menus */
	struct Plane :
		public Component
	{
		Plane(Nel19AudioProcessor& p, Utils& u, juce::String _name) :
			Component(p, u),
			menu(),
			openButton(p, u, "All the extra stuff. Do you feel extra?", [this]() { openMenu(); }, [this](juce::Graphics& g, const Button& b) { paintSettingsButton(g, b); })
		{
			setName(_name);
			getTopLevelComponent()->addAndMakeVisible(openButton);
			setInterceptsMouseClicks(false, true);
		}
		void setQBounds(juce::Rectangle<int> menuBounds, juce::Rectangle<int> openButtonBounds) {
			openButton.setBounds(openButtonBounds);
			setBounds(menuBounds);
			if (menu != nullptr)
				menu->setQBounds(menuBounds, openButtonBounds);
		}

	protected:
		std::unique_ptr<Menu> menu;
		Button openButton;
	private:
		// methods for openButton
		void openMenu() {
			menu.reset(new Menu(processor, utils, getName()));
			addAndMakeVisible(*menu);
			menu->setQBounds(getLocalBounds(), openButton.getBounds());
		}
		void paintSettingsButton(juce::Graphics& g, const Button& button) {
			const auto width = static_cast<float>(button.getWidth());
			const auto height = static_cast<float>(button.getHeight());
			const juce::Point<float> centre(width, height);
			auto minDimen = std::min(width, height);
			juce::Rectangle<float> bounds(
				(width - minDimen) * .5f,
				(height - minDimen) * .5f,
				minDimen,
				minDimen
			);
			bounds.reduce(nelG::Thicc, nelG::Thicc);
			g.setColour(utils.colours[Utils::Background]);
			g.fillRoundedRectangle(bounds, nelG::Thicc);

			g.setColour(utils.colours[Utils::Normal]);
			g.drawRoundedRectangle(bounds, nelG::Thicc, nelG::Thicc);
			const auto boundsHalf = bounds.reduced(std::min(bounds.getWidth(), bounds.getHeight()) * .25f);
			g.drawEllipse(boundsHalf.reduced(nelG::Thicc * .5f), nelG::Thicc);
			minDimen = std::min(boundsHalf.getWidth(), boundsHalf.getHeight());
			const auto radius = minDimen * .5f;
			auto bumpSize = radius * .4f;
			juce::Line<float> bump(0, radius, 0, radius + bumpSize);
			const auto translation = juce::AffineTransform::translation(bounds.getCentre());
			for (auto i = 0; i < 4; ++i) {
				const auto x = static_cast<float>(i) / 4.f;
				auto rotatedBump = bump;
				const auto rotation = juce::AffineTransform::rotation(x * nelG::Tau);
				rotatedBump.applyTransform(rotation.followedBy(translation));
				g.drawLine(rotatedBump, nelG::Thicc);
			}
			bumpSize *= .6f;
			bump.setStart(0, radius); bump.setEnd(0, radius + bumpSize);
			for (auto i = 0; i < 4; ++i) {
				const auto x = static_cast<float>(i) / 4.f;
				auto rotatedBump = bump;
				const auto rotation = juce::AffineTransform::rotation(nelG::PiQuart + x * nelG::Tau);
				rotatedBump.applyTransform(rotation.followedBy(translation));
				g.drawLine(rotatedBump, nelG::Thicc);
			}
		}
		// methods for openButton end

		void paint(juce::Graphics&) override { removeMenuIfClosed(); }
		void removeMenuIfClosed() noexcept { if (menu != nullptr && !menu->isVisible()) menu.reset(); }

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Plane)
	};
}

namespace menu2 {
	/* generic button with custom functionality onClick */
	struct Button :
		public Component
	{
		Button(Nel19AudioProcessor& p, Utils& u, juce::String&& tooltp, std::function<void()> _onClick, std::function<void(juce::Graphics&, const Button&)> _onPaint) :
			Component(p, u, tooltp, Utils::Cursor::Hover),
			onClick(_onClick),
			onPaint(_onPaint)
		{}
	protected:
		std::function<void()> onClick;
		std::function<void(juce::Graphics&, const Button&)> onPaint;

		void mouseUp(const juce::MouseEvent& evt) override {
			if (evt.mouseWasDraggedSinceMouseDown()) return;
			onClick();
		}
		void paint(juce::Graphics& g) override { onPaint(g, *this); }

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Button)
	};

	/* generic menu for holding any trees of sub menus */
	struct Menu :
		public Component
	{
		Menu(Nel19AudioProcessor& p, Utils& u, juce::String&& mName) :
			Component(p, u),
			nameLabel(mName, mName)
		{
			addAndMakeVisible(nameLabel);
			nameLabel.setJustificationType(juce::Justification::centred);
			nameLabel.setFont(utils.font);
			nameLabel.setColour(juce::Label::ColourIds::textColourId, utils.colours[Utils::Normal]);
		}
		template<typename ComponentType>
		void addEntry(ComponentType&& entryArgs) {
			entries.push_back(std::make_unique<ComponentType>(entryArgs));
			addAndMakeVisible(entries.back().get());
		}
	protected:
		juce::Label nameLabel;
		std::vector<std::unique_ptr<Component>> entries;

		void paint(juce::Graphics& g) override {
			g.setColour(juce::Colour(0xee000000));
			g.fillRoundedRectangle(getLocalBounds().toFloat(), nelG::Thicc2);
		}
		void resized() override {
			const auto width = static_cast<float>(getWidth());
			const auto height = static_cast<float>(getHeight());
			const auto nameLabelHeight = height * .2f;
			juce::Rectangle<float> nameLabelBounds(0.f, 0.f, width, nameLabelHeight);
			nameLabel.setBounds(nameLabelBounds.toNearestInt());
			if (entries.size() == 0) return;
			const auto entriesY = nameLabelBounds.getBottom();
			juce::Rectangle<float> entriesBounds(0.f, entriesY, width, height - entriesY);
			const auto entryHeight = entriesBounds.getHeight() / static_cast<float>(entries.size());
			auto entryY = entriesBounds.getY();
			for (auto& e : entries) {
				e->setBounds(juce::Rectangle<float>(0.f, entryY, width, entryHeight).toNearestInt());
				entryY += entryHeight;
			}
		}

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Menu)
	};

	static void paintSettingsButton(juce::Graphics& g, const Button& button, Utils& utils, Menu* menu) {
		const auto width = static_cast<float>(button.getWidth());
		const auto height = static_cast<float>(button.getHeight());
		const juce::Point<float> centre(width, height);
		auto minDimen = std::min(width, height);
		juce::Rectangle<float> bounds(
			(width - minDimen) * .5f,
			(height - minDimen) * .5f,
			minDimen,
			minDimen
		);
		bounds.reduce(nelG::Thicc, nelG::Thicc);
		g.setColour(utils.colours[Utils::Background]);
		g.fillRoundedRectangle(bounds, nelG::Thicc);

		if (menu != nullptr) {
			g.setColour(utils.colours[Utils::Abort]);
			g.drawRoundedRectangle(bounds, nelG::Thicc, nelG::Thicc);
			g.drawFittedText("X", bounds.toNearestInt(), juce::Justification::centred, 1, 0);
			return;
		}

		g.setColour(utils.colours[Utils::Normal]);
		g.drawRoundedRectangle(bounds, nelG::Thicc, nelG::Thicc);
		const auto boundsHalf = bounds.reduced(std::min(bounds.getWidth(), bounds.getHeight()) * .25f);
		g.drawEllipse(boundsHalf.reduced(nelG::Thicc * .5f), nelG::Thicc);
		minDimen = std::min(boundsHalf.getWidth(), boundsHalf.getHeight());
		const auto radius = minDimen * .5f;
		auto bumpSize = radius * .4f;
		juce::Line<float> bump(0, radius, 0, radius + bumpSize);
		const auto translation = juce::AffineTransform::translation(bounds.getCentre());
		for (auto i = 0; i < 4; ++i) {
			const auto x = static_cast<float>(i) / 4.f;
			auto rotatedBump = bump;
			const auto rotation = juce::AffineTransform::rotation(x * nelG::Tau);
			rotatedBump.applyTransform(rotation.followedBy(translation));
			g.drawLine(rotatedBump, nelG::Thicc);
		}
		bumpSize *= .6f;
		bump.setStart(0, radius); bump.setEnd(0, radius + bumpSize);
		for (auto i = 0; i < 4; ++i) {
			const auto x = static_cast<float>(i) / 4.f;
			auto rotatedBump = bump;
			const auto rotation = juce::AffineTransform::rotation(nelG::PiQuart + x * nelG::Tau);
			rotatedBump.applyTransform(rotation.followedBy(translation));
			g.drawLine(rotatedBump, nelG::Thicc);
		}
	}
	static void openMenu(std::unique_ptr<Menu>& menu, Nel19AudioProcessor& p, Utils& u, juce::Component& parentComp, juce::Rectangle<int> menuBounds, Button& openButton) {
		if (menu == nullptr) {
			menu = std::make_unique<Menu>(p, u, "options");
			parentComp.addAndMakeVisible(menu.get());
			menu->setBounds(menuBounds);
		}
		else
			menu.reset();

		openButton.repaint();
	}
	static void createMenuHierarchy(juce::ValueTree state, Utils& utils) {
		const juce::Identifier id("optionsHierarchy");
		/*
		auto ohState = state.getChildWithName(id);
		if (!ohState.isValid()) {
			ohState = juce::ValueTree(id);
			
			// MAIN MENU (graphics, help)
			juce::ValueTree graphicsMenu("graphics");
			ohState.appendChild(graphicsMenu, nullptr);
			juce::ValueTree helpMenu("help");
			ohState.appendChild(helpMenu, nullptr);

			// GRAPHICS MENU (colours)
			juce::ValueTree coloursMenu("colours");
			graphicsMenu.appendChild(coloursMenu, nullptr);

			//HELP MENU (tooltips)
			helpMenu.setProperty("switchbutton", "disable tooltips;enable tooltips", nullptr);

			// COLOURS MENU (each colour's selector)
			for (auto c = 0; c < Utils::EnumSize; ++c) {
				juce::ValueTree colourState("colourselector");
				const auto col = static_cast<juce::int64>(utils.colours[c].getARGB());
				colourState.setProperty(utils.colours.identify(c), col, nullptr);
				coloursMenu.appendChild(colourState, nullptr);
			}	

			// ADD ohState to state
			state.appendChild(ohState, nullptr);
		}
		*/
		//DBG(state.toXmlString());
	}
}

/*
menu stuff

1. utils are static: change all components (worth it)
	utils handles colours, tooltips enabled etc.
2. menu constructed from utils if openButton clicked
*/