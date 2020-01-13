#include "DXCanvas.h"

//

namespace DX {
	Canvas DXCanvas;
	namespace Objects {
		extern Font f_defaultFont("Gravedigger", 18);
	}

	FadeTransition::FadeTransition(Timer timer, Color startColor, Color endColor, Color* color) {
		this->timer = timer;
		this->startColor = startColor;
		this->endColor = endColor;
		this->color = color;
	}

	bool FadeTransition::tick(int deltaTime) {
		timer.elapsed += deltaTime;
		if (timer.elapsed >= timer.start) {
			if (timer.elapsed - timer.start >= timer.duration) {
				return false;
			}
			else {
				float delta = (((timer.elapsed - timer.start) * 1.0f) / timer.duration);
				color->r = startColor.r + (endColor.r - startColor.r) * delta;
				color->g = startColor.g + (endColor.g - startColor.g) * delta;
				color->b = startColor.b + (endColor.b - startColor.b) * delta;
				color->a = startColor.a + (endColor.a - startColor.a) * delta;
			}
		}
		return true;
	}

	MoveTransition::MoveTransition(Timer timer, Position startPosition, Position endPosition, Position* position) {
		this->timer = timer;
		this->startPos = startPosition;
		this->endPos = endPosition;
		this->position = position;
	}

	bool MoveTransition::tick(int deltaTime) {
		timer.elapsed += deltaTime;
		if (timer.elapsed >= timer.start) {
			if (timer.elapsed - timer.start >= timer.duration) {
				return false;
			}
			else {
				float delta = (((timer.elapsed - timer.start) * 1.0f) / timer.duration);
				position->x = startPos.x + (endPos.x - startPos.x) * delta;
				position->y = startPos.y + (endPos.y - startPos.y) * delta;
			}
		}
		return true;
	}

	void Drawable::draw(void) {
		;
	}

	void Drawable::setTimer(Timer timer) {
		this->timer = timer;
	}

	bool Drawable::tick(int deltaTime) {
		timer.elapsed += deltaTime;
		if (timer.elapsed >= timer.start) {
			if (timer.elapsed - timer.start >= timer.duration && timer.duration != -1) {
				for (std::list<FadeTransition*>::iterator i = fadeTransitions.begin(); i != fadeTransitions.end();) {
					delete* i;
					i = fadeTransitions.erase(i);
				}
				for (std::list<MoveTransition*>::iterator i = moveTransitions.begin(); i != moveTransitions.end();) {
					delete* i;
					i = moveTransitions.erase(i);
				}
				return false;
			}
		}

		for (std::list<FadeTransition*>::iterator i = fadeTransitions.begin(); i != fadeTransitions.end();) {
			bool b = (*i)->tick(deltaTime);
			if (!b) {
				delete* i;
				i = fadeTransitions.erase(i);
			}
			else
				i++;
		}

		for (std::list<MoveTransition*>::iterator i = moveTransitions.begin(); i != moveTransitions.end();) {
			bool b = (*i)->tick(deltaTime);
			if (!b) {
				delete* i;
				i = moveTransitions.erase(i);
			}
			else
				i++;
		}

		return true;
	}

	void Drawable::setColor(Color* color) {
		this->color = *color;
	}

	void Drawable::setPosition(Position* position) {
		this->position = *position;
	}

	void Drawable::addFadeTransition(Timer timer, Color startColor, Color endColor, Color* n) {
		fadeTransitions.push_back(new FadeTransition(timer, startColor, endColor, &color));
	}

	void Drawable::addMoveTransition(Timer timer, Position startPosition, Position endPosition, Position* n) {
		moveTransitions.push_back(new MoveTransition(timer, startPosition, endPosition, &position));
	}

	void Drawable::timeout(void) {
		timer.start = 0;
		timer.duration = 0;
		color.r = 0;
		color.g = 0;
		color.b = 0;
		color.a = 0;
	}

	bool Drawable::isTimedout(void) {
		return timer.remaining <= 0;
	}

	void DrawableText::draw(void) {
		// Crash in here
		Drawable::draw();

		if (sizeWidth < -1 || sizeWidth > 50 || size < -1 || size > 50)
			return;

		if (!font || !text || !font->getFont() || !font->isCreated())
			return;

		//if (!DXCanvas.isReady())
		//	return;

		RECT font_rect = { position.x, position.y, position.x, position.y + this->font->getHeight() };
		LPD3DXFONT font = this->font->getFont();
		HRESULT h, hr;

		if (!wchar) {
			h = font->DrawTextA(NULL, (char*)text, sizeWidth, &font_rect, DT_CALCRECT, D3DCOLOR_RGBA(color.r, color.g, color.b, color.a));
		}
		else {
			h = font->DrawTextW(NULL, (wchar_t*)text, sizeWidth, &font_rect, DT_CALCRECT, D3DCOLOR_RGBA(color.r, color.g, color.b, color.a));
		}

		/*
		if (h_center)
			font_rect.left -= abs(font_rect.left - font_rect.right) / 2;
		if (v_center)
			;
		*/

		if (align == center) {
			font_rect.left -= abs(font_rect.left - font_rect.right) / 2;
		}
		else if (align == right) {
			font_rect.left -= abs(font_rect.left - font_rect.right);
		}

		if (!wchar) {
			hr = font->DrawTextA(NULL, (char*)text, size, &font_rect, DT_LEFT | DT_NOCLIP, D3DCOLOR_RGBA(color.r, color.g, color.b, color.a));
		}
		else {
			hr = font->DrawTextW(NULL, (wchar_t*)text, size, &font_rect, DT_LEFT | DT_NOCLIP, D3DCOLOR_RGBA(color.r, color.g, color.b, color.a));
		}
	}

	bool DrawableText::tick(int deltaTime) {
		draw();

		bool b = Drawable::tick(deltaTime);

		if (!b)
			delete text;

		// Might need to put some stuff in here
		return b;
	}



	void DrawableText::setText(void* text, bool wchar) {
		this->text = text;
		this->wchar = wchar;
	}

	template<class T>
	void DrawableCollection<T>::add(T* item) {
		items.push_back(item);
		//return item;
	}

	template<class T>
	int DrawableCollection<T>::size(void) {
		return items.size();
	}

	template<class T>
	bool DrawableCollection<T>::tick(int deltaTime) {

		bool b = false;

		std::list<T*>::iterator i = items.begin();
		int size = items.size();
		int j = 0;
		for (; j < size; j++) {

			//if (i == items.end()) {
			//	
			//	std::cout << "ERROR2" << std::endl;
			//	std::cout << j << ", " << size << std::endl;
			//	//return true;
			//	//break;
			//}
			//if (!(*i)) {
			//	i++;
			//	continue;
			//}
			if (i == items.end()) {
				std::cout << "DC1" << std::endl;
				//return false;
			}
			if (!(*i)) {
				std::cout << "DC2" << std::endl;
				//i++;
				//continue;
			}

			bool t = (*i)->tick(deltaTime);
			b = b || t;
			if (!t) {
				delete* i;
				*i = NULL;
				i = items.erase(i);
			}
			else {
				i++;
			}
		}

		//std::cout << "B: " << j << ", " << size << std::endl;
		return b;
	}

	template<class T>
	void DrawableCollection<T>::setType(Drawable::Type type) {
		this->type = type;
	}

	template<class T>
	Drawable::Type DrawableCollection<T>::getType(void) {
		return this->type;
	}

	template<class T>
	void DrawableCollection<T>::timeout(void) {
		std::list<T*>::iterator i = items.begin();
		int size = items.size();
		for (int j = 0; j < size; j++) {
			if (i == items.end()) {
				return;
			}
			if (!(*i)) {
				i++;
				continue;
			}
			(*i)->timeout();
			i++;
		}
	}

	void Canvas::initialise(void) {
		initialised = true;
	}

	void Canvas::uninitialise(void) {
		initialised = false;
	}

	bool Canvas::isReady(void) {
		return initialised && textCollection == NULL && lineCollection == NULL;
	}


	void Canvas::begin(void) {
		//std::cout << "begin" << std::endl;
		textCollection = new DrawableCollection<DrawableText>;
		lineCollection = new DrawableCollection<DrawableLine>;
		uninitialise();
	}

	void Canvas::end(void) {

		//std::cout << "end" << std::endl;
		if (textCollection) {
			if (textCollection->size())
				textCollections.push_back(textCollection);
			else {
				delete textCollection;
			}
		}
		textCollection = NULL;

		if (lineCollection) {
			if (lineCollection->size())
				lineCollections.push_back(lineCollection);
			else {
				delete lineCollection;
			}
		}
		lineCollection = NULL;

		//font = &f_defaultFont;
		color.r = 255;
		color.g = 255;
		color.b = 255;
		color.a = 255;
		position.x = 0;
		position.y = 0;
		initialise();
	}

	void Canvas::drawText(void* text, int duration, unsigned int align, bool wchar, int size, int sizeWidth) {

		//wcout << (wchar_t*)text << endl;

		/*
		int s = wcslen((wchar_t*)text) + 1;
		wchar_t* p = (wchar_t*)text;
		for (int i = 0; i < s; i++) {
			wcout << p;
			p++;
		}
		wcout << endl;
		*/

		if (!textCollection) {
			cout << "textCollection is null" << endl;
			return;
		}

		DrawableText* dt = new DrawableText;
		dt->setColor(&color);
		dt->setPosition(&position);
		dt->font = font;

		void* allocatedText;
		if (wchar) {
			int s = wcslen((wchar_t*)text) + 1;
			if (s > 50 || s < 1)
				return;
			allocatedText = new wchar_t[s];
			wcscpy((wchar_t*)allocatedText, (wchar_t*)text);


		}
		else {
			int s = strlen((char*)text) + 1;
			allocatedText = new char[s];
			strcpy((char*)allocatedText, (char*)text);


		}




		dt->setText(allocatedText, wchar);
		dt->size = size;
		dt->sizeWidth = sizeWidth;
		Timer t;
		t.duration = duration;
		//t.duration = 1;
		dt->setTimer(t);

		//dt->h_center = hcenter;
		dt->align = (DrawableText::Align)align;

		//std::cout << "A1" << endl;
		if (textCollection)
			textCollection->add(dt);


		//std::cout << "A2" << endl;

		currentDrawable = dt;
	}

	void Canvas::drawLine(int x1, int y1, int x2, int y2, int radii) {

		if (!lineCollection) {
			cout << "lineCollection is null" << endl;
			return;
		}

		DrawableLine* dt = new DrawableLine(x1, y1, x2, y2, radii);
		dt->setColor(&color);
		//dt->setPosition(&position);

		Timer t;
		t.duration = 0;
		//t.duration = 1;
		dt->setTimer(t);

		if (lineCollection)
			lineCollection->add(dt);


		currentDrawable = dt;
	}

	int Canvas::getTextWidth(void* text, bool wchar) {

		if (!font || !text || !font->getFont())
			return -100;

		RECT font_rect = { 0,0,0,0 };
		LPD3DXFONT font = this->font->getFont();
		HRESULT h, hr;

		if (!wchar) {
			h = font->DrawTextA(NULL, (char*)text, -1, &font_rect, DT_CALCRECT, D3DCOLOR_RGBA(0, 0, 0, 0));
		}
		else {
			h = font->DrawTextW(NULL, (wchar_t*)text, -1, &font_rect, DT_CALCRECT, D3DCOLOR_RGBA(0, 0, 0, 0));
		}

		return font_rect.right - font_rect.left;

	}


	void Canvas::tick(int deltaTime) {
		uninitialise();
		if (deltaTime <= 0)
			;// return;

		std::list<DrawableCollection<DrawableText>*>::iterator i = textCollections.begin();
		int count = textCollections.size();
		int j = 0;
		for (; j < count; j++) {
			//if (i == textCollections.end()) {
			//	std::cout << "ERROR1" << std::endl;
			//	std::cout << j << ", " << count << std::endl;
			//	return;
			//}
			if (i == textCollections.end()) {
				std::cout << "LC1" << std::endl;
				//return;
			}
			if (!(*i)) {

				std::cout << "LC2" << std::endl;
				//i++;
				//continue;
			}
			bool b = (*i)->tick(deltaTime);
			if (!b) {

				delete (DrawableCollection<DrawableText>*)* i;
				//*i = NULL;
				i = textCollections.erase(i);
			}
			else {
				i++;
			}
		}

		//////////////////////////
		std::list<DrawableCollection<DrawableLine>*>::iterator k = lineCollections.begin();
		count = lineCollections.size();
		j = 0;
		for (; j < count; j++) {
			//if (i == textCollections.end()) {
			//	std::cout << "ERROR1" << std::endl;
			//	std::cout << j << ", " << count << std::endl;
			//	return;
			//}
			if (k == lineCollections.end()) {
				//

				std::cout << "MC1" << std::endl;
				//return;
			}
			if (!(*k)) {

				std::cout << "MC2" << std::endl;
				//i++;
				//continue;
			}
			bool b = (*k)->tick(deltaTime);
			if (!b) {

				delete (DrawableCollection<DrawableLine>*)* k;
				//*i = NULL;
				k = lineCollections.erase(k);
			}
			else {
				k++;
			}
		}

		//////////////////////////
		initialise();
		//std::cout << "A: " << j << ", " << count << std::endl;
	}

	void Canvas::setPos(int x, int y) {
		this->position.x = x;
		this->position.y = y;
	}

	void Canvas::setColor(int r, int g, int b, int a) {
		this->color.r = r;
		this->color.g = g;
		this->color.b = b;
		this->color.a = a;
	}

	void Canvas::setFont(DX::Objects::Font* font) {
		this->font = font;
	}

	void Canvas::addFadeTransition(Timer timer, Color startColor, Color endColor) {
		currentDrawable->addFadeTransition(timer, startColor, endColor, NULL);
	}

	void Canvas::addMoveTransition(Timer timer, Position startPos, Position endPos) {
		currentDrawable->addMoveTransition(timer, startPos, endPos, NULL);
	}

	void Canvas::setTimer(Timer timer) {
		currentDrawable->setTimer(timer);
	}

	int Canvas::getFontHeight(void) {
		//return ((DrawableText*)currentDrawable)->font->getHeight();
		if (font && font->getFont())
			return font->getHeight();
		else
			return 0;
	}

	void Canvas::removeTypeFromTexts(Drawable::Type type) {
		//if (deltaTime <= 0)
		// return;
		//if (!textCollections)
		//	return;

		int count = textCollections.size();
		if (count < 1)
			return;

		std::list<DrawableCollection<DrawableText>*>::iterator i = textCollections.begin();
		for (int j = 0; j < count; j++) {
			if (i == textCollections.end()) {
				return;
			}
			if (!(*i)) {
				i++;
				continue;
			}
			if ((*i)->getType() == type) {
				(*i)->timeout();
				(*i)->setType(Drawable::None);
			}
			i++;
		}
	}

	int Canvas::countTypeFromTexts(Drawable::Type type) {
		int c = 0;
		int count = textCollections.size();
		std::list<DrawableCollection<DrawableText>*>::iterator i = textCollections.begin();
		for (int j = 0; j < count; j++) {
			if (i == textCollections.end()) {
				return -1;
			}
			if (!(*i)) {
				i++;
				continue;
			}
			if ((*i)->getType() == type) {
				c++;
			}
			i++;
		}
		return c++;
	}

	void Canvas::setType(Drawable::Type type) {
		if (textCollection)
			textCollection->setType(type);
	}

	void Canvas::clear(void) {
		end();

		textCollections.clear();

		return;

		int count = textCollections.size();
		list<DrawableCollection<DrawableText>*>::iterator i = textCollections.begin();
		for (int j = 0; j < count; j++) {
			if (i == textCollections.end()) {
				return;
			}
			if (!(*i)) {
				i++;
				continue;
			}
			i = textCollections.erase(i);
		}
	}
}

namespace DX {
	void updateCanvas(void) {
		DXCanvas.uninitialise();
		unsigned long long tickCount = GetTickCount64();
		if (DXCanvas.lastTickTime != 0)// && tickCount - DXCanvas.lastTickTime>0)
			DXCanvas.tick(tickCount - DXCanvas.lastTickTime);
		DXCanvas.lastTickTime = tickCount;
		DXCanvas.initialise();
	}
}