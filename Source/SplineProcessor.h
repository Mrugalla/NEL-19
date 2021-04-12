#pragma once

namespace spline {
	struct Point {
		Point(const juce::Point<float>& a = { 0.f, 0.f }) :
			dragStart(a),
			p(a),
			selected(false)
		{}
		bool select(const util::Range& range) {
			if (p.x >= range.start && p.x < range.end) {
				selected = true;
				dragStart = p;
			}
			return selected;
		}
		void deselect() { selected = false; }
		void drag(const juce::Point<float>& vec, const juce::Rectangle<float>& bounds) {
			if (selected)
				p = dragStart + vec;
			if (p.x <= bounds.getX()) p.x = bounds.getX() + 1;
			else if (p.x >= bounds.getRight()) p.x = bounds.getRight() - 1;
			if (p.y <= bounds.getY()) p.y = bounds.getY();
			else if (p.y >= bounds.getBottom()) p.y = bounds.getBottom();
		}
		void paint(juce::Graphics& g) {
			if (selected) g.setColour(juce::Colour(util::ColYellow));
			else g.setColour(juce::Colour(util::ColGreenNeon));
			g.drawEllipse({ p.x - util::Thicc, p.y - util::Thicc, util::Thicc2 + 1, util::Thicc2 + 1 }, util::Thicc);
		}
		juce::Point<float> dragStart, p;
		bool selected;
	};

	struct Points {
		Points(const std::vector<juce::Point<float>>& p = {}) :
			pts(),
			selectionEmpty(true)
		{
			pts.reserve(p.size());
			for (auto i = 0; i < p.size(); ++i)
				pts.emplace_back(p[i]);
		}
		void setCopyToMap(std::function<void(juce::Point<float>&)> m) { copyToMap = m; }
		void setCopyFromMap(std::function<void(juce::Point<float>&)> m) { copyFromMap = m; }
		bool addPoint(const juce::Point<float>& p, const juce::Rectangle<float>& bounds) {
			if (p.x > bounds.getX() && p.x < bounds.getWidth()) {
				for (const auto& pt : pts) if (p.x == pt.p.x) return false;
				pts.push_back(p);
				sort();
				return true;
			}
			return false;
		}
		bool removePoints(const util::Range& range) {
			bool removedPoints = false;
			for (auto i = 1; i < pts.size() - 1; ++i) {
				auto& p = pts[i].p;
				if (p.x >= range.start)
					if (p.x <= range.end) {
						auto it = pts.begin() + i;
						pts.erase(it);
						removedPoints = true;
						--i;
					}
					else return removedPoints;
			}
			return removedPoints;
		}
		void drag(const juce::Point<float>& vec, const juce::Rectangle<float>& bounds) {
			for (auto& p : pts)
				p.drag(vec, bounds);
			sort();
		}
		void makeFunctionOfX() {
			auto xLast = pts[0].p.x;
			for (auto p = 1; p < pts.size() - 1; ++p) {
				auto x = pts[p].p.x;
				if (std::abs(x - xLast) < 1) {
					pts.erase(pts.begin() + p);
					--p;
				}
				xLast = x;
			}
			if (xLast == pts[pts.size() - 1].p.x)
				pts.erase(pts.end() - 2);
		}
		bool select(const util::Range& range) {
			selectionEmpty = true;
			for (auto p = 1; p < pts.size(); ++p)
				if (pts[p].select(range))
					selectionEmpty = false;
			return selectionEmpty;
		}
		void deselect() { for (auto& p : pts) p.deselect(); selectionEmpty = true; }
		void paint(juce::Graphics& g) { for (auto& p : pts) p.paint(g); }
		const size_t numPoints() const { return pts.size(); }
		const Point& operator[](int i) const { return pts[i]; }
		void sort() { std::sort(pts.begin(), pts.end(), [](Point& a, Point& b) { return a.p.x < b.p.x; }); }
		void copyTo(std::vector<juce::Point<float>>& other) const {
			other.clear();
			other.push_back(pts[0].p);
			auto xLast = pts[0].p.x;
			for (auto p = 1; p < pts.size(); ++p) {
				const auto x = pts[p].p.x;
				if (!(std::abs(x - xLast) < 1))
					other.push_back(pts[p].p);
				xLast = x;
			}
			for (auto& o : other)
				copyToMap(o);
			other[0].x = 0.f;
			other[other.size() - 1].x = 1.f;
		}
		void copyFrom(const std::vector<juce::Point<float>>& other) {
			pts.clear();
			pts.reserve(other.size());
			for (auto i = 0; i < other.size(); ++i)
				pts.emplace_back(other[i]);
			for (auto& p : pts)
				copyFromMap(p.p);
		}
		const bool isSelectionEmpty() const { return selectionEmpty; }
		const bool empty() const { return pts.empty(); }
		void dbg() {
			juce::String str("pts: " + pts.size());
			str += "\n";
			for (const auto& p : pts) {
				str += juce::String(p.p.x);
				str += ", ";
				str += juce::String(p.p.y);
				str += " -> ";
				str += p.selected ? "1" : "0";
				str += "\n";
			}
			DBG(str);
		}
	private:
		std::vector<Point> pts;
		bool selectionEmpty;
		std::function<void(juce::Point<float>&)> copyToMap, copyFromMap;
	};

	static void createTable(std::vector<float>& table, const std::vector<juce::Point<float>>& points) {
		auto p0 = 0, p1 = 1;
		auto x0 = points[p0].x;
		auto x1 = points[p1].x;
		auto xRange = x1 - x0;
		auto xRangeInv = 1.f / xRange;
		const auto tableSizenv = 1.f / static_cast<float>(table.size());
		for (auto i = 0; i < table.size(); ++i) {
			const auto tRatio = tableSizenv * static_cast<float>(i);
			if (tRatio >= x1) {
				++p0;
				++p1;
				x0 = points[p0].x;
				x1 = points[p1].x;
				xRange = x1 - x0;
				xRangeInv = 1.f / xRange;
			}
			const auto idx = static_cast<float>(p0) + (tRatio - x0) * xRangeInv;
			table[i] = interpolation::Spline::process(points, idx);
		}
		for (auto& t : table)
			if (t <= -1.f) t = -1.f + std::numeric_limits<float>::min();
			else if (t >= 1.f) t = 1.f - std::numeric_limits<float>::min();
	}

	class Creator {
		enum class State { Init, Loading, TableRefreshed, PullingTable, Ready };
	public:
		Creator() :
			points({ {0,0},{1,0} }),
			table(),
			vt(),
			id("spline"), idPoint("point"), idX("x"), idY("y"),
			state(State::Init)
		{ table.resize(2056, 0.f); }
		// SET
		void setState(juce::ValueTree& v) {
			vt = v.getChildWithName(id);
			if (!vt.isValid()) {
				vt = juce::ValueTree(id);
				v.appendChild(vt, nullptr);
			}
			const auto numChildren = vt.getNumChildren();
			if (numChildren > 2) {
				points.clear();
				for (auto i = 0; i < numChildren; ++i) {
					const auto child = vt.getChild(i);
					if (child.getType() == idPoint) {
						const auto x = static_cast<float>(child.getProperty(idX));
						const auto y = static_cast<float>(child.getProperty(idY));
						points.push_back({ x, y });
					}
				}
			}
			makeTable();
			state.store(State::Ready);
		}
		void getState(juce::ValueTree& v) {
			if (!vt.isValid()) {
				vt = juce::ValueTree(id);
				v.appendChild(vt, nullptr);
			}
			vt.removeAllChildren(nullptr);
			for (const auto& p : points) {
				juce::ValueTree child(idPoint);
				child.setProperty(idX, p.x, nullptr);
				child.setProperty(idY, p.y, nullptr);
				vt.appendChild(child, nullptr);
			}
		}
		// IDs
		const juce::Identifier& getID() const { return id; }
		// GUI SIDE
		bool requestChange(const Points& newPoints) {
			if (!isReady()) return false;
			state.store(State::Loading);
			newPoints.copyTo(points);
			makeTable();
			return true;
		}
		bool isReady() { return state.load() == State::Ready; }
		const std::vector<float>& getTable() const { return table; }
		const std::vector<juce::Point<float>>& getPoints()const { return points; }
		// DSP SIDE
		bool tableRefreshed() {
			if (state.load() == State::TableRefreshed) {
				state.store(State::PullingTable);
				return true;
			}
			return false;
		}
		void setReady() { state.store(State::Ready); }
	private:
		std::vector<juce::Point<float>> points;
		std::vector<float> table;
		juce::ValueTree vt;
		juce::Identifier id, idPoint, idX, idY;
		std::atomic<State> state;

		void makeTable() {
			createTable(table, points);
			state.store(State::TableRefreshed);
		}

		void dbg() {
			juce::String str;
			for (auto& t : points)
				str += t.toString() + " :: ";
			DBG(str);
		}
	};
}