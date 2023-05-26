#pragma once
#include <array>
#include <JuceHeader.h>
#include "modsys/ModSysGUI.h"

namespace menu2
{
	using namespace gui;
	using NotificationType = gui::NotificationType;
	using Shared = gui::Shared;

	using Path = juce::Path;
	using Stroke = juce::PathStrokeType;
	using Bounds = juce::Rectangle<int>;
	using BoundsF = juce::Rectangle<float>;
	using Point = juce::Point<int>;
	using PointF = juce::Point<float>;
	using Line = juce::Line<int>;
	using LineF = juce::Line<float>;
	using Just = juce::Justification;
	using String = juce::String;
	using Graphics = juce::Graphics;
	using Colour = juce::Colour;
	using Font = juce::Font;
	using Component = juce::Component;
	using Timer = juce::Timer;
	using Mouse = juce::MouseEvent;
	using MouseWheel = juce::MouseWheelDetails;
	using Image = juce::Image;
	using Just = juce::Justification;
	using Random = juce::Random;
	using ValueTree = juce::ValueTree;
	using Identifier = juce::Identifier;
	using Time = juce::Time;

	inline void paintMenuButton(Graphics& g, Button& b, const Utils& utils, bool selected = false)
	{
		//auto& blinky = b.blinky;
		//blinky.flash(g, juce::Colours::white);
		const auto thicc = utils.thicc;
		const auto bounds = b.getLocalBounds().toFloat().reduced(thicc);
		const auto bounds2 = bounds.reduced(thicc);
		g.setColour(Shared::shared.colour(ColourID::Bg));
		g.fillRoundedRectangle(bounds2, thicc);
		juce::Colour mainCol;
		if (b.isMouseOver())
		{
			g.setColour(Shared::shared.colour(ColourID::Hover));
			g.fillRoundedRectangle(bounds2, thicc);
		}
		if (selected)
			mainCol = Shared::shared.colour(ColourID::Interact);
		else
			mainCol = Shared::shared.colour(ColourID::Txt);
		g.setFont(Shared::shared.font);
		g.setColour(mainCol);
		//blinky.flash(g, mainCol);
		g.drawRoundedRectangle(bounds, thicc, thicc);
		g.drawFittedText(b.getName(), bounds.toNearestInt(), Just::centred, 1);
	}
	
	struct ColourSelector :
		public Comp
	{
		ColourSelector(Utils& u) :
			Comp(u, "", CursorType::Interact),
			layout
			(
				{ 1, 5 },
				{ 13, 1 }
			),
			selector(27, 4, 7),
			colourButtons
			{
				Button(u, "this is the colour for " + toString(static_cast<ColourID>(0))),
				Button(u, "this is the colour for " + toString(static_cast<ColourID>(1))),
				Button(u, "this is the colour for " + toString(static_cast<ColourID>(2))),
				Button(u, "this is the colour for " + toString(static_cast<ColourID>(3))),
				Button(u, "this is the colour for " + toString(static_cast<ColourID>(4))),
				Button(u, "this is the colour for " + toString(static_cast<ColourID>(5))),
				Button(u, "this is the colour for " + toString(static_cast<ColourID>(6))),
				Button(u, "this is the colour for " + toString(static_cast<ColourID>(7))),
				Button(u, "this is the colour for " + toString(static_cast<ColourID>(8))),
				Button(u, "this is the colour for " + toString(static_cast<ColourID>(9)))
			},
			undoButton(u, "You want to go back to better times."),
			defaultButton(u, "You wish to go back to the default colour."),
			browser(u, ".col", "Colours"),
			startColour(),
			currentColour(),
			colIdx(0),
			timerIdx(0)
		{
			undoButton.onClick = [this]() { onClickUNDO(); };
			defaultButton.onClick = [this]() { onClickDEFAULT(); };

			undoButton.onPaint = [this](Graphics& g, Button& b) { paintMenuButton(g, b, utils); };
			defaultButton.onPaint = [this](Graphics& g, Button& b) { paintMenuButton(g, b, utils); };

			for (auto i = 0; i < NumCols; ++i)
			{
				auto& btn = colourButtons[i];
				btn.setState(false);
				addAndMakeVisible(btn);
				btn.onPaint = makeTextButtonOnPaint(toString(static_cast<ColourID>(i)), Just::centred, 1);
				btn.onClick = [&, i]()
				{
					for (auto& b : colourButtons)
						b.setState(false);
					colourButtons[i].setState(true);

					initSelector(static_cast<ColourID>(i));

					for (auto& b : colourButtons)
						b.repaint();
				};
			}

			addAndMakeVisible(selector);
		
			addAndMakeVisible(undoButton);		undoButton.setName("undo");
			addAndMakeVisible(defaultButton);	defaultButton.setName("default");

			initSelector(ColourID::Bg);

			browser.init(*this);
			
			browser.loadFunc = [&](const juce::ValueTree& vt)
			{
				for (auto c = 0; c < NumCols; ++c)
				{
					const auto cID = static_cast<ColourID>(c);
					const auto cIDStr = toString(cID);
					const auto cChild = vt.getChildWithName(cIDStr);
					const auto cProp = cChild.getProperty("val", "");
					const auto cStr = cProp.toString();
					Colour col;
					if (cStr.isNotEmpty())
						col = Colour::fromString(cStr);
					else
						col = getDefault(cID);

					Shared::shared.setColour(c, col);
				}
				notify(NotificationType::ColourChanged, nullptr);
			};
			
			browser.saveFunc = [&]()
			{
				auto vt = juce::ValueTree("PROPERTIES");
				for (auto c = 0; c < NumCols; ++c)
				{
					const auto cID = static_cast<ColourID>(c);
					const auto cIDStr = toString(cID);
					auto cChild = juce::ValueTree(cIDStr);
					const auto cProp = Shared::shared.colour(cID).toString();
					cChild.setProperty("val", cProp, nullptr);
					vt.appendChild(cChild, nullptr);
				}
				return vt;
			};

			setOpaque(false);
		}

		void updateTimer() override
		{
			if (!isShowing())
				return;
			
			for(auto& btn: colourButtons)
				btn.updateTimer();
			undoButton.updateTimer();
			defaultButton.updateTimer();
			browser.updateTimer();
			
			++timerIdx;
			if (timerIdx < 15)
				return;

			timerIdx = 0;

			const auto curCol = selector.getCurrentColour();
			if (curCol == currentColour)
				return;
			
			currentColour = curCol;
			update(currentColour);
		}

		void initSelector(ColourID cID)
		{
			colIdx = static_cast<int>(cID);
			currentColour = startColour = Shared::shared.colour(cID);
			selector.setCurrentColour(startColour);
			colourButtons[colIdx].setState(true);
		}
		
		void update(Colour col)
		{
			Shared::shared.setColour(colIdx, col);
			notify(NotificationType::ColourChanged, &colIdx);
		}
		
		void resized() override
		{
			layout.setBounds(getLocalBounds().toFloat());
			{
				auto colourButtonsArea = layout(0, 0, 1, 2);
				auto x = colourButtonsArea.getX();
				auto y = colourButtonsArea.getY();
				auto w = colourButtonsArea.getWidth();
				auto h = colourButtonsArea.getHeight() / NumCols;

				for (auto i = 0; i < NumCols; ++i)
				{
					auto& btn = colourButtons[i];
					btn.setBounds(BoundsF(x,y,w,h).toNearestInt());
					y += h;
				}
			}
			layout.place(selector, 1, 0, 1, 1);
			{
				auto buttonsArea = layout(1, 1, 1, 1);
				auto x = buttonsArea.getX();
				auto y = buttonsArea.getY();
				auto w = buttonsArea.getWidth() / 3.f;
				auto h = buttonsArea.getHeight();

				undoButton.setBounds(BoundsF(x, y, w, h).toNearestInt());
				x += w;
				defaultButton.setBounds(BoundsF(x, y, w, h).toNearestInt());
				x += w;
				browser.getOpenCloseButton().setBounds(BoundsF(x, y, w, h).toNearestInt());
			}
			layout.place(browser, 0, 0, 2, 2, utils.thicc);
		}

		void paint(Graphics& g) override
		{
			g.fillAll(Shared::shared.colour(ColourID::Darken));
		}
		
	protected:
		Layout layout;
		std::array<Button, gui::NumCols> colourButtons;
		juce::ColourSelector selector;
		Button undoButton, defaultButton;
		PresetBrowser browser;
		Colour startColour, currentColour;
		int colIdx;

		int timerIdx;
		
		void onClickUNDO()
		{
			currentColour = startColour;
			update(currentColour);
			selector.setCurrentColour(currentColour);
		}
		
		void onClickDEFAULT()
		{
			currentColour = startColour = getDefault(static_cast<ColourID>(colIdx));
			update(currentColour);
			selector.setCurrentColour(currentColour);
		}
	};

	struct SwitchButton :
		public Comp
	{
		SwitchButton(Utils& u, const juce::String& tooltp, const juce::String& name,
			std::function<void(int)> onSwitch, const std::vector<juce::var>& options,
			std::function<int(int)> onIsEnabled
		) :
			Comp(u, tooltp),
			label(u, name),
			buttons()
		{
			for (auto i = 0; i < options.size(); ++i) {
				auto onClick = [os = onSwitch, &btns = buttons, j = i]()
				{
					os(j);
					for (auto& btn : btns)
						btn->repaint();
				};
				auto onPaint = [this, isEnabled = onIsEnabled, j = i](juce::Graphics& g, Button& b)
				{
					paintMenuButton(g, b, utils, isEnabled(j));
				};

				buttons.push_back(std::make_unique<Button>
				(
					u, tooltp
				));
				auto& back = *buttons.back();
				back.onClick = onClick;
				back.onPaint = onPaint;
				buttons[i]->setName(options[i].toString());
				addAndMakeVisible(buttons[i].get());
			}
			addAndMakeVisible(label);
		}
	protected:
		Label label;
		std::vector<std::unique_ptr<Button>> buttons;

		void paint(juce::Graphics&) override {}
		
		void resized() override
		{
			auto x = 0.f;
			const auto y = 0.f;
			const auto width = static_cast<float>(getWidth());
			const auto height = static_cast<float>(getHeight());
			const auto compWidth = width / static_cast<float>(buttons.size() + 1);

			label.setBounds(BoundsF(x, y, compWidth, height).toNearestInt());
			x += compWidth;
			for (auto& btn : buttons)
			{
				btn->setBounds(BoundsF(x, y, compWidth, height).toNearestInt());
				x += compWidth;
			}
		}
	};

	struct TextBox :
		public Comp,
		public juce::Timer
	{
		TextBox(Utils& u, const juce::String& tooltp, const juce::String& _name,
			std::function<bool(const juce::String& txt)> _onUpdate, std::function<juce::String()> onDefaultText, juce::String&& _unit = "") :
			Comp(u, tooltp),
			blinkyBoy(this),
			onUpdate(_onUpdate),
			txt(onDefaultText()),
			txtDefault(txt),
			unit(_unit),
			pos(txt.length()),
			showTick(false)
		{
			setName(_name);
			setWantsKeyboardFocus(true);
		}
	protected:
		BlinkyBoy blinkyBoy;
		std::function<bool(const juce::String& txt)> onUpdate;
		juce::String txt, txtDefault, unit;
		int pos;
		bool showTick;

		void mouseUp(const Mouse& evt) override
		{
			if (evt.mouseWasDraggedSinceMouseDown()) return;
			startTimer(static_cast<int>(1000.f / 3.f));
			grabKeyboardFocus();
		}

		void paint(Graphics& g) override
		{
			blinkyBoy.flash(g, juce::Colours::white);

			const auto thicc = utils.thicc;
			const auto bounds = getLocalBounds().toFloat().reduced(thicc);
			g.setFont(Shared::shared.font);
			g.setColour(Shared::shared.colour(ColourID::Bg));
			g.fillRoundedRectangle(bounds, thicc);
			blinkyBoy.flash(g, Shared::shared.colour(ColourID::Interact));
			g.setColour(Shared::shared.colour(ColourID::Interact));
			g.drawRoundedRectangle(bounds, thicc, thicc);
			if (showTick)
			{
				const auto txt2Paint = getName() + ": " + txt.substring(0, pos) + "|" + txt.substring(pos) + unit;
				g.drawFittedText(txt2Paint, getLocalBounds(), juce::Justification::centred, 1);
			}
			else
				g.drawFittedText(getName() + ": " + txt + unit, getLocalBounds(), juce::Justification::centred, 1);
		}
		void timerCallback() override
		{
			if (!hasKeyboardFocus(true)) return;
			showTick = !showTick;
			repaint();
		}

		bool keyPressed(const juce::KeyPress& key) override
		{
			const auto code = key.getKeyCode();
			if (code == juce::KeyPress::escapeKey)
			{
				backToDefault();
				repaintWithTick();
			}
			else if (code == juce::KeyPress::backspaceKey)
			{
				if (txt.length() > 0)
				{
					txt = txt.substring(0, pos - 1) + txt.substring(pos);
					--pos;
					repaintWithTick();
				}
			}
			else if (code == juce::KeyPress::deleteKey)
			{
				if (txt.length() > 0)
				{
					txt = txt.substring(0, pos) + txt.substring(pos + 1);
					repaintWithTick();
				}
			}
			else if (code == juce::KeyPress::leftKey)
			{
				if (pos > 0)
					--pos;
				repaintWithTick();
			}
			else if (code == juce::KeyPress::rightKey)
			{
				if (pos < txt.length())
					++pos;
				repaintWithTick();
			}
			else if (code == juce::KeyPress::returnKey)
			{
				bool successful = onUpdate(txt);
				if (successful)
				{
					blinkyBoy.trigger(3.f);
					txtDefault = txt;
					stopTimer();
					giveAwayKeyboardFocus();
				}
				else
					backToDefault();
				repaintWithTick();
			}
			else
			{
				// character is text
				txt = txt.substring(0, pos) + key.getTextCharacter() + txt.substring(pos);
				++pos;
				repaintWithTick();
			}
			return true;
		}
		void repaintWithTick()
		{
			showTick = true;
			repaint();
		}
		void backToDefault()
		{
			txt = txtDefault;
			pos = txt.length();
		}
	};

	struct ImageStrip :
		public Comp
	{
		using Img = std::unique_ptr<ImageComp>;
		using Images = std::vector<Img>;

		ImageStrip(Utils& u) :
			Comp(u, "", CursorType::Default),
			images()
		{
			setBufferedToImage(false);
			setInterceptsMouseClicks(false, true);
		}
		
		void addImage(const char* data, const int size, String&& _tooltip)
		{
			images.push_back(std::make_unique<ImageComp>
			(
				utils, std::move(_tooltip), data, size
			));
			addAndMakeVisible(images.back().get());
		}
		
	protected:
		Images images;

		void paint(juce::Graphics&) override {}

		void resized() override
		{
			const auto thicc = utils.thicc;
			const auto bounds = getLocalBounds().toFloat().reduced(thicc);
			auto x = bounds.getX();
			const auto y = bounds.getY();
			const auto w = bounds.getWidth() / static_cast<float>(images.size());
			const auto h = bounds.getHeight();
			for (auto& img : images)
			{
				img->setBounds(BoundsF(x,y,w,h).reduced(thicc).toNearestInt());
				x += w;
			}
		}
	};

	struct TextComp :
		public Comp
	{
		TextComp(Utils& u, String&& _txt) :
			Comp(u, "", CursorType::Default),
			txt(_txt)
		{

		}
	protected:
		String txt;

		void paint(Graphics& g)
		{
			const auto thicc = utils.thicc;
			const auto bounds = getLocalBounds().toFloat().reduced(thicc);
			g.setColour(Shared::shared.colour(ColourID::Txt));
			g.drawFittedText(txt, bounds.toNearestInt(), Just::left, 100);
		}
	};

	struct Link :
		public Comp
	{
		Link(Utils& u, String&& _tooltip, String&& _name, String&& _link) :
			Comp(u, ""),
			button(u, std::move(_tooltip))
		{
			setBufferedToImage(false);
			setInterceptsMouseClicks(false, true);
			button.onPaint = makeTextButtonOnPaint(std::move(_name));
			button.onClick = [&b = button, link = _link]()
			{
				juce::URL url(link);
				url.launchInDefaultBrowser();
				b.repaint();
			};
			addAndMakeVisible(button);
		}
	protected:
		gui::Button button;

		void paint(Graphics&) override {}
		
		void resized() override
		{
			button.setBounds(getLocalBounds());
		}
	};

	struct LinkStrip :
		public Comp
	{
		using HyperLink = std::unique_ptr<Link>;

		LinkStrip(Utils& u) :
			Comp(u, "")
		{
			setBufferedToImage(false);
			setInterceptsMouseClicks(false, true);
		}
		
		void addLink(juce::String&& _tooltip, juce::String&& _name, juce::String&& _link)
		{
			links.push_back(std::make_unique<Link>
			(
				utils, std::move(_tooltip), std::move(_name), std::move(_link)
			));
			addAndMakeVisible(links.back().get());
		}
	protected:
		std::vector<HyperLink> links;

		void paint(Graphics&) override
		{}
		
		void resized() override
		{
			const auto thicc = utils.thicc;
			const auto bounds = getLocalBounds().toFloat().reduced(thicc);
			auto x = bounds.getX();
			const auto y = bounds.getY();
			const auto w = bounds.getWidth() / static_cast<float>(links.size());
			const auto h = bounds.getHeight();
			for (auto& link : links)
			{
				link->setBounds(BoundsF(x, y, w, h).reduced(thicc).toNearestInt());
				x += w;
			}
		}
	};

	struct ErkenntnisseComp :
		public Comp
	{
		ErkenntnisseComp(Utils& u) :
			Comp(u, "", CursorType::Default),
			layout
			{
				{ 1, 1, 1, 1, 1 },
				{ 13, 1, 3 }
			},
			editor(u, "Enter or edit wisdom.", "Enter wisdom..."),
			date(u, ""),
			manifest(u, "Click here to manifest wisdom to the manifest of wisdom!"),
			inspire(u, "Click here to get inspired by past wisdom of the manifest of wisdom!"),
			reveal(u, "Click here to reveal wisdom from the manifest of wisdom!"),
			clear(u, "Click here to clear the wisdom editor to write more wisdom!"),
			paste(u, "Click here to paste wisdom from the clipboard to the wisdom editor!"),

			timerIdx(0)
		{
			editor.label.font = Shared::shared.fontFlx;
			date.font = Shared::shared.fontFlx;
				
			const File folder(getFolder());
			if (!folder.exists())
				folder.createDirectory();

			addAndMakeVisible(editor);
			addAndMakeVisible(date);

			addAndMakeVisible(manifest);
			addAndMakeVisible(inspire);
			addAndMakeVisible(reveal);
			addAndMakeVisible(clear);
			addAndMakeVisible(paste);
			
			manifest.onPaint = makeTextButtonOnPaint("Manifest");
			inspire.onPaint = makeTextButtonOnPaint("Inspire");
			reveal.onPaint = makeTextButtonOnPaint("Reveal");
			clear.onPaint = makeTextButtonOnPaint("Clear");
			paste.onPaint = makeTextButtonOnPaint("Paste");

			editor.onReturn = [&]()
			{
				saveToDisk();
				return true;
			};

			editor.onClick = [&]()
			{
				editor.enable();
				return true;
			};

			manifest.onClick = [&]()
			{
				saveToDisk();
			};

			inspire.onClick = [&]()
			{
				const File folder(getFolder());

				const auto fileTypes = File::TypesOfFileToFind::findFiles;
				const String extension(".txt");
				const auto wildCard = "*" + extension;
				const auto numFiles = folder.getNumberOfChildFiles(fileTypes, wildCard);
				if (numFiles == 0)
					return parse("I am deeply sorry. There is no wisdom in the manifest of wisdom yet.");

				Random rand;
				auto idx = rand.nextInt(numFiles);

				const juce::RangedDirectoryIterator files
				(
					folder,
					false,
					wildCard,
					fileTypes
				);

				for (const auto& it : files)
				{
					if (idx == 0)
					{
						const File file(it.getFile());
						parse(file.getFileName());
						editor.setText(file.loadFileAsString());
						editor.disable();
						return;
					}
					else
						--idx;
				}
			};

			reveal.onClick = [&]()
			{
				const File file(getFolder() + date.getText());
				if (file.exists())
					file.revealToUser();

				const File folder(getFolder());
				folder.revealToUser();
			};

			clear.onClick = [&]()
			{
				editor.clear();
				editor.enable();
				parse("");
			};

			paste.onClick = [&]()
			{
				const auto cbTxt = juce::SystemClipboard::getTextFromClipboard();
				if (cbTxt.isEmpty())
					return;
				editor.addText(editor.getText() + cbTxt);
			};
		}

		void updateTimer() override
		{
			editor.updateTimer();
			date.updateTimer();
			manifest.updateTimer();
			inspire.updateTimer();
			reveal.updateTimer();
			clear.updateTimer();
			paste.updateTimer();

			++timerIdx;
			if (timerIdx < 15)
				return;
			timerIdx = 0;

			editor.enable();
		}

		void resized() override
		{
			layout.setBounds(getLocalBounds().toFloat());

			layout.place(editor, 0, 0, 5, 1, false);
			layout.place(date, 0, 1, 5, 1, false);

			layout.place(manifest, 0, 2, 1, 1, false);
			layout.place(inspire, 1, 2, 1, 1, false);
			layout.place(reveal, 2, 2, 1, 1, false);
			layout.place(clear, 3, 2, 1, 1, false);
			layout.place(paste, 4, 2, 1, 1, false);
		}

		void paint(Graphics& g) override
		{
			g.setColour(Shared::shared.colour(ColourID::Bg));
			g.fillRect(editor.getBounds().toFloat());
			//g.setColour(Shared::shared.colour(ColourID::Hover));
			g.fillRect(date.getBounds().toFloat());
		}

		String getFolder()
		{
			const auto slash = File::getSeparatorString();
			const auto specialLoc = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory);

			return specialLoc.getFullPathName() + slash + "Mrugalla" + slash + "SharedState" + slash + "TheManifestOfWisdom" + slash;
		}

		void saveToDisk()
		{
			if (editor.isEmpty())
				return parse("You have to enter some wisdom in order to manifest it.");

			const auto now = Time::getCurrentTime();
			const auto nowStr = now.toString(true, true, false, true).replaceCharacters(" ", "_").replaceCharacters(":", "_");

			File file(getFolder() + nowStr + ".txt");

			if (!file.existsAsFile())
				file.create();
			else
				return parse("Relax! You can only manifest 1 wisdom per minute.");

			file.appendText(editor.getText());
			editor.disable();

			parse("Manifested: " + nowStr);
		}

		void parse(String&& msg)
		{
			date.setText(msg);
			date.repaint();
		}

		Layout layout;
		TextEditor editor;
		Label date;
		Button manifest, inspire, reveal, clear, paste;

		int timerIdx;
	};

	struct Menu :
		public Comp
	{
		enum ID { ID, MENU, TOOLTIP, COLOURSELECTOR, WISDOM, SWITCH, TEXTBOX, IMGSTRIP, TXT, LINK, LINKSTRIP, NumIDs };

		Menu(Nel19AudioProcessor& p, Utils& u, ValueTree _xml, Menu* _parent = nullptr) :
			Comp(u, ""),
			processor(p),
			xml(_xml),
			nameLabel(u, xml.getProperty("id", "")),
			subMenu(nullptr),
			colourSelector(u),
			erkenntnisse(u),
			parent(_parent)
		{
			nameLabel.font = Shared::shared.fontFlx; nameLabel.font.setHeight(30.f);
			std::array<Identifier, NumIDs> id =
			{ 
				"id",
				"menu",
				"tooltip",
				"colourselector",
				"wisdom",
				"switch",
				"textbox",
				"imgStrip",
				"txt",
				"link",
				"linkstrip"
			};
			addEntries(id);
			addAndMakeVisible(nameLabel);

			auto& top = utils.pluginTop;
			top.addChildComponent(colourSelector);
			top.addChildComponent(erkenntnisse);
		}
		
		Nel19AudioProcessor& processor;
		ValueTree xml;
		Label nameLabel;
		std::vector<std::unique_ptr<Comp>> entries;
		std::unique_ptr<Menu> subMenu;
		ColourSelector colourSelector;
		ErkenntnisseComp erkenntnisse;
		Menu* parent;

		void setVisibleAllMenus(bool e)
		{
			Comp::setVisible(e);
			if (parent != nullptr)
				parent->setVisibleAllMenus(e);
		}

		void paint(Graphics& g) override
		{
			g.fillAll(Shared::shared.colour(ColourID::Bg));
		}
		
		void resized() override
		{
			const auto width = static_cast<float>(getWidth());
			const auto height = static_cast<float>(getHeight());
			const auto thicc = utils.thicc;
			const auto thicc2 = thicc * 2.f;
			BoundsF nameLabelBounds;
			{
				const auto w = static_cast<float>(nameLabel.font.getStringWidth(nameLabel.getText())) + thicc2;
				const auto h = height * .2f;
				const auto x = (width - w) * .5f;
				const auto y = 0.f;
				nameLabelBounds.setBounds(x, y, w, h);
				nameLabel.setBounds(nameLabelBounds.toNearestInt());
			}
			if (entries.size() == 0)
				return;
			const auto entriesY = nameLabelBounds.getBottom();
			const auto entriesHeight = height - entriesY;
			BoundsF entriesBounds(0.f, entriesY, width, entriesHeight);
			const auto entryHeight = entriesHeight / static_cast<float>(entries.size());
			auto entryY = entriesY;
			for (auto& e : entries)
			{
				e->setBounds(BoundsF(0.f, entryY, width, entryHeight).toNearestInt());
				entryY += entryHeight;
			}

			if (subMenu != nullptr)
				subMenu->setBounds(0, 0, getWidth(), getHeight());
			{
				auto& top = utils.pluginTop;
				const auto topBounds = top.getBounds();
				const auto minDimen = std::min(topBounds.getWidth(), topBounds.getHeight());
				const auto reduced = minDimen / 6;

				const auto nBounds = top.getLocalBounds().reduced(reduced);
				colourSelector.setBounds(nBounds);
				erkenntnisse.setBounds(nBounds);
			}
		}
	
		void updateTimer() override
		{
			nameLabel.updateTimer();
			for (auto& entry : entries)
				entry->updateTimer();
			if(subMenu != nullptr)
				subMenu->updateTimer();
			colourSelector.updateTimer();
			erkenntnisse.updateTimer();
		}

	private:
		void addEntries(const std::array<juce::Identifier, NumIDs>& id)
		{
			for (auto i = 0; i < xml.getNumChildren(); ++i)
			{
				const auto child = xml.getChild(i);
				const auto type = child.getType();
				if (type == id[MENU])
					addSubMenuButton(id, child, i);
				else if (type == id[COLOURSELECTOR])
					addColourSelector(id, child);
				else if (type == id[WISDOM])
					addManifestOfWisdom(id, child);
				else if (type == id[SWITCH])
					addSwitchButton(id, child, i);
				else if (type == id[TEXTBOX])
					addTextBox(id, child, i);
				else if (type == id[IMGSTRIP])
					addImgStrip(id, child, i);
				else if (type == id[TXT])
					addText(id, child, i);
				else if (type == id[LINK])
					addLink(id, child, i);
				else if (type == id[LINKSTRIP])
					addLinkStrip(child);
			}
		}

		void addSubMenuButton(const std::array<juce::Identifier, NumIDs>& id, ValueTree child, const int i)
		{
			const auto onClick = [this, idx = i]()
			{
				if (subMenu == nullptr)
				{
					auto nXml = xml.getChild(idx);
					subMenu.reset(new Menu(processor, utils, nXml, this));
					addAndMakeVisible(*subMenu.get());
					subMenu->setBounds(getLocalBounds());
				}
			};
			
			const auto onPaint = [this](juce::Graphics& g, Button& b)
			{
				paintMenuButton(g, b, utils);
			};
			
			const auto tooltp = child.getProperty(id[TOOLTIP]);
			entries.push_back(std::make_unique<Button>(utils, tooltp.toString()));
			auto& btn = *reinterpret_cast<Button*>(entries.back().get());

			btn.onClick = onClick;
			btn.onPaint = onPaint;
			const auto buttonName = child.getProperty(id[ID]);
			entries.back()->setName(buttonName.toString());
			addAndMakeVisible(*entries.back().get());
		}
		
		void addColourSelector(const std::array<juce::Identifier, NumIDs>& id, ValueTree child)
		{
			const auto buttonName = child.getProperty(id[ID]).toString();
			entries.push_back(std::make_unique<Button>(utils, "individualize the colourscheme here."));
			auto& btn = *reinterpret_cast<Button*>(entries.back().get());
			btn.setName(buttonName);
			addAndMakeVisible(btn);

			btn.onPaint = [](Graphics& g, Button& b)
			{
				paintMenuButton(g, b, b.utils);
			};

			btn.onClick = [&]()
			{
				auto& top = utils.pluginTop;
				colourSelector.setVisible(true);
				const auto bounds = top.getBounds();
				const auto minDimen = std::min(bounds.getWidth(), bounds.getHeight());
				const auto reduced = minDimen / 6;
				colourSelector.setBounds(top.getLocalBounds().reduced(reduced));
				setVisibleAllMenus(false);
			};
		}
		
		void addManifestOfWisdom(const std::array<juce::Identifier, NumIDs>& id, ValueTree child)
		{
			const auto _tooltip = child.getProperty(id[TOOLTIP]).toString();
			entries.push_back(std::make_unique<Button>(utils, _tooltip));
			auto& btn = *reinterpret_cast<Button*>(entries.back().get());
			btn.setName("Manifest Of Wisdom");
			addAndMakeVisible(btn);

			btn.onPaint = [](Graphics& g, Button& b)
			{
				paintMenuButton(g, b, b.utils);
			};

			btn.onClick = [&]()
			{
				auto& top = utils.pluginTop;
				erkenntnisse.setVisible(true);
				const auto bounds = top.getBounds();
				const auto minDimen = std::min(bounds.getWidth(), bounds.getHeight());
				const auto reduced = minDimen / 6;
				erkenntnisse.setBounds(top.getLocalBounds().reduced(reduced));
				setVisibleAllMenus(false);
			};
		}

		void addSwitchButton(const std::array<juce::Identifier, NumIDs>& id, juce::ValueTree child, const int,
			std::function<void(int)> onSwitch, const juce::String& buttonName, std::function<bool(int)> onIsEnabled)
		{
			const auto tooltp = child.getProperty(id[TOOLTIP]);
			std::vector<juce::var> options;
			for (auto c = 0; c < child.getNumChildren(); ++c)
			{
				auto optionChild = child.getChild(c);
				options.push_back(optionChild.getProperty(id[ID]));
			}
			entries.push_back(std::make_unique<SwitchButton>
			(
				utils, tooltp.toString(), buttonName, onSwitch, options, onIsEnabled
			));
			addAndMakeVisible(*entries.back().get());
		}
		
		void addSwitchButton(const std::array<juce::Identifier, NumIDs>& id,
			juce::ValueTree child, const int i)
		{
			const auto buttonName = child.getProperty(id[ID]).toString();
			// buttonName must match id in menu.xml
			if (buttonName == "tooltips")
			{
				const auto onSwitch = [this](int e)
				{
					bool eb = e == 0 ? false : true;
					utils.setTooltipsEnabled(eb);
				};
				const auto onIsEnabled = [this](int i)
				{
					return (utils.getTooltipsEnabled() ? 1 : 0) == i;
				};
				addSwitchButton(id, child, i, onSwitch, buttonName, onIsEnabled);
			}
			else if (buttonName.contains("modType"))
			{
				auto mIdx = 0;
				for (auto j = 0; j < buttonName.length(); ++j)
					if (buttonName[j] == '1')
					{
						mIdx = 1;
						break;
					}
				const auto onSwitch = [this, mIdx](int e)
				{
					const auto objType = vibrato::ObjType::ModType;
					const auto objStr = vibrato::with(objType, mIdx);
					const juce::Identifier id(objStr);
					const auto modType = vibrato::ModType(e);
					const auto mID = vibrato::toString(modType);
					auto user = processor.appProperties.getUserSettings();
					user->setValue(id, mID);
				};
				const auto onIsEnabled = [this, mIdx](int i)
				{
					const auto user = processor.appProperties.getUserSettings();
					const auto objType = vibrato::ObjType::ModType;
					const auto objStr = vibrato::with(objType, mIdx);
					const juce::Identifier id(objStr);
					const auto mID = user->getValue(id);
					const auto modType = vibrato::getModType(mID);
					return static_cast<int>(modType) == i;
				};
				addSwitchButton(id, child, i, onSwitch, buttonName, onIsEnabled);
			}
		}
		
		void addTextBox(const std::array<Identifier, NumIDs>&, ValueTree, const int)
		{
			//const auto buttonName = child.getProperty(id[ID]).toString();
		}
		
		void addImgStrip(const std::array<juce::Identifier, NumIDs>& id, juce::ValueTree child, const int)
		{
			const auto stripName = child.getProperty(id[ID]).toString();
			if (stripName == "logos")
			{
				entries.push_back(std::make_unique<ImageStrip>(this->utils));

				auto strip = static_cast<ImageStrip*>(entries.back().get());
				strip->addImage
				(
					BinaryData::vst3_logo_small_png, BinaryData::vst3_logo_small_pngSize, "thanks to steinberg for having invented something that transcendents into magic occasionally."
				);
				strip->addImage
				(
					BinaryData::shuttle_png, BinaryData::shuttle_pngSize, "thanks to my son Lionel, who inspires me to push myself everyday."
				);
				strip->addImage
				(
					BinaryData::juce_png, BinaryData::juce_pngSize, "thanks to the juce framework for enabling programming noobs like me to fulfill their dreams."
				);

				addAndMakeVisible(entries.back().get());
			}
		}
		
		void addText(const std::array<juce::Identifier, NumIDs>&, juce::ValueTree child, const int)
		{
			entries.push_back(std::make_unique<TextComp>
			(
				utils, child.getProperty("text").toString()
			));
			addAndMakeVisible(entries.back().get());
		}
		
		void addLink(const std::array<juce::Identifier, NumIDs>&, juce::ValueTree child, const int)
		{
			entries.push_back(std::make_unique<Link>
			(
				utils,
				child.getProperty("tooltip").toString(),
				child.getProperty("id").toString(),
				child.getProperty("link").toString()
			));
			addAndMakeVisible(entries.back().get());
		}
		
		void addLinkStrip(ValueTree child)
		{
			entries.push_back(std::make_unique<LinkStrip>(this->utils));

			auto strip = static_cast<LinkStrip*>(entries.back().get());

			for (auto j = 0; j < child.getNumChildren(); ++j)
			{
				const auto c = child.getChild(j);
				strip->addLink
				(
					c.getProperty("tooltip").toString(),
					c.getProperty("id").toString(),
					c.getProperty("link").toString()
				);
			}

			addAndMakeVisible(entries.back().get());
		}
	};

	inline void paintMenuButton(Graphics& g, Button& button, Utils& utils, Menu* menu)
	{
		const auto width = static_cast<float>(button.getWidth());
		const auto height = static_cast<float>(button.getHeight());
		const PointF centre(width, height);
		auto minDimen = std::min(width, height);
		BoundsF bounds
		(
			(width - minDimen) * .5f,
			(height - minDimen) * .5f,
			minDimen,
			minDimen
		);
		const auto thicc = utils.thicc;
		bounds.reduce(thicc, thicc);
		g.setColour(Shared::shared.colour(ColourID::Bg));
		g.fillRoundedRectangle(bounds, thicc);
		if (button.isMouseOver())
		{
			g.setColour(Shared::shared.colour(ColourID::Hover));
			g.fillRoundedRectangle(bounds, thicc);
			g.setColour(Shared::shared.colour(ColourID::Interact));
			if (menu != nullptr)
			{
				g.setColour(Shared::shared.colour(ColourID::Abort));
				g.drawRoundedRectangle(bounds, thicc, thicc);
				g.drawFittedText("X", bounds.toNearestInt(), Just::centred, 1, 0);
				return;
			}
		}
		else
			if (menu == nullptr)
				g.setColour(Shared::shared.colour(ColourID::Txt));
			else
			{
				g.setColour(Shared::shared.colour(ColourID::Abort));
				g.drawRoundedRectangle(bounds, thicc, thicc);
				g.drawFittedText("X", bounds.toNearestInt(), Just::centred, 1, 0);
				return;
			}
		g.drawRoundedRectangle(bounds, thicc, thicc);
		const auto boundsHalf = bounds.reduced(std::min(bounds.getWidth(), bounds.getHeight()) * .25f);
		g.drawEllipse(boundsHalf.reduced(thicc * .5f), thicc);
		minDimen = std::min(boundsHalf.getWidth(), boundsHalf.getHeight());
		const auto radius = minDimen * .5f;
		auto bumpSize = radius * .4f;
		LineF bump(0.f, radius, 0.f, radius + bumpSize);
		const auto translation = juce::AffineTransform::translation(bounds.getCentre());
		for (auto i = 0; i < 4; ++i)
		{
			const auto x = static_cast<float>(i) / 4.f;
			auto rotatedBump = bump;
			const auto rotation = juce::AffineTransform::rotation(x * Tau);
			rotatedBump.applyTransform(rotation.followedBy(translation));
			g.drawLine(rotatedBump, thicc);
		}
		bumpSize *= .6f;
		bump.setStart(0, radius); bump.setEnd(0, radius + bumpSize);
		for (auto i = 0; i < 4; ++i)
		{
			const auto x = static_cast<float>(i) / 4.f;
			auto rotatedBump = bump;
			const auto rotation = juce::AffineTransform::rotation(PiQuart + x * Tau);
			rotatedBump.applyTransform(rotation.followedBy(translation));
			g.drawLine(rotatedBump, thicc);
		}
	}
	
	inline std::unique_ptr<juce::XmlElement> loadXML(const char* data, const int sizeInBytes)
	{
		return juce::XmlDocument::parse(juce::String(data, sizeInBytes));
	}
	
	inline void openMenu(std::unique_ptr<Menu>& menu, Nel19AudioProcessor& p,
		Utils& utils, Component& parentComp, Bounds menuBounds,
		Button& openButton)
	{
		if (menu == nullptr)
		{
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