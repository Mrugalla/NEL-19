#pragma once

namespace menu2 {
	/* generic button with custom functionality onClick */
	struct Button :
		public Comp
	{
		Button(Nel19AudioProcessor& p, Utils& u, juce::String&& tooltp, std::function<void()> _onClick, std::function<void(juce::Graphics&, const Button&)> _onPaint) :
			Comp(p, u, tooltp, Utils::Cursor::Hover),
			onClick(_onClick),
			onPaint(_onPaint)
		{
			setBufferedToImage(true);
		}
		Button(Nel19AudioProcessor& p, Utils& u, const juce::String& tooltp, std::function<void()> _onClick, std::function<void(juce::Graphics&, const Button&)> _onPaint) :
			Comp(p, u, tooltp, Utils::Cursor::Hover),
			onClick(_onClick),
			onPaint(_onPaint)
		{
			setBufferedToImage(true);
		}
	protected:
		std::function<void()> onClick;
		std::function<void(juce::Graphics&, const Button&)> onPaint;

		void mouseEnter(const juce::MouseEvent& evt) override {
			Comp::mouseEnter(evt);
			repaint();
		}
		void mouseExit(const juce::MouseEvent& evt) override { repaint(); }
		void mouseUp(const juce::MouseEvent& evt) override {
			if (evt.mouseWasDraggedSinceMouseDown()) return;
			onClick();
		}
		void paint(juce::Graphics& g) override { onPaint(g, *this); }

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Button)
	};

	/* method for painting a button that opens a sub-menu */
	static void paintMenuButton(juce::Graphics& g, const Button& b, const Utils& utils, bool selected = false) {
		const auto bounds = b.getLocalBounds().toFloat().reduced(nelG::Thicc);
		const auto bounds2 = bounds.reduced(nelG::Thicc);
		g.setColour(utils.colours[Utils::ColourID::Background]);
		g.fillRoundedRectangle(bounds2, nelG::Thicc);
		if (b.isMouseOver()) {
			g.setColour(utils.colours[Utils::ColourID::HoverButton]);
			g.fillRoundedRectangle(bounds2, nelG::Thicc);
			g.setColour(utils.colours[Utils::ColourID::Interactable]);
		}
		else
			if(!selected)
				g.setColour(utils.colours[Utils::ColourID::Normal]);
			else
				g.setColour(utils.colours[Utils::ColourID::Modulation]);
		g.setFont(utils.font);
		g.drawRoundedRectangle(bounds, nelG::Thicc, nelG::Thicc);
		g.drawFittedText(b.getName(), bounds.toNearestInt(), juce::Justification::centred, 1);
	}

	/* generic title label for each (sub-)menu */
	struct Label : public juce::Label {
		Label(Utils& _utils, const juce::String& _name) :
			juce::Label("< " + _name + " >", "< " + _name + " >"),
			utils(_utils)
		{
			setMouseCursor(utils.cursors[Utils::Cursor::Norm]);
			setJustificationType(juce::Justification::centred);
			setFont(utils.font);
			setColour(juce::Label::ColourIds::backgroundColourId, utils.colours[Utils::Background]);
			setColour(juce::Label::ColourIds::textColourId, utils.colours[Utils::Normal]);
			setBufferedToImage(true);
		}
	protected:
		Utils& utils;
		void mouseEnter(const juce::MouseEvent& evt) override {
			utils.updateTooltip(nullptr);
			juce::Label::mouseEnter(evt);
		}

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Label)
	};

	/* generic colourselector */
	struct ColourSelector :
		public Comp
	{
		ColourSelector(Nel19AudioProcessor& p, Utils& u, int idx, std::function<void()> _onExit) :
			Comp(p, u, "", Utils::Cursor::Hover),
			selector(27, 4, 7),
			okButton(p, u, "you accept the changes you made.", [this]() { onClickOK(); }, [this](juce::Graphics& g, const Button& b) { paintMenuButton(g, b, utils); }),
			undoButton(p, u, "you want to go back to better times.", [this]() { onClickUNDO(); }, [this](juce::Graphics& g, const Button& b) { paintMenuButton(g, b, utils); }),
			defaultButton(p, u, "back to default colour.", [this]() { onClickDEFAULT(); }, [this](juce::Graphics& g, const Button& b) { paintMenuButton(g, b, utils); }),
			startColour(utils.colours[idx]),
			colIdx(idx),
			onExit(_onExit)
		{
			addAndMakeVisible(selector);
			addAndMakeVisible(okButton);		okButton.setName("OK");
			addAndMakeVisible(undoButton);		undoButton.setName("undo");
			addAndMakeVisible(defaultButton);	defaultButton.setName("default");

			selector.setCurrentColour(startColour);
			setBufferedToImage(true);
		}
		void update() {
			utils.colours[colIdx] = selector.getCurrentColour();
			Utils::repaintStrict(getTopLevelComponent());
		}
	protected:
		juce::ColourSelector selector;
		Button okButton, undoButton, defaultButton;
		juce::Colour startColour;
		const int colIdx;
		std::function<void()> onExit;

		void resized() override {
			const auto bounds = getLocalBounds().toFloat();
			const auto selectorBounds = nelG::maxQuadIn(bounds);
			selector.setBounds(selectorBounds.toNearestInt());
			const auto buttonsX = selectorBounds.getRight();
			auto buttonY = 0.f;
			const auto buttonsWidth = bounds.getWidth() - buttonsX;
			const auto buttonsHeight = bounds.getHeight() / 3.f;

			okButton.setBounds(juce::Rectangle<float>(buttonsX, buttonY, buttonsWidth, buttonsHeight).toNearestInt());
			buttonY += buttonsHeight;
			undoButton.setBounds(juce::Rectangle<float>(buttonsX, buttonY, buttonsWidth, buttonsHeight).toNearestInt());
			buttonY += buttonsHeight;
			defaultButton.setBounds(juce::Rectangle<float>(buttonsX, buttonY, buttonsWidth, buttonsHeight).toNearestInt());
		}
	private:
		void onClickOK() {
			utils.save(processor, static_cast<Utils::ColourID>(colIdx), selector.getCurrentColour());
			onExit();
		}
		void onClickUNDO() {
			utils.save(processor, static_cast<Utils::ColourID>(colIdx), startColour);
			onExit();
		}
		void onClickDEFAULT() {
			utils.save(processor, static_cast<Utils::ColourID>(colIdx));
			onExit();
		}

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ColourSelector)
	};

	/* generic switch button */
	struct SwitchButton :
		public Comp
	{
		SwitchButton(Nel19AudioProcessor& p, Utils& u, const juce::String& tooltp, const juce::String& name,
			std::function<void(int)> onSwitch, const std::vector<juce::var>& options,
			std::function<int(int)> onIsEnabled
		) :
			Comp(p, u, tooltp),
			label(utils, name),
			buttons()
		{
			for (auto i = 0; i < options.size(); ++i) {
				auto onClick = [os = onSwitch, &btns = buttons, j = i]() {
					os(j);
					for(auto& btn: btns)
						btn->repaint();
				};
				auto onPaint = [this, isEnabled = onIsEnabled, j = i](juce::Graphics& g, const Button& b) {
					paintMenuButton(g, b, utils, isEnabled(j));
				};

				buttons.push_back(std::make_unique<Button>(
					p, u, tooltp, onClick, onPaint
				));
				buttons[i]->setName(options[i].toString());
				addAndMakeVisible(buttons[i].get());
			}

			addAndMakeVisible(label);
			setBufferedToImage(true);
		}
	protected:
		Label label;
		std::vector<std::unique_ptr<Button>> buttons;

		void resized() override {
			auto x = 0.f;
			const auto y = 0.f;
			const auto width = static_cast<float>(getWidth());
			const auto height = static_cast<float>(getHeight());
			const auto compWidth = width / static_cast<float>(buttons.size() + 1);

			label.setBounds(juce::Rectangle<float>(x, y, compWidth, height).toNearestInt());
			x += compWidth;
			for (auto& btn : buttons) {
				btn->setBounds(juce::Rectangle<float>(x, y, compWidth, height).toNearestInt());
				x += compWidth;
			}
		}

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SwitchButton)
	};

	/* generic textbox */
	struct TextBox :
		public Comp,
		public juce::Timer
	{
		TextBox(Nel19AudioProcessor& p, Utils& u, const juce::String& tooltp, const juce::String& _name,
			std::function<bool(const juce::String& txt)> _onUpdate, std::function<juce::String()> onDefaultText, juce::String&& _unit = "") :
			Comp(p, u, tooltp),
			onUpdate(_onUpdate),
			txt(onDefaultText()),
			txtDefault(txt),
			unit(_unit),
			pos(txt.length()),
			showTick(false)
		{
			setName(_name);
			setWantsKeyboardFocus(true);
			setBufferedToImage(true);
		}
	protected:
		std::function<bool(const juce::String& txt)> onUpdate;
		juce::String txt, txtDefault, unit;
		int pos;
		bool showTick;

		void mouseUp(const juce::MouseEvent& evt) override {
			if (evt.mouseWasDraggedSinceMouseDown()) return;
			startTimer(1000.f / 1.5f);
			grabKeyboardFocus();
		}

		void paint(juce::Graphics& g) override {
			const auto bounds = getLocalBounds().toFloat().reduced(nelG::Thicc);
			g.setFont(utils.font);
			g.setColour(utils.colours[Utils::ColourID::Background]);
			g.fillRoundedRectangle(bounds, nelG::Thicc);
			g.setColour(utils.colours[Utils::ColourID::Interactable]);
			g.drawRoundedRectangle(bounds, nelG::Thicc, nelG::Thicc);
			if (showTick) {
				const auto txt2Paint = getName() + ": " + txt.substring(0, pos) + "|" + txt.substring(pos) + unit;
				g.drawFittedText(txt2Paint, getLocalBounds(), juce::Justification::centred, 1);
			}
			else
				g.drawFittedText(getName() + ": " + txt + unit, getLocalBounds(), juce::Justification::centred, 1);

		}
		void resized() override {
			const auto width = static_cast<float>(getWidth());
			const auto height = static_cast<float>(getHeight());
		}
		void timerCallback() override {
			if (!hasKeyboardFocus(true)) return;
			showTick = !showTick;
			repaint();
		}

		bool keyPressed(const juce::KeyPress& key) override {
			const auto code = key.getKeyCode();
			if (code == juce::KeyPress::escapeKey) {
				backToDefault();
				repaintWithTick();
			}
			else if (code == juce::KeyPress::backspaceKey) {
				if (txt.length() > 0) {
					txt = txt.substring(0, pos - 1) + txt.substring(pos);
					--pos;
					repaintWithTick();
				}
			}
			else if (code == juce::KeyPress::deleteKey) {
				if (txt.length() > 0) {
					txt = txt.substring(0, pos) + txt.substring(pos + 1);
					repaintWithTick();
				}
			}
			else if (code == juce::KeyPress::leftKey) {
				if (pos > 0) --pos;
				repaintWithTick();
			}
			else if (code == juce::KeyPress::rightKey) {
				if (pos < txt.length()) ++pos;
				repaintWithTick();
			}
			else if (code == juce::KeyPress::returnKey) {
				bool successful = onUpdate(txt);
				if (successful)
					txtDefault = txt;
				else
					backToDefault();
				repaintWithTick();
			}
			else {
				// character is text
				txt = txt.substring(0, pos) + key.getTextCharacter() + txt.substring(pos);
				++pos;
				repaintWithTick();
			}
			return true;
		}
		void repaintWithTick() {
			showTick = true;
			repaint();
		}
		void backToDefault() {
			txt = txtDefault;
			pos = txt.length();
		}

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextBox)
	};

	/* recursive menu for holding any trees of sub menus and components */
	class Menu :
		public Comp,
		public juce::Timer
	{
		enum ID { ID, MENU, TOOLTIP, COLOURSELECTOR, SWITCH, TEXTBOX, NumIDs };
	public:
		Menu(Nel19AudioProcessor& p, Utils& u, juce::ValueTree _xml, Menu* _parent = nullptr) :
			Comp(p, u),
			xml(_xml),
			nameLabel(u, xml.getProperty("id", "")),
			subMenu(nullptr),
			colourSelector(nullptr),
			parent(_parent)
		{
			std::array<juce::Identifier, NumIDs> id = { "id", "menu", "tooltip", "colourselector", "switch", "textbox" };
			addEntries(id);
			addAndMakeVisible(nameLabel);
			startTimerHz(12);
			setBufferedToImage(true);
		}
	protected:
		juce::ValueTree xml;
		Label nameLabel;
		std::vector<std::unique_ptr<Comp>> entries;
		std::unique_ptr<Menu> subMenu;
		std::unique_ptr<ColourSelector> colourSelector;
		Menu* parent;

		void setVisibleAllMenus(bool e) {
			Comp::setVisible(e);
			if (parent != nullptr)
				parent->setVisibleAllMenus(e);
		}

		void paint(juce::Graphics& g) override {
			g.setColour(utils.colours[Utils::ColourID::Darken]);
			const auto bounds = getLocalBounds().toFloat().reduced(nelG::Thicc2);
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
			const auto entriesHeight = height - entriesY;
			juce::Rectangle<float> entriesBounds(0.f, entriesY, width, entriesHeight);
			const auto entryHeight = entriesHeight / static_cast<float>(entries.size());
			auto entryY = entriesY;
			for (auto& e : entries) {
				e->setBounds(juce::Rectangle<float>(0.f, entryY, width, entryHeight).reduced(1).toNearestInt());
				entryY += entryHeight;
			}
		}
		void timerCallback() override {
			if (colourSelector != nullptr && colourSelector->isVisible())
				colourSelector->update();
		}
	private:
		void addEntries(const std::array<juce::Identifier, NumIDs>& id) {
			for (auto i = 0; i < xml.getNumChildren(); ++i) {
				const auto child = xml.getChild(i);
				const auto type = child.getType();
				if (type == id[MENU])
					addSubMenuButton(id, child, i);
				else if (type == id[COLOURSELECTOR])
					addColourSelector(id, child, i);
				else if (type == id[SWITCH])
					addSwitchButton(id, child, i);
				else if (type == id[TEXTBOX])
					addTextBox(id, child, i);
			}
		}
		void addSubMenuButton(const std::array<juce::Identifier, NumIDs>& id, juce::ValueTree child, const int i) {
			const auto onClick = [this, idx = i]() {
				if (subMenu == nullptr) {
					auto nXml = xml.getChild(idx);
					subMenu.reset(new Menu(processor, utils, nXml, this));
					addAndMakeVisible(*subMenu.get());
					subMenu->setBounds(getLocalBounds());
				}
			};
			const auto onPaint = [this](juce::Graphics& g, const Button& b) {
				paintMenuButton(g, b, utils);
			};
			const auto tooltp = child.getProperty(id[TOOLTIP]);
			entries.push_back(std::make_unique<Button>(processor, utils, tooltp.toString(), onClick, onPaint));
			const auto buttonName = child.getProperty(id[ID]);
			entries.back()->setName(buttonName.toString());
			addAndMakeVisible(*entries.back().get());
		}
		void addColourSelector(const std::array<juce::Identifier, NumIDs>& id, juce::ValueTree child, const int i) {
			const auto onClick = [this, idx = i]() {
				const auto onExit = [this]() {
					auto top = getTopLevelComponent();
					colourSelector->setVisible(false);
					setVisibleAllMenus(true);
					Utils::repaintStrict(top);
				};
				colourSelector.reset(new ColourSelector(processor, utils, idx, onExit));
				auto top = getTopLevelComponent();
				top->addAndMakeVisible(*colourSelector.get());
				const auto bounds = top->getBounds();
				const auto minDimen = std::min(bounds.getWidth(), bounds.getHeight());
				const auto reduced = minDimen / 6;
				colourSelector->setBounds(top->getLocalBounds().reduced(reduced));
				setVisibleAllMenus(false);
			};
			const auto onPaint = [this](juce::Graphics& g, const Button& b) {
				paintMenuButton(g, b, utils);
			};
			const auto buttonName = child.getProperty(id[ID]).toString();
			const auto tooltp = juce::String("adjust the colour of ") + buttonName + juce::String(" UI elements.");
			entries.push_back(std::make_unique<Button>(processor, utils, tooltp, onClick, onPaint));
			entries.back()->setName(buttonName);
			addAndMakeVisible(*entries.back().get());
		}
		void addSwitchButton(const std::array<juce::Identifier, NumIDs>& id, juce::ValueTree child, const int i,
			std::function<void(int)> onSwitch, const juce::String& buttonName, std::function<bool(int)> onIsEnabled) {
			const auto tooltp = child.getProperty(id[TOOLTIP]);
			std::vector<juce::var> options;
			for (auto c = 0; c < child.getNumChildren(); ++c) {
				auto optionChild = child.getChild(c);
				options.push_back(optionChild.getProperty(id[ID]));
			}
			entries.push_back(std::make_unique<SwitchButton>(
				processor, utils, tooltp.toString(), buttonName, onSwitch, options, onIsEnabled
			));
			addAndMakeVisible(*entries.back().get());
		}
		void addSwitchButton(const std::array<juce::Identifier, NumIDs>& id, juce::ValueTree child, const int i) {
			const auto buttonName = child.getProperty(id[ID]).toString();
			// buttonName must match id in menu.xml
			if (buttonName == "tooltips") {
				const auto onSwitch = [this](int e) {
					if (e == 0)
						utils.setTooltipEnabled(processor, false);
					else
						utils.setTooltipEnabled(processor, true);
				};
				const auto onIsEnabled = [this](int i) { return (utils.tooltipEnabled ? 1 : 0) == i; };
				addSwitchButton(id, child, i, onSwitch, buttonName, onIsEnabled);
			}
			else if (buttonName == "parameter popup") {
				const auto onSwitch = [this](int e) {
					if (e == 0)
						utils.setPopUpEnabled(processor, false);
					else
						utils.setPopUpEnabled(processor, true);
				};
				const auto onIsEnabled = [this](int i) { return (utils.popUpEnabled ? 1 : 0) == i; };
				addSwitchButton(id, child, i, onSwitch, buttonName, onIsEnabled);
			}
			else if (buttonName == "interpolation") {
				const auto onSwitch = [this](int e) {
					processor.vibrato.setInterpolationType(static_cast<vibrato::InterpolationType>(e));
					juce::String vibInterpolationID("vibInterpolation");
					processor.apvts.state.setProperty(vibInterpolationID, e, nullptr);
					auto user = processor.appProperties.getUserSettings();
					user->setValue(vibInterpolationID, e);
				};
				const auto onIsEnabled = [this](int i) {
					return static_cast<int>(processor.vibrato.getInterpolationType()) == i;
				};
				addSwitchButton(id, child, i, onSwitch, buttonName, onIsEnabled);
			}
		}
		void addTextBox(const std::array<juce::Identifier, NumIDs>& id, juce::ValueTree child, const int i) {
			const auto buttonName = child.getProperty(id[ID]).toString();
			// buttonName must match id in menu.xml
			if (buttonName == "buffersize") {
				const auto onUpdate = [this](const juce::String& txt) {
					const auto newDelaySize = txt.getFloatValue();
					if (newDelaySize <= 0)
						return false; // delay can't be below 0ms
					else if (newDelaySize > 10000.f)
						return false; // delay can't be longer than 10 sec
					auto user = processor.appProperties.getUserSettings();
					juce::String vibDelaySizeID("vibDelaySize");
					user->setValue(vibDelaySizeID, newDelaySize);
					processor.apvts.state.setProperty(vibDelaySizeID, newDelaySize, nullptr);
					processor.vibrato.resizeDelaySafe(processor, newDelaySize);
					return true;
				};
				const auto onDefaultStr = [this]() {
					const auto dly = processor.vibrato.getSizeInMs(processor);
					return juce::String(dly).substring(0, 4);
				};
				const auto tooltp = child.getProperty(id[TOOLTIP]);
				entries.push_back(std::make_unique<TextBox>(
					processor, utils, tooltp.toString(), buttonName, onUpdate, onDefaultStr, " ms"
				));
				addAndMakeVisible(entries.back().get());
			}
		}

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Menu)
	};

	static void paintMenuButton(juce::Graphics& g, const Button& button, Utils& utils, Menu* menu) {
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
		if (button.isMouseOver()) {
			g.setColour(utils.colours[Utils::HoverButton]);
			g.fillRoundedRectangle(bounds, nelG::Thicc);
		}

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
	static std::unique_ptr<juce::XmlElement> loadXML(const char* data, const int sizeInBytes) {
		return juce::XmlDocument::parse(juce::String(data, sizeInBytes));
	}
	static void openMenu(std::unique_ptr<Menu>& menu, Nel19AudioProcessor& p, Utils& utils, juce::Component& parentComp, juce::Rectangle<int> menuBounds, Button& openButton) {
		if (menu == nullptr) {
			auto xml = loadXML(BinaryData::menu_xml, BinaryData::menu_xmlSize);
			auto state = juce::ValueTree::fromXml(*xml.get());
			menu = std::make_unique<Menu>(p, utils, state);
			parentComp.addAndMakeVisible(menu.get());
			menu->setBounds(menuBounds);
		}
		else
			menu.reset(nullptr);

		openButton.repaint();
	}
}

/*


*/