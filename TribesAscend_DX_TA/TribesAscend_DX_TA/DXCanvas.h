#include "DX.h"

#include <iostream>
#include <list>
#include <vector>

using namespace std;

namespace DX {
	extern int i_resx;
	extern int i_resy;

	namespace Objects {
		class Font {
		private:
			unsigned int height = 0;
			unsigned int width = 0;
		public:
			LPD3DXFONT font = NULL;
			char name[40];
			bool created = false;

			Font(char* fontName, int height) {
				memcpy(name, fontName, 40);
				this->height = height;
			}

			LPD3DXFONT getFont(void) {
				return font;
			}

			unsigned int getHeight(void) {
				return height;
			}

			HRESULT create(void) {
				std::cout << "B" << std::endl;
				if (isCreated())
					return D3D_OK;
				std::cout << "C" << std::endl;
				HRESULT res = 0;
				if (DX::mainDevice) {
					std::cout << "D" << std::endl;
					res = D3DXCreateFontA(DX::mainDevice, height, 0, FW_NORMAL, 1, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, name, &font);
				}
				std::cout << "E" << std::endl;
				if (res == D3D_OK) {
					created = true;
					std::cout << "F" << std::endl;
				}
				else {
					font = NULL;
					std::cout << "G" << std::endl;
				}
				std::cout << "H" << std::endl;
				return res;
			}

			bool isCreated(void) {
				return created;
			}
		};

		extern DX::Objects::Font f_defaultFont;
	}	
}

namespace DX {

	struct Position {
		int x = 0, y = 0;
	};

	struct Color {
		int r = 255, g = 255, b = 255, a = 255;
	};

	struct Timer {
		int start = 0, duration = 1, elapsed = 0, remaining = 0;
	};

	class FadeTransition {
		//Position startPos, endPos;
		Color startColor, endColor;
		Color* color;
		Timer timer;

	public:
		FadeTransition(Timer timer, Color startColor, Color endColor, Color* color);
		bool tick(int deltaTime);

	};

	class MoveTransition {
		Position startPos, endPos;
		Position* position;
		//Color startColor, endColor;
		Timer timer;

	public:
		MoveTransition(Timer timer, Position startPosition, Position endPosition, Position* position);
		bool tick(int deltaTime);
	};

	class Drawable {
	protected:
		Position position;
		Color color;
		Timer timer;

		std::list<FadeTransition*> fadeTransitions;
		std::list<MoveTransition*> moveTransitions;

	public:

		static enum Type { Normal, Kill, Accolade, Hitmarker, hud_player, None, Hero, Type0, Type1, Type2, Type3, Type4, Type5, Type6, Type7, Type8, Type9 };

		virtual void draw(void);

		virtual bool tick(int deltaTime);

		// Set the Drawables color
		// Not implemented, will be for transitions
		void setColor(Color* color);

		void setPosition(Position* position);

		void setTimer(Timer timer);

		void addFadeTransition(Timer timer, Color startColor, Color endColor, Color* color);

		void addMoveTransition(Timer timer, Position startPosition, Position endPosition, Position* position);

		void timeout(void);

		bool isTimedout(void);
	};

	class DrawableLine : public Drawable {
		int x1, y1, x2, y2, radii;
	public:
		DrawableLine(int x1, int y1, int x2, int y2, int radii = 1) {
			this->x1 = x1;
			this->y1 = y1;
			this->x2 = x2;
			this->y2 = y2;
			this->radii = radii;
		}

		void draw(void) {
			Drawable::draw();
			int dist = sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
			int divs = 50;
			divs = dist;
			divs = 10;
			D3DRECT BarRect = { 0, 0, 1920, 1080 };
			//mainDevice->Clear(1, &BarRect, D3DCLEAR_TARGET | D3DCLEAR_TARGET, D3DCOLOR_XRGB(255, 0, 0), 0, 0);
			int x1 = this->x1;
			int y1 = this->y1;
			for (int i = 0; i < divs; i++) {
				int p_x = x1 + ((1.0 * i / divs) * (this->x2 - this->x1));
				int p_y = y1 + ((1.0 * i / divs) * (this->y2 - this->y1));
				//x1 = p_x;
				//y1 = p_y;
				BarRect = { p_x, p_y, p_x + radii, p_y + radii };
				mainDevice->Clear(1, &BarRect, D3DCLEAR_TARGET | D3DCLEAR_TARGET, D3DCOLOR_XRGB(color.r, color.g, color.b), 0, 0);
			}
		}

		bool tick(int deltaTime) {
			draw();

			bool b = Drawable::tick(deltaTime);

			// Might need to put some stuff in here
			return b;
		}

	};

	class DrawableText : public Drawable {

	public:
		static enum Align { left, center, right };
	private:

		void* text = NULL;
		bool wchar = false;
		bool v_center = false;

	public:
		Align align = center;
		bool h_center = true;

		DX::Objects::Font* font = &DX::Objects::f_defaultFont;
		int size = -1;
		int sizeWidth = -1;

		virtual void draw(void);

		virtual bool tick(int deltaTime);

		void setText(void* text, bool wchar = true);
	};

	// Not implemented yet
	class DrawableSprite : Drawable {
		virtual void draw(void) {
			Drawable::draw();
		}

		virtual bool tick(int deltaTime) {
			bool b = Drawable::tick(deltaTime);
			if (b) {
				draw();
			}
			return b;
		}
	};

	template<class T>
	class DrawableCollection {
	public:
		// Not implemented yet
		//static enum Type { Normal, Kill, Accolade, Hitmarker };
	private:
		Drawable::Type type = Drawable::Normal;
		// List of Drawable<Text/Sprite>

	public:
		std::list<T*> items;
		// Add item to the list
		void add(T* item);

		// Get list size
		int size(void);

		bool tick(int deltaTime);

		void setType(Drawable::Type type);

		Drawable::Type getType(void);

		void timeout(void);
	};

	class Canvas {
		// These are lists of collections
		std::list<DrawableCollection<DrawableText>*> textCollections;
		std::list<DrawableCollection<DrawableSprite>*> spriteCollections;
		std::list<DrawableCollection<DrawableLine>*> lineCollections;


		// There are the current collections
		DrawableCollection<DrawableText>* textCollection = NULL;
		DrawableCollection<DrawableSprite>* spriteCollection = NULL;
		DrawableCollection<DrawableLine>* lineCollection = NULL;

		//Probably wont need these two, or at least not yet
		DrawableText* currentText = NULL;
		DrawableSprite* currentSprite = NULL;
		DrawableLine* currentLine = NULL;
		Drawable* currentDrawable = NULL;

		//Canvas parameters
		Position position;
		Color color;
		
		DX::Objects::Font* font = &DX::Objects::f_defaultFont;

	public:
		bool initialised = false;
		unsigned long lastTickTime = 0;

		void initialise(void);

		void uninitialise(void);

		bool isReady(void);

		// Exposed to Lua
		// Always call begin before doing anything with Canvas
		void begin(void);

		// Exposed to Lua
		// Always call end after working with Canvas, before the function returns
		void end(void);

		// Exposed to Lua
		// Add a text string to the collection, the bool is for if using wide char
		void drawText(void* text, int duration = 0, unsigned int align = 1, bool wchar = true, int size = -1, int sizeWidth = -1);

		void drawLine(int x1, int y1, int x2, int y2, int radii = 1);

		int getTextWidth(void* text, bool wchar = true);

		// Not exposed to Lua
		// This ticks the Canvas collections -> collection -> DrawableText -> Drawable
		void tick(int deltaTime);

		// Exposed to Lua
		// Set Canvas position (X,Y where the text is drawn)
		void setPos(int x, int y);

		// Exposed to Lua
		// Set Canvas color (basically set the text color)
		void setColor(int r, int g, int b, int a);

		// Exposed to Lua
		// Set Canvas font (the font the text will use)
		void setFont(DX::Objects::Font* font);

		void addFadeTransition(Timer timer, Color startColor, Color endColor);

		void addMoveTransition(Timer timer, Position startPos, Position endPos);

		void setTimer(Timer timer);

		int getFontHeight(void);

		void removeTypeFromTexts(Drawable::Type type);

		int countTypeFromTexts(Drawable::Type type);

		void setType(Drawable::Type);

		void clear(void);
	};

	extern Canvas DXCanvas;

	void updateCanvas(void);
}