#pragma once
#include <JuceHeader.h>

/*

namespace spline {
	struct Editor :
		public nelG::Comp,
		public juce::Timer
	{
		struct Selector {
			Selector(Editor& e) :
				editor(e),
				pos(-1, -1),
				width(util::Thicc),
				range(0, 0)
			{}
			void operator+=(float deltaY) {
				const auto w = editor.bounds.getWidth();
				width = juce::jlimit(util::Thicc, w, width + (deltaY > 0.f ? .01f : -.01f) * w);
			}
			bool operator()() {
				return editor.points.select(range);
			}
			void deselect() { editor.points.deselect(); }
			void updatePosition(const juce::Point<float>& p) {
				pos = p;
				auto start = pos.x - width;
				auto end = pos.x + width;
				if (start <= editor.bounds.getX()) start = editor.bounds.getX() + 1;
				if (end >= editor.bounds.getRight()) end = editor.bounds.getRight() - 1;
				if (start >= end) start = end - 1;
				range.set(start, end);
			}
			void updatePosition() { pos.setX(-1); }
			void paint(juce::Graphics& g) {
				if (pos.x == -1) return;
				g.setColour(juce::Colour(util::ColGreenNeon).withAlpha(.2f));
				g.fillRect(range.start, editor.bounds.getY(), range.distance, editor.bounds.getHeight());
			}
			const util::Range& getRange() const { return range; }
		private:
			Editor& editor;
			juce::Point<float> pos;
			float width;
			util::Range range;
		};

		Editor(Nel19AudioProcessor& p, nelG::Utils& u) :
			nelG::Comp(p, u, "Draw a shape in this super awesome spline editor!"),
			select(*this),
			bounds(),
			creator(p.nel19.getSplineCreator()),
			spline(),
			points(),
			wannaUpdate(false)
		{
			setMouseCursor(u.cursors[nelG::Utils::Cursor::Cross]);
			startTimerHz(24);
		}
		Creator& getCreator() { return creator; }
		const Points& getPoints() const { return points; }
		void setPoints(const Points& p) {
			points = p;
			wannaUpdate = true;
			tryToPushUpdate();
		}
		const std::vector<float>& getSpline() const { return spline; }
	private:
		Selector select;
		juce::Rectangle<float> bounds;
		Creator& creator;
		std::vector<float> spline;
		Points points;
		bool wannaUpdate;

		void paint(juce::Graphics& g) override {
#if DebugLayout
			g.setColour(juce::Colours::red);
			g.drawRect(getLocalBounds());
#endif
			g.setColour(juce::Colour(util::ColGreen));
			g.drawRoundedRectangle(bounds, util::Rounded, util::Thicc);
			select.paint(g);
			paintSpline(g);
			points.paint(g);
		}
		void paintSpline(juce::Graphics& g) {
			g.setColour(juce::Colour(util::ColGreen));
			auto xLast = bounds.getX();
			auto yLast = spline[0];
			for (auto i = 1; i < spline.size(); ++i) {
				const auto y = spline[i];
				const auto x = xLast + 1.f;
				g.drawLine({ xLast, yLast, x, y }, util::Thicc);
				xLast = x;
				yLast = y;
			}
		}
		void resized() override {
			bounds = getLocalBounds().toFloat().reduced(util::Thicc2);
			spline.resize(static_cast<int>(bounds.getWidth()), 0);
			points.setCopyToMap([&](juce::Point<float>& p) {
				p.setXY(
					(p.x - bounds.getX()) / bounds.getWidth(),
					2.f * (p.y - bounds.getY()) / bounds.getHeight() - 1.f
				);
				});
			points.setCopyFromMap([&](juce::Point<float>& p) {
				p.setXY(
					p.x * bounds.getWidth() + bounds.getX(),
					(p.y * .5f + .5f) * bounds.getHeight() + bounds.getY()
				);
			});
			
			points.copyFrom(creator.getPoints());
			updateTable();
		}
		void timerCallback() override { tryToPushUpdate(); }

		void mouseDown(const juce::MouseEvent&) override {
			select();
			repaint();
		}
		void mouseDrag(const juce::MouseEvent& evt) override {
			select.updatePosition(evt.position);
			if (!points.isSelectionEmpty()) {
				points.drag(evt.getOffsetFromDragStart().toFloat(), bounds);
				wannaUpdate = true;
			}
			repaint();
		}
		void mouseUp(const juce::MouseEvent& evt) override {
			if (evt.mouseWasDraggedSinceMouseDown()) {
				if (points.isSelectionEmpty()) return;
				points.makeFunctionOfX();
				wannaUpdate = true;
			}
			else {
				if (evt.mods.isLeftButtonDown())
					wannaUpdate = points.addPoint(evt.position, bounds);
				else
					wannaUpdate = points.removePoints(select.getRange());
			}
			tryToPushUpdate();
			points.deselect();
			select.updatePosition(evt.position);
			repaint();
		}
		void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) {
			select += wheel.deltaY; repaint();
		}
		void mouseMove(const juce::MouseEvent& evt) override {
			nelG::Comp::mouseMove(evt);
			select.updatePosition(evt.position); repaint();
		}
		void mouseExit(const juce::MouseEvent& evt) override {
			select.updatePosition(); repaint();
		}

		void updateTable() {
			const auto heightHalf = bounds.getHeight() * .5f;
			const auto& table = creator.getTable();
			const auto splineSizeInv = 1.f / static_cast<float>(spline.size());
			const auto map = static_cast<float>(table.size()) * splineSizeInv;
			for (auto x = 0; x < spline.size(); ++x) {
				const auto tX = static_cast<int>(x * map);
				spline[x] = bounds.getY() + table[tX] * heightHalf + heightHalf;
			}
			creator.setReady();
		}

		void tryToPushUpdate() {
			if (wannaUpdate && creator.requestChange(points)) {
				wannaUpdate = false;
				updateTable();
				repaint();
			}
		}

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)
	};

	static constexpr float PresetHeightRelative = .3f;

	class PresetMenu :
		public nelG::Comp {
		class Preset :
			public nelG::Comp
		{
			struct DeleteButton :
				public nelG::Comp
			{
				DeleteButton(Nel19AudioProcessor& p, nelG::Utils& u, Preset& pst) :
					nelG::Comp(p, u, "Delete this preset."),
					preset(pst),
					hovering(false)
				{}
			private:
				Preset& preset;
				bool hovering;

				void paint(juce::Graphics& g) override {
					const auto bounds = getLocalBounds().toFloat().reduced(util::Thicc);
					g.setColour(juce::Colour(util::ColRed));
					if(hovering)
						g.drawRoundedRectangle(bounds, util::Rounded, util::Thicc2);
					else
						g.drawRoundedRectangle(bounds, util::Rounded, util::Thicc);
					g.setFont(utils.font);
					g.drawFittedText("DELETE!", bounds.toNearestInt(), juce::Justification::centred, 1, 0);
				}
				void mouseEnter(const juce::MouseEvent&) override {
					hovering = true;
					preset.setHovering(true);
				}
				void mouseExit(const juce::MouseEvent&) override {
					hovering = false;
					repaint();
				}
				void mouseUp(const juce::MouseEvent& evt) override {
					if (evt.mouseWasDraggedSinceMouseDown()) return;
					preset.remove();
				}

				JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeleteButton)
			};
			struct LoadButton :
				public nelG::Comp
			{
				LoadButton(Nel19AudioProcessor& p, nelG::Utils& u, Preset& pst) :
					nelG::Comp(p, u, "Load this preset."),
					preset(pst),
					hovering(false)
				{}
			private:
				Preset& preset;
				bool hovering;

				void paint(juce::Graphics& g) override {
					const auto bounds = getLocalBounds().toFloat().reduced(util::Thicc);
					
					if (hovering) {
						g.setColour(juce::Colour(util::ColYellow));
						g.drawRoundedRectangle(bounds, util::Rounded, util::Thicc2);
					}	
					else {
						g.setColour(juce::Colour(util::ColGreen));
						g.drawRoundedRectangle(bounds, util::Rounded, util::Thicc);
					}
					g.setFont(utils.font);
					g.drawFittedText("LOAD!", bounds.toNearestInt(), juce::Justification::centred, 1, 0);
				}
				void mouseEnter(const juce::MouseEvent&) override {
					hovering = true;
					preset.setHovering(true);
				}
				void mouseExit(const juce::MouseEvent&) override {
					hovering = false;
					repaint();
				}
				void mouseUp(const juce::MouseEvent& evt) override {
					if (evt.mouseWasDraggedSinceMouseDown()) return;
					preset.load();
				}

				JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoadButton)
			};
		public:
			Preset(Nel19AudioProcessor& p, nelG::Utils& u, const juce::String& n, const Points& pts, PresetMenu& m) :
				nelG::Comp(p, u),
				menu(m),
				name(n),
				points(pts),
				table(),
				deleteButton(p, u, *this),
				loadButton(p, u, *this),
				rb2(
					{ 0, 15, 25, 10 },
					{ 0, 50, 50 }
				),
				hovering(false)
			{
				addAndMakeVisible(deleteButton);
				addAndMakeVisible(loadButton);
				table.resize(128);
				std::vector<juce::Point<float>> jPoints;
				points.copyTo(jPoints);
				createTable(table, jPoints);
			}
			const juce::String& getPresetName() const { return name; }
			const Points& getPresetPoints() const { return points; }
			void remove() { menu.deletePreset(this); }
			void load() { menu.loadPreset(this); }
			void setHovering(bool h) {
				hovering = h;
				repaint();
			}
		private:
			PresetMenu& menu;
			const juce::String name;
			const Points points;
			std::vector<float> table;
			DeleteButton deleteButton;
			LoadButton loadButton;
			nelG::RatioBounds2 rb2;
			bool hovering;
			void paint(juce::Graphics& g) override {
#if DebugRatioBounds
				rb2.paintGrid(g);
#endif
				g.setFont(utils.font);
				if(hovering) g.setColour(juce::Colour(util::ColYellow));
				else g.setColour(juce::Colour(util::ColGreen));
				const auto imgBounds = rb2(0, 0, 2, 2);
				g.drawRoundedRectangle(imgBounds, util::Rounded, util::Thicc);
				const auto map = static_cast<float>(table.size()) / imgBounds.getWidth();
				auto yLast = (table[0] * .5f + .5f) * imgBounds.getHeight();
				for (auto i = 1; i < imgBounds.getWidth(); ++i) {
					const auto iFloat = static_cast<float>(i);
					const auto idx = map * iFloat;
					const auto y = (table[static_cast<int>(idx)] * .5f + .5f) * imgBounds.getHeight();
					const auto x = iFloat + imgBounds.getX();
					g.drawLine(x - 1, yLast, x, y, util::Thicc);
					yLast = y;
				}
				g.drawFittedText(name, imgBounds.reduced(util::Thicc2).toNearestInt(), juce::Justification::topRight, 1, 0);
			}
			void resized() override {
				const auto bounds = getLocalBounds().toFloat().reduced(util::Thicc);
				rb2.setBounds(bounds);
				loadButton.setBounds(rb2(2,0).toNearestInt());
				deleteButton.setBounds(rb2(2,1).toNearestInt());
			}
			void mouseEnter(const juce::MouseEvent&) override { setHovering(true); }
			void mouseExit(const juce::MouseEvent&) override { setHovering(false); }

			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Preset)
		};

		struct EnterNameTextField :
			public nelG::Comp,
			public juce::Timer
		{
			EnterNameTextField(Nel19AudioProcessor& p, nelG::Utils& u, juce::String tooltp, PresetMenu& m) :
				nelG::Comp(p, u, tooltp),
				presetMenu(m),
				str(""),
				drawTick(false),
				tickPos(0)
			{
				setMouseCursor(utils.cursors[nelG::Utils::Cursor::Hover]);
				setWantsKeyboardFocus(true);
				setMouseClickGrabsKeyboardFocus(true);
				startTimer(800);
			}
			void getKeyboardFocus() {
				startTimer(800);
				grabKeyboardFocus();
			}
			void loseKeyboardFocus() {
				moveKeyboardFocusToSibling(true);
				stopTimer();
				drawTick = false;
				tickPos = str.length();
				repaint();
			}
		private:
			PresetMenu& presetMenu;
			juce::String str;
			bool drawTick;
			int tickPos;
		
			void paint(juce::Graphics& g) override {
				const auto bounds = getLocalBounds().toFloat().reduced(util::Thicc);
				if (hasKeyboardFocus(false)) g.setColour(juce::Colour(util::ColYellow));
				else g.setColour(juce::Colour(util::ColGreen));
				g.drawRoundedRectangle(bounds, util::Rounded, util::Thicc);
				juce::String text = "Preset Name: ";
				text += str.substring(0, tickPos);
				if (drawTick) text += "|";
				text += str.substring(tickPos);
				g.drawFittedText(text, bounds.toNearestInt(), juce::Justification::centred, 1);
			}
			bool keyPressed(const juce::KeyPress& key) override {
				if (!hasKeyboardFocus(false)) return false;
				if (key == juce::KeyPress::backspaceKey) {
					if (str.isEmpty()) return false;
					str = str.substring(0, tickPos - 1) + str.substring(tickPos);
					--tickPos;
				}
				else if (key == juce::KeyPress::deleteKey) {
					if (str.isEmpty()) return false;
					str = str.substring(0, tickPos) + str.substring(tickPos + 1);
				}
				else if (key == juce::KeyPress::leftKey) {
					tickPos = juce::jlimit(0, str.length(), tickPos - 1);
				}
				else if (key == juce::KeyPress::rightKey) {
					tickPos = juce::jlimit(0, str.length(), tickPos + 1);
				}
				else if (key == juce::KeyPress::returnKey) {
					if (str.isEmpty()) return false;
					presetMenu.savePreset(str);
					str = "";
					loseKeyboardFocus();
				}
				else if (key == juce::KeyPress::escapeKey) {
					loseKeyboardFocus();
				}
				else {
					const auto ltr = key.getTextCharacter();
					str = str.substring(0, tickPos) + juce::String::charToString(ltr) + str.substring(tickPos);
					++tickPos;
				}
				repaint();
				return true;
			}
			void mouseUp(const juce::MouseEvent& evt) override {
				if (evt.mouseWasDraggedSinceMouseDown()) return;
				getKeyboardFocus();
				repaint();
			}
			void timerCallback() override {
				drawTick = !drawTick;
				repaint();
			}

			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnterNameTextField)
		};

		class Browser :
			public nelG::Comp
		{
			struct ScrollBar :
				public nelG::Comp {
				ScrollBar(Nel19AudioProcessor& p, nelG::Utils& u, Browser& b) :
					nelG::Comp(p, u, "Scroll the preset browser"),
					browser(b),
					positionRelative(0.f), dragStartPos(0.f)
				{ setMouseCursor(utils.cursors[nelG::Utils::Cursor::Hover]); }
				bool needScrollBar() { return browser.getItemsHeightRelative() > 1.f; }
				const float getPositionRelative() const { return positionRelative; }
			private:
				Browser& browser;
				float positionRelative, dragStartPos;

				void paint(juce::Graphics& g) override {
					const auto x = util::Thicc;
					const auto width = static_cast<float>(getWidth()) - util::Thicc2;
					const auto barHeight = static_cast<float>(getHeight()) - util::Thicc;
					float heightRelative;
					if (!needScrollBar()) {
						positionRelative = 0.f;
						heightRelative = 1.f;
					}
					else
						heightRelative = browser.getItemsHeightRelative();
					const auto height = juce::jlimit(util::Thicc, barHeight, barHeight / heightRelative);
					const auto position = positionRelative * (barHeight - height);
					const auto y = juce::jlimit(util::Thicc, barHeight, util::Thicc + position);
					g.setColour(juce::Colour(util::ColYellow));
					g.drawRoundedRectangle({ x,y,width,height }, util::Rounded, util::Thicc);
				}

				void mouseDown(const juce::MouseEvent&) override {
					dragStartPos = positionRelative;
				}
				void mouseDrag(const juce::MouseEvent& evt) override {
					if (!needScrollBar()) return;
					const auto speed = 4.f / browser.getItemsHeightRelative();
					const auto distance = static_cast<float>(evt.getDistanceFromDragStartY());
					const auto height = static_cast<float>(getHeight());
					const auto distanceRelative = distance / height;
					positionRelative = juce::jlimit(0.f, 1.f, dragStartPos + distanceRelative * speed);
					browser.updateItemsPositions();
				}
				void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override {
					if (!needScrollBar()) return;
					const auto speed = .2f / browser.getItemsHeightRelative();
					positionRelative += wheel.deltaY < 0 ? speed : -speed;
					positionRelative = juce::jlimit(0.f, 1.f, positionRelative);
					browser.updateItemsPositions();
				}

				JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScrollBar)
			};
		public:
			Browser(Nel19AudioProcessor& p, nelG::Utils& u, PresetMenu& m) :
				nelG::Comp(p, u),
				scrollBar(p, u, *this),
				menu(m),
				presets(),
				curPreset(nullptr),
				itemsBounds(0,0,0,0),
				itemsHeightRelative(0)
			{
				addAndMakeVisible(scrollBar);
			}
			void setQBounds(juce::Rectangle<int> scrollBarBounds, juce::Rectangle<int> listItemsBounds) {
				const auto x = std::min(scrollBarBounds.getX(), listItemsBounds.getX());
				const auto y = std::min(scrollBarBounds.getY(), listItemsBounds.getY());
				const auto right = std::max(scrollBarBounds.getRight(), listItemsBounds.getRight());
				const auto bottom = std::max(scrollBarBounds.getBottom(), listItemsBounds.getBottom());
				const auto width = right - x;
				const auto height = bottom - y;
				setBounds(x, y, width, height);
				scrollBarBounds.setX(scrollBarBounds.getX() - x);
				scrollBarBounds.setY(scrollBarBounds.getY() - y);
				scrollBar.setBounds(scrollBarBounds);
				// list items setBounds
				itemsBounds.setBounds(
					listItemsBounds.getX() - x,
					listItemsBounds.getY() - y,
					listItemsBounds.getWidth(),
					listItemsBounds.getHeight()
				);
			}
			void setCurrentPreset(Preset* p) { curPreset = p; }
			const Preset* getCurrentPreset() const { return curPreset; }
			void savePreset(const juce::String& name, const Points& points) {
				presets.push_back(std::make_unique<Preset>(
					processor,
					utils,
					name,
					points,
					menu
				));
				const auto width = static_cast<float>(itemsBounds.getWidth()) - util::Thicc2;
				const auto browserHeight = static_cast<float>(itemsBounds.getHeight());
				const auto height = browserHeight * PresetHeightRelative;
				juce::Rectangle<float> itemBounds(0.f, 0.f, width, height);
				presets[presets.size() - 1].get()->setBounds(itemBounds.toNearestInt());
				addAndMakeVisible(presets[presets.size() - 1].get());
				updateItemsPositions();
			}
			void deletePreset(Preset* prst) {
				if (prst == curPreset) curPreset = nullptr;
				for(auto p = 0; p < presets.size(); ++p)
					if (presets[p].get() == prst) {
						presets.erase(presets.begin() + p);
						return;
					}
			}
			const float getItemsHeightRelative() const { return itemsHeightRelative; }
			void updateItemsPositions() {
				const auto x = static_cast<float>(itemsBounds.getX()) + util::Thicc;
				const auto browserY = static_cast<float>(itemsBounds.getY()) + util::Thicc;
				const auto browserHeight = static_cast<float>(itemsBounds.getHeight());
				auto offset = 0.f;
				if (scrollBar.needScrollBar()) {
					const auto posRel = scrollBar.getPositionRelative();
					offset = -posRel * itemsHeightRelative * browserHeight;
				}
				for (auto p = 0; p < presets.size(); ++p) {
					const auto pFloat = static_cast<float>(p);
					const auto margin = pFloat * util::Thicc;
					const auto y = browserY + pFloat * PresetHeightRelative * browserHeight + offset + margin;
					presets[p].get()->setTopLeftPosition(x, y);
				}
				calculateItemsHeightRelative();
				repaint();
			}
		private:
			ScrollBar scrollBar;
			PresetMenu& menu;
			std::vector<std::unique_ptr<Preset>> presets;
			Preset* curPreset;
			juce::Rectangle<int> itemsBounds;
			float itemsHeightRelative;

			void calculateItemsHeightRelative() {
				const auto browserHeight = static_cast<float>(itemsBounds.getHeight());
				auto itemsHeight = util::Thicc;
				for (auto& p : presets)
					itemsHeight += static_cast<float>(p.get()->getHeight()) + util::Thicc;
				itemsHeightRelative = itemsHeight / browserHeight;
			}

			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Browser)
		};
	public:
		PresetMenu(Nel19AudioProcessor& p, nelG::Utils& u, Editor& e) :
			nelG::Comp(p, u),
			rb2(
				{ 0, 3, 12, 3, 184, 37 },
				{ 0, 15, 7, 128 }
			),
			exitButton(p, u, *this),
			enterNameTextField(p, u, "Name your spline preset and hit enter to save!", *this),
			browser(p, u, *this),
			editor(e),
			presetID("preset")
		{
			addAndMakeVisible(exitButton);
			addChildComponent(enterNameTextField);
			addAndMakeVisible(browser);
		}
		void setVisible(bool e) override {
			juce::Component::setVisible(e);
			if (e) {
				enterNameTextField.setVisible(true);
				enterNameTextField.getKeyboardFocus();
			}
		}
		const Preset* getCurrentPreset() const { return browser.getCurrentPreset(); }
		void savePreset(const juce::String& name) { browser.savePreset(name, editor.getPoints()); }
		void deletePreset(Preset* p) {
			browser.deletePreset(p);
			browser.updateItemsPositions();
		}
		void loadPreset(Preset* p) {
			browser.setCurrentPreset(p);
			editor.setPoints(p->getPresetPoints());
		}
	private:
		nelG::RatioBounds2 rb2;
		ExitButton exitButton;
		EnterNameTextField enterNameTextField;
		Browser browser;
		Editor& editor;
		juce::Identifier presetID;

		void paint(juce::Graphics& g) override {
			const auto bounds = getLocalBounds().toFloat().reduced(util::Thicc);
			g.setColour(juce::Colour(util::ColBlack).withAlpha(.9f));
			g.fillRoundedRectangle(bounds, util::Rounded);
#if DebugRatioBounds
			rb2.paintGrid(g);
#endif
			g.setColour(juce::Colour(util::ColGreen));
			g.drawRoundedRectangle(bounds, util::Rounded, util::Thicc);
		}
		void resized() override {
			const auto bounds = getLocalBounds().toFloat().reduced(util::Thicc2);
			rb2.setBounds(bounds);
			exitButton.setBounds(rb2(1, 0).toNearestInt());
			enterNameTextField.setBounds(rb2(3, 0).toNearestInt());
			browser.setQBounds(
				rb2(1, 2).toNearestInt(),
				rb2(3, 2).toNearestInt()
			);
		}

		void mouseUp(const juce::MouseEvent& evt) override {
			if (evt.mouseWasDraggedSinceMouseDown()) return;
			enterNameTextField.loseKeyboardFocus();
		}

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetMenu)
	};

	struct PresetButton :
		public nelG::Comp
	{
		PresetButton(Nel19AudioProcessor& p, nelG::Utils& u, juce::String tooltp, PresetMenu& m) :
			nelG::Comp(p, u, tooltp),
			menu(m)
		{
			setMouseCursor(u.cursors[nelG::Utils::Cursor::Hover]);
		}
	private:
		PresetMenu& menu;
		void paint(juce::Graphics& g) override {
			const auto bounds = getLocalBounds().toFloat().reduced(util::Thicc);
			g.setColour(juce::Colour(util::ColGreen));
			g.drawRoundedRectangle(bounds, util::Rounded, util::Thicc);
			const auto curPreset = menu.getCurrentPreset();
			juce::String text = curPreset == nullptr ?
				"No preset selected.." :
				"Preset: " + curPreset->getPresetName();
			auto font = utils.font;
			font.setHeight(11);
			g.setFont(font);
			g.drawFittedText(text, bounds.toNearestInt(), juce::Justification::centred, 1);
		}
		void mouseUp(const juce::MouseEvent&) override { menu.setVisible(true); }

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetButton)
	};
}

*/

/*
to do:

	PresetMenu
		EnterNameTextField
		Browser
			Preset (n)
			ScrollBar


PRESET BROWSER
	repaint bugs (preset hover, textfield)
		wenn von scrollbar zu weniger entries (delete)
	serialization
	scroll (opposite drag) on presets browser

serialization:
preset vector to base64, base64 to var, var to appProperties

APP DATA: C:\Users\Eine Alte Oma\AppData\Roaming\Mrugalla\NEL
VAR: https://docs.juce.com/master/classvar.html
APP PROPERTIES: https://docs.juce.com/master/classPropertiesFile.html
BASE64: https://docs.juce.com/master/structBase64.html
	https://en.wikipedia.org/wiki/Base64
VALUE TREE WRITE TO STREAM: https://docs.juce.com/master/classValueTree.html#a6305ca13b5d95cd526e1ea7a953df365
	https://forum.juce.com/t/writing-valuetree-to-to-stream-and-then-to-base64encoding-twice-doesnt-give-same-result/25628
OUTPUT STREAM: https://docs.juce.com/master/classMemoryOutputStream.html

*/

/*
			auto user = p.appProperties.getUserSettings();
			if (user->isValidFile()) {
				auto& props = user->getAllProperties();
				auto& keys = props.getAllKeys();
				for (auto k = 0; k < keys.size(); ++k) {
					if (keys[k] == presetID.toString()) {
						auto pName = "";
						Points pPoints;
						juce::Image pThumbNail;
						presets.push_back({pName, pPoints, pThumbNail});
					}
				}
				//tooltipEnabled = !user->getBoolValue(tooltipID, tooltipEnabled);
				//user->setValue(tooltipID, tooltipEnabled);
			}
			*/