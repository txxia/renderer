#include "Util.h"
#include "GUI.h"
#include "System/Memory.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"
#include "Math/Geometry.h"
#include "Drawing.h"

using std::vector;

namespace TX { namespace UI { namespace GUI {
	class Window {
	public:
		const uint32	id;
		DrawList		drawList;
		bool			accessed;
		float			contentHeight;
		float			scroll;
		void Reset(){ accessed = false; drawList.Clear(); }
		const Rect& GetRect(){ return drawList.clipRectStack.back(); }
		static uint32 GetID(const char *name){
			// sdbm
			uint32 hash = 0;
			char c;
			while (c = *name++){
				hash = c + (hash << 6) + (hash << 16) - hash;	// hash(i) = hash(i - 1) * 65599 + str[i]; 
			}
			return hash;
		}
	public:
		Window(uint32 id) : id(id), accessed(false), scroll(0.f), contentHeight(1.f){}
	};

	// Ids that uniquely identify a widget inside a window
	struct Widget {
		Window*		window = nullptr;
		uint32		itemId = 0;
		uint32		index = 0;
		Widget() : window(nullptr), itemId(0), index(0){}
		Widget(const Widget& ot) : window(ot.window), itemId(ot.itemId), index(ot.index){}
		Widget& operator = (const Widget& ot){ window = ot.window; itemId = ot.itemId; index = ot.index; return *this; }
		bool operator == (const Widget& ot){ return window == ot.window && itemId == ot.itemId && index == ot.index; }
		bool operator != (const Widget& ot){ return window != ot.window || itemId != ot.itemId || index != ot.index; }
		void Reset(Window *w = nullptr){ window = w; itemId = index = 0; }
		bool HasValue(){ return window; }
	};
	std::ostream& operator << (std::ostream& os, const Widget& w)
	{
		return os << w.window << "-" << w.itemId << "-" << w.index;
	}

	struct State {
		Style				style;
		Input				input;
		vector<Window*>		windows;
		Widget				current;
		Widget				hot;
		Widget				hotToBe;
		Widget				active;

		Vector2				initPos;	// initial widget position
		Vector2				widgetPos;	// current widget position
		const Color*		currColor;
		Vector2				drag;		// can be either mouse offset relative to the widget being dragged, or the total amount

		// OpenGL related
		GLuint				program;
		GLuint				vao;		// vertex array
		GLuint				vbo;		// vertex buffer
		GLuint				ebo;		// element buffer
		GLuint				vertShader;
		GLuint				fragShader;
		GLuint				locProj;		// proj: projection matrix
		GLuint				locTex;			// tex: texture
		GLuint				locPos;			// pos: vertex position
		GLuint				locUV;			// uv:	texture coordinate
		GLuint				locCol;			// col: color
		
		// Get the pointer to the window, will create a new one if it didn't exist
		Window* GetWindow(const char *name){
			Window *result = nullptr;
			if (name){
				// search for existing windows
				uint32 id = Window::GetID(name);
				for (Window *w : windows){
					if (w->id == id){
						result = w;
						break;
					}
				}
				if (!result){
					windows.push_back(new Window(id));
					result = windows.back();
				}
				result->accessed = true;
			}
			return result;
		}
		Window* NextWindow(const char *name)						{ current.Reset(GetWindow(name)); return current.window; }
		const Widget& NextItem()									{ current.itemId++; current.index = 0; return current; }
		const Widget& NextIndex()									{ current.index++; return current; }
		const void AdvanceLine(bool pad = false, int lines = 1)		{ widgetPos.y += style.LineHeight * lines; if (pad) widgetPos.y += style.WidgetPadding; }
		const int CompareDist(const Widget& w1, const Widget& w2) {
			if (w1.window == w2.window) return 0;
			if (!w1.window) return 1;
			if (!w2.window) return -1;
			for (Window *w : windows)
				if (w == w1.window || w == w2.window)
					return w == w1.window ? 1 : -1;
			assert(false);	// window does not exist
			return 0;	
		}
		float CenterPadding(float containerSize, float elementSize){
			return (containerSize - elementSize) * 0.5f;
		}
	};
	static State G;

	////////////////////////////////////////////////////////////////////
	// Helper declaration
	////////////////////////////////////////////////////////////////////

	typedef std::string (Tagger)(const char* tagName, void *val);
	typedef void (StringCallback)(const std::string& str);

	bool IsHot();
	bool IsActive();
	void SetHot();
	void SetActive();
	void ClearActive();
	bool CheckMouse(MouseButton button, MouseButtonState buttonState);
	bool CheckButton(MouseButton wheel);
	void WordWrap(const FontMap& font, const char* text, float maxWidth, StringCallback processLine);
	void ScrollBar(const Rect& hotArea, float areaHeight, float contentHeight, float& scroll);
	void Scroll(float& scroll, int step, float contentHeight);
	////////////////////////////////////////////////////////////////////
	// API implementation
	////////////////////////////////////////////////////////////////////

	void Init(FontMap& font){
		G.style.Font = &font;
		G.style.Update();
		const GLchar *vertShaderSrc = R"(
			#version 330
			uniform mat4 proj;
			in vec2 pos;
			in vec2 uv;
			in vec4 col;
			out vec2 fragUV;
			out vec4 fragCol;
			void main(){
				fragUV = uv;
				fragCol = col;
				gl_Position = proj * vec4(pos.xy,0,1);
			})";
		const GLchar *fragShaderSrc = R"(
			#version 330
			#define PRECISION 0.00001
			uniform sampler2D tex;
			in vec2 fragUV;
			in vec4 fragCol;
			out vec4 outCol;

			bool not_zero(float val){
				return step(-PRECISION, val) * (1.0 - step(PRECISION, val)) == 0.0;
			}

			void main(){
				outCol = fragCol;
				if (not_zero(fragUV.s) || not_zero(fragUV.t)) 
					outCol.w = texture(tex, fragUV.st).w;
			})";

		G.program = glCreateProgram();
		G.vertShader = glCreateShader(GL_VERTEX_SHADER);
		G.fragShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(G.vertShader, 1, &vertShaderSrc, 0);
		glShaderSource(G.fragShader, 1, &fragShaderSrc, 0);
		glCompileShader(G.vertShader);
		glCompileShader(G.fragShader);
		glAttachShader(G.program, G.vertShader);
		glAttachShader(G.program, G.fragShader);
		glLinkProgram(G.program);

		G.locProj = glGetUniformLocation(G.program, "proj");
		G.locTex = glGetUniformLocation(G.program, "tex");
		G.locPos = glGetAttribLocation(G.program, "pos");
		G.locUV = glGetAttribLocation(G.program, "uv");
		G.locCol = glGetAttribLocation(G.program, "col");
		
		glGenBuffers(1, &G.vbo);
		glGenBuffers(1, &G.ebo);

		glGenVertexArrays(1, &G.vao);
		glBindVertexArray(G.vao);
		glBindBuffer(GL_ARRAY_BUFFER, G.vbo);
		glEnableVertexAttribArray(G.locPos);
		glEnableVertexAttribArray(G.locUV);
		glEnableVertexAttribArray(G.locCol);

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
		glVertexAttribPointer(G.locPos, 2, GL_FLOAT, GL_FALSE, sizeof(DrawVert), (GLvoid*)OFFSETOF(DrawVert, pos));
		glVertexAttribPointer(G.locUV, 2, GL_FLOAT, GL_FALSE, sizeof(DrawVert), (GLvoid*)OFFSETOF(DrawVert, uv));
		glVertexAttribPointer(G.locCol, 4, GL_FLOAT, GL_TRUE, sizeof(DrawVert), (GLvoid*)OFFSETOF(DrawVert, col));
#undef OFFSETOF

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	void Shutdown(){
		for (Window *w : G.windows) MemDelete(w);

		if (G.vao) glDeleteVertexArrays(1, &G.vao);
		if (G.vbo) glDeleteBuffers(1, &G.vbo);
		if (G.ebo) glDeleteBuffers(1, &G.ebo);
		G.vao = G.vbo = G.ebo = 0;

		glDeleteProgram(G.program);
		glDeleteShader(G.vertShader);
		glDeleteShader(G.fragShader);
		G.program = G.vertShader = G.fragShader = 0;
	}

	void BeginFrame(const Input& input){
		G.input = input;
		G.current.Reset();
		G.hot = G.hotToBe;
		G.hotToBe.Reset();

		for (Window *w : G.windows) w->Reset();
	}
	void EndFrame(){
		// focus on the window where the active widget is located
		if (G.active.HasValue()){
			Window *window = G.active.window;
			// move the window to the end
			if (window != G.windows.back()){
				for (int i = 0; i < G.windows.size(); i++)
					if (G.windows[i] == window)
						G.windows.erase(G.windows.begin() + i);
				G.windows.push_back(window);
			}
		}
		// delete windows we didn't touch in this frame
		G.windows.erase(
			std::remove_if(
				G.windows.begin(), G.windows.end(), 
				[](Window *w) {bool die = !w->accessed; if (die) delete w; return die; }), 
			G.windows.end());

		// ============================================================
		// backup program & texture
		GLint lastProgram, lastTexture;
		glGetIntegerv(GL_CURRENT_PROGRAM, &lastProgram);
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &lastTexture);
		
		// setup
		const float w = G.input.screen.x;
		const float h = G.input.screen.y;
		const Matrix4x4 orthoProjection(
			2.f/w,	0.f,	0.f,	0.f,
			0.f,	2.f/-h,	0.f,	0.f,
			0.f,	0.f,	-1.f,	0.f,
			-1.f,	1.f,	0.f,	1.f);

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glEnable(GL_SCISSOR_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquation(GL_FUNC_ADD);
		glActiveTexture(GL_TEXTURE0);

		// Render windows
		glUseProgram(G.program);
		glUniform1i(G.locTex, 0);
		glUniformMatrix4fv(G.locProj, 1, GL_FALSE, &orthoProjection[0][0]);
		glBindVertexArray(G.vao);
		glBindTexture(GL_TEXTURE_2D, G.style.Font->TexID());
		for (Window *w : G.windows){
			const DrawIdx *idxBufOffset = 0;
			DrawList& drawList = w->drawList;
			if (drawList.cmdBuf.size() > 0){
				glBindBuffer(GL_ARRAY_BUFFER, G.vbo);
				glBufferData(GL_ARRAY_BUFFER,
					(GLsizeiptr)drawList.vtxBuf.size() * sizeof(DrawVert),
					(GLvoid*)drawList.vtxBuf.data(),
					GL_STREAM_DRAW);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, G.ebo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER,
					(GLsizeiptr)drawList.idxBuf.size() * sizeof(DrawIdx),
					(GLvoid*)drawList.idxBuf.data(),
					GL_STREAM_DRAW);
				for (const DrawCmd& cmd : drawList.cmdBuf){
					glScissor((int)cmd.clipRect.min.x, (int)(h - cmd.clipRect.max.y), (int)cmd.clipRect.Width(), (int)cmd.clipRect.Height());
					glDrawElements(GL_TRIANGLES,
						(GLsizei)cmd.idxCount,
						GL_UNSIGNED_SHORT,
						idxBufOffset);
					idxBufOffset += cmd.idxCount;
				}
			}
		}

		// restore program texture
		glDisable(GL_SCISSOR_TEST);
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glUseProgram(lastProgram);
		glBindTexture(GL_TEXTURE_2D, lastTexture);
	}
	//	+---------------------------------------+
	//  |                 1                     |
	//  +---------------------------------------+
	//  |                                       |
	//  |                 2                     |
	//  |                                       |
	//  +-----------------------------------+---+
	//  |                 3                 | 4 |
	//  +-----------------------------------+---+
	//  1 - Header, 2 - Body(Scrollable), 3 - Bottom, 4 - Resize
	void BeginWindow(const char *name, Rect& rect){
		Window *W = G.NextWindow(name);
		W->drawList.PushClipRect(rect);
		float padding = G.style.WindowPadding;
		float textPadding = G.style.TextPaddingY;
		Rect header(rect.min, Vector2(rect.max.x, rect.min.y + padding));
		Rect body(
			Vector2(rect.min.x, rect.min.y + padding),
			Vector2(rect.max.x, rect.max.y - padding));
		Rect bottom(
			Vector2(rect.min.x, rect.max.y - padding),
			Vector2(rect.max.x - padding, rect.max.y));

		#pragma region window logic
		if (IsActive()){
			if (CheckMouse(MouseButton::LEFT, MouseButtonState::UP)){
				ClearActive();
			}
			else {
				Rect dragArea(Vector2::ZERO, G.input.screen - Vector2(padding));
				rect.MoveTo(dragArea.ClosestPoint(G.input.mouse - G.drag));
			}
		}
		if (IsHot()){
			if (CheckMouse(MouseButton::LEFT, MouseButtonState::DOWN)){
				SetActive();
				G.drag = G.input.mouse - rect.min;
			}
			else if (CheckButton(MouseButton::WHEEL_DOWN)){
				Scroll(W->scroll, 1, W->contentHeight);
			}
			else if (CheckButton(MouseButton::WHEEL_UP)){
				Scroll(W->scroll, -1, W->contentHeight);
			}
		}
		if (body.Contains(G.input.mouse) || header.Contains(G.input.mouse) || bottom.Contains(G.input.mouse)) {
			SetHot();
		}
		#pragma endregion
		#pragma region window rendering

		// Header
		W->drawList.AddTriangle(
			Vector2(rect.min.x, rect.min.y + padding),
			rect.min + Vector2(padding),
			Vector2(rect.min.x + padding, rect.min.y),
			G.style.Colors[Style::Palette::Accent]);
		W->drawList.AddRect(
			Vector2(rect.min.x + padding, rect.min.y),
			header.max,
			G.style.Colors[Style::Palette::Accent]);
		W->drawList.AddText(
			rect.min.x + padding + textPadding,
			rect.min.y + padding - textPadding,
			G.style.Font,
			name,
			G.style.Colors[Style::Palette::Text]);
		// Body
		W->drawList.AddRect(body.min, body.max, G.style.Colors[Style::Palette::Background]);
		// Bottom
		W->drawList.AddRect(bottom.min, bottom.max, G.style.Colors[Style::Palette::Background]);
		#pragma endregion

		// Resize handle
		G.NextItem();
		Rect resize(rect.max - padding, rect.max);
		#pragma region resize handle logic
		Color *resizeColor = &G.style.Colors[Style::Palette::Accent];
		if (resize.Contains(G.input.mouse)){
			SetHot();
		}
		if (IsHot()){
			resizeColor = &G.style.Colors[Style::Palette::AccentHighlight];
			if (CheckMouse(MouseButton::LEFT, MouseButtonState::DOWN)){
				SetActive();
				G.drag = G.input.mouse - rect.max;
			}
		}
		if (IsActive()){
			resizeColor = &G.style.Colors[Style::Palette::AccentActive];
			if (CheckMouse(MouseButton::LEFT, MouseButtonState::UP)){
				ClearActive();
			}
			else {
				rect.max.x = Math::Max(rect.min.x + 5 * padding, G.input.mouse.x - G.drag.x);
				rect.max.y = Math::Max(rect.min.y + 6 * padding, G.input.mouse.y - G.drag.y);
			}
		}
		#pragma endregion
		#pragma region resize handle rendering
		W->drawList.AddTriangle(body.max, bottom.TR(), bottom.max, *resizeColor, true);
		#pragma endregion
		
		// Scroll bar
		Rect scrollBarArea(
			body.max.x - padding,
			body.min.y,
			body.max.x,
			body.max.y);
		Rect scrollRect = Rect(
			body.min.x + padding,
			body.min.y + G.style.WidgetPadding,
			body.max.x - padding,
			body.max.y);
		float scrollRectHeight = scrollRect.Height();
		float scrollRectCenterY = scrollRect.min.y + scrollRectHeight * 0.5f;
		ScrollBar(scrollBarArea, scrollRectHeight, W->contentHeight, W->scroll);

		float contentOffset = (W->contentHeight > scrollRectHeight) ? W->scroll * (W->contentHeight - scrollRectHeight) : 0.f;
		G.initPos = G.widgetPos = Vector2(scrollRect.min.x, scrollRect.min.y - contentOffset);
		
		// Update clip rect
		W->drawList.PushClipRect(scrollRect);
	}

	void EndWindow(){
		G.current.window->contentHeight = G.widgetPos.y - G.initPos.y;
	}

	void Divider(){
		Vector2 points[2];
		const Rect& clipRect = G.current.window->GetRect();
		points[0] = G.widgetPos;
		points[1].x = clipRect.max.x;
		points[1].y = G.widgetPos.y;
		#pragma region rendering
		G.current.window->drawList.AddPolyLine(
			points, 2,
			G.style.Colors[Style::Palette::Hint],
			false, 1.f);
		#pragma endregion
		G.widgetPos.y += G.style.WidgetPadding;
	}
	void Text(const char *text, bool isHint){
		G.NextItem();
		G.currColor = &G.style.Colors[isHint ? Style::Palette::Hint : Style::Palette::Text];

		#pragma region rendering
		WordWrap(*G.style.Font, text, G.current.window->GetRect().Width(), [](const std::string& line){
			G.AdvanceLine();
			G.current.window->drawList.AddText(
				G.widgetPos.x,
				G.widgetPos.y - G.style.TextPaddingY,
				G.style.Font,
				line.data(), *G.currColor);
		});
		G.AdvanceLine(true, 0);
		#pragma endregion
	}
	bool Button(const char *name, bool enabled){
		G.NextItem(); bool clicked = false;

		Color *bgColor = &G.style.Colors[Style::Palette::Accent];
		float textWidth = G.style.Font->GetWidth(name);
		Rect button(G.widgetPos, G.widgetPos + Vector2(textWidth + 2.f * G.style.TextPaddingX, G.style.LineHeight));
		bool hovering = button.Contains(G.input.mouse);
		#pragma region logic
		if (hovering) {
			SetHot();
		}
		if (IsHot()){
			bgColor = &G.style.Colors[Style::Palette::AccentHighlight];
			if (CheckMouse(MouseButton::LEFT, MouseButtonState::DOWN)){
				SetActive();
			}
		}
		if (IsActive()){
			bgColor = &G.style.Colors[Style::Palette::AccentActive];
			if (CheckMouse(MouseButton::LEFT, MouseButtonState::UP)){
				ClearActive();
				if (hovering){
					clicked = true;
				}
			}
		}
		#pragma endregion
		#pragma region rendering
		G.current.window->drawList.AddRect(button.min, button.max, *bgColor, true);
		G.current.window->drawList.AddText(
			button.min.x + G.style.TextPaddingX,
			button.max.y - G.style.TextPaddingY,
			G.style.Font,
			name,
			G.style.Colors[Style::Palette::Text]);
		G.AdvanceLine(true);
		#pragma endregion
		return clicked;
	}
	template <typename T>
	bool Slider(const char *name, T *val, T min, T max, T step, Tagger getTag){
		G.NextItem(); bool changed = false;
		G.widgetPos.y += G.style.WidgetPadding;

		Color *sliderColor = &G.style.Colors[Style::Palette::Accent];
		Color *trackColor = sliderColor;
		float sliderSize = G.style.HalfLineHeight();
		float halfSliderSize = sliderSize / 2.f;

		Rect hotArea(
			G.widgetPos.x, G.widgetPos.y + sliderSize,
			G.current.window->GetRect().max.x, G.widgetPos.y + G.style.LineHeight);
		Vector2 slider(G.widgetPos.x + halfSliderSize, G.widgetPos.y + G.style.LineHeight * 0.75f);
		float length = hotArea.Width() - sliderSize;
		#pragma region logic
		if (hotArea.Contains(G.input.mouse))
			SetHot();
		if (IsHot()){
			trackColor = sliderColor = &G.style.Colors[Style::Palette::AccentHighlight];
			if (CheckMouse(MouseButton::LEFT, MouseButtonState::DOWN))
				SetActive();
		}
		if (IsActive()){
			trackColor = &G.style.Colors[Style::Palette::AccentHighlight];
			sliderColor = &G.style.Colors[Style::Palette::AccentActive];
			T newVal = Math::Lerp(Math::Clamp(G.input.mouse.x - slider.x, 0.f, length) / length, min, max);
			if (step > Math::EPSILON){
				newVal = Math::Clamp(Math::Round(float(newVal - min) / step) * step + min, min, max);
			}
			if (newVal != *val){
				*val = newVal;
				changed = true;
			}
			if (CheckMouse(MouseButton::LEFT, MouseButtonState::UP))
				ClearActive();
		}
		float offset = Math::Clamp(float(*val - min) / (max - min) * length, 0.f, length);
		#pragma endregion
		#pragma region rendering
		// Text
		G.current.window->drawList.AddText(
			hotArea.min.x, hotArea.min.y - G.style.TextPaddingY,
			G.style.Font, getTag(name, val).data(),
			G.style.Colors[Style::Palette::Text]);
		// Slider
		Vector2 trackLine[2] = { slider, slider + Vector2(length, 0.f) };
		slider.x += offset;
		G.current.window->drawList.AddPolyLine(trackLine, 2, *trackColor, false, G.style.ScrollBarWidth);
		G.current.window->drawList.AddRect(
			slider - halfSliderSize,
			slider + halfSliderSize,
			*sliderColor,
			true);

		G.AdvanceLine(true);
		#pragma endregion
		return changed;
	}
	bool FloatSlider(const char *name, float& val, float min, float max, float step){
		return Slider<float>(name, &val, min, max, step, [](const char *name, void *v){
			std::ostringstream text;
			text << name << ":  " << std::setprecision(4) << std::fixed << *((float *)v);
			return text.str();
		});
	}
	bool IntSlider(const char *name, int& val, int min, int max, int step){
		return Slider<int>(name, &val, min, max, step, [](const char *name, void *v){
			std::ostringstream text;
			text << name << ":  " << *((int *)v);
			return text.str();
		});
	}
	bool RadioButton(const char *name, int& val, int itemVal){
		G.NextItem(); bool changed = false;

		Color *holeColor = &G.style.Colors[Style::Palette::Accent];
		Rect hotArea(G.widgetPos, G.widgetPos + Vector2(G.style.LineHeight));
		#pragma region logic
		if (hotArea.Contains(G.input.mouse)) SetHot();
		else ClearActive();
		if (IsHot()){
			holeColor = &G.style.Colors[Style::Palette::AccentHighlight];
			if (CheckMouse(MouseButton::LEFT, MouseButtonState::DOWN)){
				SetActive();
			}
		}
		if (IsActive()){
			holeColor = &G.style.Colors[Style::Palette::AccentActive];
			if (CheckMouse(MouseButton::LEFT, MouseButtonState::UP)){
				if (val != itemVal){
					val = itemVal;
					changed = true;
				}
				ClearActive();
			}
		}
		#pragma endregion
		#pragma region rendering
		Vector2 center = hotArea.Center();
		G.current.window->drawList.AddCircle(
			center,
			G.style.FormWidgetRadius,
			*holeColor, true);
		if (val == itemVal){
			G.current.window->drawList.AddCircle(
				center,
				G.style.FormWidgetSelectedRadius,
				G.style.Colors[Style::Palette::AccentActive],
				true);
		}
		G.current.window->drawList.AddText(
			hotArea.max.x + G.style.TextPaddingX,
			hotArea.max.y - G.style.TextPaddingY,
			G.style.Font,
			name,
			G.style.Colors[Style::Palette::Text]);
		#pragma endregion
		G.AdvanceLine(true);
		return changed;
	}
	bool CheckBox(const char *name, bool& val){
		G.NextItem(); bool changed = false;

		Color *boxColor = &G.style.Colors[Style::Palette::Accent];
		Rect hotArea(G.widgetPos, G.widgetPos + Vector2(G.style.LineHeight));
		#pragma region logic
		if (hotArea.Contains(G.input.mouse)) SetHot();
		else ClearActive();
		if (IsHot()){
			boxColor = &G.style.Colors[Style::Palette::AccentHighlight];
			if (CheckMouse(MouseButton::LEFT, MouseButtonState::DOWN)){
				SetActive();
			}
		}
		if (IsActive()){
			boxColor = &G.style.Colors[Style::Palette::AccentActive];
			if (CheckMouse(MouseButton::LEFT, MouseButtonState::UP)){
				ClearActive();
				val = !val;
				changed = true;
			}
		}
		#pragma endregion
		#pragma region rendering
		Vector2 center = hotArea.Center();
		G.current.window->drawList.AddRect(
			center - G.style.FormWidgetRadius,
			center + G.style.FormWidgetRadius,
			*boxColor, true);
		if (val){
			G.current.window->drawList.AddRect(
				center - G.style.FormWidgetSelectedRadius,
				center + G.style.FormWidgetSelectedRadius,
				G.style.Colors[Style::Palette::AccentActive],
				true);
		}
		G.current.window->drawList.AddText(
			hotArea.max.x + G.style.TextPaddingX,
			hotArea.max.y - G.style.TextPaddingY,
			G.style.Font,
			name,
			G.style.Colors[Style::Palette::Text]);
		G.AdvanceLine(true);
		#pragma endregion
		return changed;
	}
	void ProgressBar(const char *name, const float& percent){
		G.NextItem();

		float padding = G.style.HalfLineHeight() / 2.f;
		float fullLength = G.current.window->GetRect().max.x - G.widgetPos.x - 2.f * padding;
		float barLength = fullLength * Math::Clamp(percent, 0.f, 1.f);

		#pragma region rendering
		// Text
		G.current.window->drawList.AddText(
			G.widgetPos.x, G.widgetPos.y + G.style.HalfLineHeight() - G.style.TextPaddingY,
			G.style.Font, name,
			G.style.Colors[Style::Palette::Text]);
		// Progress bar
		Vector2 line[3];
		line[0] = Vector2(G.widgetPos.x + padding, G.widgetPos.y + G.style.HalfLineHeight() + padding);
		line[1] = line[0] + Vector2(barLength, 0.f);
		line[2] = line[0] + Vector2(fullLength, 0.f);
		if (line[0].x < line[1].x)
			G.current.window->drawList.AddPolyLine(line, 2, G.style.Colors[Style::Palette::AccentActive], false, G.style.ProgressBarWidth);
		if (line[1].x < line[2].x)
			G.current.window->drawList.AddPolyLine(line + 1, 2, G.style.Colors[Style::Palette::Accent], false, G.style.ProgressBarWidth);
		
		G.AdvanceLine(true);
		#pragma endregion
	}

	////////////////////////////////////////////////////////////////////
	// Helper implementations
	////////////////////////////////////////////////////////////////////

	bool IsHot(){ return G.hot.HasValue() && G.hot == G.current; }
	bool IsActive(){ return G.active.HasValue() && G.active == G.current; }
	void SetHot(){
		if (!G.active.HasValue() && 
			G.hotToBe != G.current && 
			G.CompareDist(G.hotToBe, G.current) >= 0) {
			G.hotToBe = G.current;
		}
	}
	void SetActive(){
		G.active = G.current;
	}
	void ClearActive(){
		if (IsActive())
			G.active.Reset();
	}
	bool CheckMouse(MouseButton button, MouseButtonState buttonState){
		return G.input.button == button && G.input.buttonState == buttonState;
	}
	bool CheckButton(MouseButton button){
		return G.input.button == button;
	}
	void WordWrap(const FontMap& font, const char* text, float maxWidth, StringCallback processLine){
		std::istringstream words(text);
		std::ostringstream line;
		std::string word;
		float wordWidth, spaceLeft = maxWidth;
		const float spaceWidth = font.GetWidth(" ");
		while (words >> word){
			wordWidth = font.GetWidth(word.data());
			while (word.length() > 0){
				if (spaceLeft < wordWidth + spaceWidth){
					if (spaceLeft == maxWidth){
						// break down the word without hyphenation
						int length = word.length();
						float segWidth = 0.f;
						int charCount = 0;
						float charWidth = font.GetWidth(word[charCount]);
						while (segWidth + charWidth < maxWidth && charCount < length){
							segWidth += charWidth;
							charWidth = font.GetWidth(word[++charCount]);
						}
						processLine(charCount != length ? std::string(word.begin(), word.begin() + charCount) : word);
						wordWidth -= segWidth;
						word.erase(0, charCount);
					}
					else {
						processLine(line.str());
					}
					line.clear();
					line.str("");
					spaceLeft = maxWidth;
				}
				else {
					line << word << ' ';
					word.clear();
					spaceLeft -= wordWidth + spaceWidth;
				}
			}
		}
		if (line.tellp() > 0){
			processLine(line.str());
		}
	}
	void ScrollBar(const Rect& hotArea, float areaHeight, float contentHeight, float& scroll){
		G.NextItem();

		Color* barColor = &G.style.Colors[Style::Palette::Accent];
		float areaWidth = hotArea.Width();
		float padding = areaWidth * 0.35f;
		float ratio = areaHeight / contentHeight;
		float barHalfHeight = Math::Max(G.style.WidgetPadding, (hotArea.Height() - 2 * padding) * ratio) * 0.5f;
		float rangeStart = hotArea.min.y + padding + barHalfHeight;
		float rangeEnd = hotArea.max.y - padding - barHalfHeight;

		if (rangeStart >= rangeEnd)
			return;

		float barCenterY = Math::Lerp(scroll, rangeStart, rangeEnd);

		#pragma region logic
		if (hotArea.Contains(G.input.mouse)){
			SetHot();
		}
		if (IsHot()){
			bool hovering = Math::InBounds(
				G.input.mouse.y, 
				barCenterY - barHalfHeight, 
				barCenterY + barHalfHeight);
			if (hovering)
				barColor = &G.style.Colors[Style::Palette::AccentHighlight];
			if (CheckMouse(MouseButton::LEFT, MouseButtonState::DOWN)){
				SetActive();
				G.drag.y = hovering ? G.input.mouse.y - barCenterY : 0.f;
			}
			else if (CheckButton(MouseButton::WHEEL_DOWN)){
				Scroll(scroll, 1, contentHeight);
			}
			else if (CheckButton(MouseButton::WHEEL_UP)){
				Scroll(scroll, -1, contentHeight);
			}
		}
		if (IsActive()){
			barColor = &G.style.Colors[Style::Palette::AccentActive];
			barCenterY = Math::Clamp(G.input.mouse.y - G.drag.y, rangeStart, rangeEnd);
			scroll = Math::InvLerp(barCenterY, rangeStart, rangeEnd);
			if (CheckMouse(MouseButton::LEFT, MouseButtonState::UP)){
				ClearActive();
			}
		}
		#pragma endregion

		#pragma region rendering

		Rect bar(
			hotArea.min.x + padding,
			barCenterY - barHalfHeight,
			hotArea.max.x - padding,
			barCenterY + barHalfHeight);
		G.current.window->drawList.AddRect(bar.min, bar.max, *barColor);
		#pragma endregion
	}
	void Scroll(float& scroll, int step, float contentHeight){ 
		scroll = Math::Clamp(scroll + step * G.style.ScrollSpeed / contentHeight, 0.f, 1.f); 
	}
}}}